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
    node_.subscribe_to_ptp_instance (&ptpSubscriber_).wait();
}

AudioSendersModel::~AudioSendersModel()
{
    node_.unsubscribe_from_ptp_instance (&ptpSubscriber_).wait();
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

void AudioSendersModel::updateSenderConfiguration (const rav::Id senderId, rav::RavennaSender::Configuration config) const
{
    // Override audio format
    config.audio_format.byte_order = rav::AudioFormat::ByteOrder::be;
    config.audio_format.ordering = rav::AudioFormat::ChannelOrdering::interleaved;
    // Don't override num_channels, sample_rate and encoding

    auto result = node_.update_sender_configuration (senderId, std::move (config)).get();
    if (!result)
    {
        RAV_LOG_ERROR ("Failed to update sender configuration: {}", result.error());
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
    const int numOutputChannels,
    const int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    std::ignore = outputChannelData;
    std::ignore = numOutputChannels;
    std::ignore = context;

    const auto& localClock = ptpSubscriber_.get_local_clock();
    auto ptpNow = localClock.now();

    TRACY_ZONE_SCOPED;

    // If time information is available, use that
    if (context.hostTimeNs != nullptr)
        ptpNow = localClock.get_adjusted_time (*context.hostTimeNs);

    const rav::AudioBufferView outputBuffer (outputChannelData, static_cast<size_t> (numOutputChannels), static_cast<size_t> (numSamples));
    outputBuffer.clear();

    if (!deviceFormat_.is_valid())
        return;

    if (!localClock.is_calibrated())
        return;

    const auto rtpNow = static_cast<uint32_t> (ptpNow.to_rtp_timestamp (deviceFormat_.sample_rate));

    if (!rtpTs_.has_value())
    {
        if (!localClock.is_locked())
            return;
        rtpTs_ = rtpNow;
    }

    // Positive means audio device is ahead of the PTP clock, negative means behind
    const auto drift = rav::WrappingUint32 (rtpNow).diff (*rtpTs_);
    auto ratio = static_cast<double> (deviceFormat_.sample_rate) /
                 static_cast<double> (static_cast<int32_t> (deviceFormat_.sample_rate) + drift);
    ratio = std::clamp (ratio, 0.5, 1.5);
    const auto ratioFiltered = driftFilter_.update (ratio);

    TRACY_PLOT ("Sender drift", static_cast<double> (drift));
    TRACY_PLOT ("Sender asrc ratio", ratio);
    TRACY_PLOT ("Sender asrc ratio filtered", ratioFiltered);
    TRACY_PLOT ("Sender asrc ratio confidence", driftFilter_.confidence);

    const auto result = resampleProcess (
        resampler_.get(),
        inputChannelData,
        numSamples,
        resamplerBuffer_.data(),
        static_cast<int> (resamplerBuffer_.num_frames()),
        ratioFiltered);

    RAV_ASSERT_DEBUG (result.input_used == static_cast<uint32_t> (numSamples), "Num input frame mismatch");

    auto lock = realtimeSharedContext_.lock_realtime();

    for (const auto* sender : lock->senders)
        sender->processBlock (resamplerBuffer_.with_num_frames (result.output_generated).const_view(), *rtpTs_);

    *rtpTs_ += result.output_generated;
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

    const auto numInputChannels = static_cast<size_t> (device->getActiveInputChannels().countNumberOfSetBits());

    const auto maxNumFramesPerBlock = static_cast<uint32_t> (device->getCurrentBufferSizeSamples());
    resamplerBuffer_.resize (numInputChannels, maxNumFramesPerBlock * 2);

    resampler_.reset (resampleInit (static_cast<int> (deviceFormat_.num_channels), 256, 320, 1.0, SUBSAMPLE_INTERPOLATE | BLACKMAN_HARRIS));
    if (resampler_ == nullptr)
        RAV_LOG_ERROR ("Failed to initialize resampler");

    for (const auto& sender : senders_)
        sender->prepareInput (deviceFormat_);

    updateRealtimeSharedContext();
}

void AudioSendersModel::audioDeviceStopped()
{
    rtpTs_.reset();
    deviceFormat_ = {};
    resampler_.reset();
}

void AudioSendersModel::audioDeviceError (const juce::String& errorMessage)
{
    RAV_LOG_ERROR ("Audio device error: {}", errorMessage.toStdString());
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
    if (!realtimeSharedContext_.update (std::move (newContext)))
    {
        RAV_LOG_ERROR ("Failed to update realtime shared context");
    }
}

AudioSendersModel::Sender::Sender (AudioSendersModel& owner, const rav::Id senderId) : owner_ (owner), senderId_ (senderId)
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

void AudioSendersModel::Sender::processBlock (const rav::AudioBufferView<const float>& inputBuffer, const uint32_t timestamp) const
{
    std::ignore = owner_.node_.send_audio_data_realtime (senderId_, inputBuffer, timestamp);
}
