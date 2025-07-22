/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "AudioSendersModel.hpp"

AudioSendersModel::AudioSendersModel (rav::RavennaNode& node) : node_ (node)
{
    node_.subscribe (this).wait();
    node_.subscribe_to_ptp_instance (this).wait();
}

AudioSendersModel::~AudioSendersModel()
{
    node_.unsubscribe_from_ptp_instance (this).wait();
    node_.unsubscribe (this).wait();
}

tl::expected<rav::Id, std::string> AudioSendersModel::createSender() const
{
    rav::RavennaSender::Configuration config;
    config.session_name = {}; // Can be left empty in which case RavennaNode will generate a name.
    config.ttl = 15;
    config.packet_time = rav::aes67::PacketTime::ms_1();
    config.payload_type = 98;

    config.audio_format.byte_order = rav::AudioFormat::ByteOrder::be;
    config.audio_format.ordering = rav::AudioFormat::ChannelOrdering::interleaved;
    config.audio_format.sample_rate = 48'000;
    config.audio_format.num_channels = 2;
    config.audio_format.encoding = rav::AudioEncoding::pcm_s16;

    config.destinations.emplace_back (
        rav::RavennaSender::Destination { rav::Rank::primary(), { boost::asio::ip::address_v4::any(), 5004 }, true });
    config.destinations.emplace_back (
        rav::RavennaSender::Destination { rav::Rank::secondary(), { boost::asio::ip::address_v4::any(), 5004 }, true });

    return node_.create_sender (config).get();
}

void AudioSendersModel::removeSender (const rav::Id senderId) const
{
    return node_.remove_sender (senderId).wait();
}

void AudioSendersModel::updateSenderConfiguration (const rav::Id senderId, rav::RavennaSender::Configuration config)
    const
{
    // Override audio format
    config.audio_format.byte_order = rav::AudioFormat::ByteOrder::be;
    config.audio_format.ordering = rav::AudioFormat::ChannelOrdering::interleaved;
    // Don't override num_channels, sample_rate and encoding

    auto result = node_.update_sender_configuration (senderId, std::move (config)).get();
    if (!result)
    {
        RAV_ERROR ("Failed to update sender configuration: {}", result.error());
    }
}

bool AudioSendersModel::subscribe (Subscriber* subscriber)
{
    if (subscribers_.add (subscriber))
    {
        for (const auto& sender : senders_)
            subscriber->onAudioSenderUpdated (sender->getSenderId(), &sender->getState());
        return true;
    }
    return false;
}

bool AudioSendersModel::unsubscribe (Subscriber* subscriber)
{
    return subscribers_.remove (subscriber);
}

void AudioSendersModel::ravenna_sender_added (const rav::RavennaSender& sender)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);

    executor_.callAsync ([this, senderId = sender.get_id()] {
        RAV_ASSERT (findSender (senderId) == nullptr, "Receiver already exists");
        const auto& it = senders_.emplace_back (std::make_unique<Sender> (*this, senderId));
        it->prepareInput (deviceFormat_);
        updateRealtimeSharedContext();
    });
}

void AudioSendersModel::ravenna_sender_removed (rav::Id sender_id)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);

    executor_.callAsync ([this, sender_id] {
        // No need to unsubscribe from the receiver (via ravenna_node), as the stream no longer exists at this point.
        for (auto it = senders_.begin(); it != senders_.end(); ++it)
        {
            if ((*it)->getSenderId() == sender_id)
            {
                std::unique_ptr<Sender> tmp = std::move (*it); // Keep alive until after the context is updated
                senders_.erase (it);
                updateRealtimeSharedContext();
                break;
            }
        }
        for (const auto& subscriber : subscribers_)
            subscriber->onAudioSenderUpdated (sender_id, nullptr);
    });
}

void AudioSendersModel::audioDeviceIOCallbackWithContext (
    const float* const* inputChannelData,
    const int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    const int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    TRACY_ZONE_SCOPED;

    std::ignore = outputChannelData;
    std::ignore = numOutputChannels;
    std::ignore = context;

    const rav::AudioBufferView outputBuffer (
        outputChannelData,
        static_cast<size_t> (numOutputChannels),
        static_cast<size_t> (numSamples));
    outputBuffer.clear();

    const rav::AudioBufferView buffer (
        inputChannelData,
        static_cast<size_t> (numInputChannels),
        static_cast<size_t> (numSamples));

    auto lock = realtimeSharedContext_.lock_realtime();

    if (!lock->deviceFormat.is_valid())
        return;

    const auto& clock = get_local_clock();
    auto ptp_ts = static_cast<uint32_t> (clock.now().to_samples (lock->deviceFormat.sample_rate));

    if (!lock->rtp_ts.has_value())
    {
        if (!clock.is_calibrated())
            return;
        lock->rtp_ts = ptp_ts;
    }

    // Positive means audio device is ahead of the PTP clock, negative means behind
    TRACY_PLOT (
        "sender drift",
        static_cast<int64_t> (rav::WrappingUint32 (ptp_ts).diff (rav::WrappingUint32 (*lock->rtp_ts))));

    for (const auto* sender : lock->senders)
        sender->processBlock (buffer, *lock->rtp_ts);

    *lock->rtp_ts += static_cast<uint32_t> (numSamples);
}

void AudioSendersModel::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    deviceFormat_ = rav::AudioFormat {
        rav::AudioFormat::ByteOrder::le,
        rav::AudioEncoding::pcm_f32,
        rav::AudioFormat::ChannelOrdering::noninterleaved,
        static_cast<uint32_t> (device->getCurrentSampleRate()),
        static_cast<uint32_t> (device->getActiveInputChannels().countNumberOfSetBits()),
    };

    for (const auto& sender : senders_)
        sender->prepareInput (deviceFormat_);

    updateRealtimeSharedContext();
}

void AudioSendersModel::audioDeviceStopped() {}

void AudioSendersModel::audioDeviceError (const juce::String& errorMessage)
{
    RAV_ERROR ("Audio device error: {}", errorMessage.toStdString());
}

AudioSendersModel::Sender* AudioSendersModel::findSender (const rav::Id senderId) const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    for (const auto& sender : senders_)
        if (sender->getSenderId() == senderId)
            return sender.get();
    return nullptr;
}

void AudioSendersModel::updateRealtimeSharedContext()
{
    auto newContext = std::make_unique<RealtimeSharedContext>();
    for (const auto& sender : senders_)
        newContext->senders.push_back (sender.get());
    newContext->deviceFormat = deviceFormat_;
    if (!realtimeSharedContext_.update (std::move (newContext)))
    {
        RAV_ERROR ("Failed to update realtime shared context");
    }
}

AudioSendersModel::Sender::Sender (AudioSendersModel& owner, const rav::Id senderId) :
    owner_ (owner),
    senderId_ (senderId)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    owner_.node_.subscribe_to_sender (senderId_, this).wait();
}

AudioSendersModel::Sender::~Sender()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    owner_.node_.unsubscribe_from_sender (senderId_, this).wait();
}

rav::Id AudioSendersModel::Sender::getSenderId() const
{
    return senderId_;
}

const AudioSendersModel::SenderState& AudioSendersModel::Sender::getState() const
{
    return state_;
}

void AudioSendersModel::Sender::ravenna_sender_configuration_updated (
    const rav::Id sender_id,
    const rav::RavennaSender::Configuration& configuration)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (owner_.node_);

    executor_.callAsync ([this, sender_id, configuration] {
        state_.senderConfiguration = configuration;
        for (auto* subscriber : owner_.subscribers_)
            subscriber->onAudioSenderUpdated (sender_id, &state_);
    });
}

void AudioSendersModel::Sender::prepareInput (const rav::AudioFormat inputFormat)
{
    state_.inputFormat = inputFormat;

    for (auto* subscriber : owner_.subscribers_)
        subscriber->onAudioSenderUpdated (senderId_, &state_);
}

void AudioSendersModel::Sender::processBlock (
    const rav::AudioBufferView<const float>& inputBuffer,
    const uint32_t timestamp) const
{
    std::ignore = owner_.node_.send_audio_data_realtime (senderId_, inputBuffer, timestamp);
}
