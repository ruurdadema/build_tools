/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "AudioSenders.hpp"

AudioSenders::AudioSenders (rav::RavennaNode& node) : node_ (node)
{
    node_.subscribe (this).wait();
}

AudioSenders::~AudioSenders()
{
    node_.unsubscribe (this).wait();
}

rav::Id AudioSenders::createSender() const
{
    return node_.create_sender().get();
}

void AudioSenders::removeSender (const rav::Id senderId) const
{
    return node_.remove_sender (senderId).wait();
}

void AudioSenders::updateSenderConfiguration (const rav::Id senderId, rav::RavennaSender::ConfigurationUpdate update)
    const
{
    // Override audio format
    if (update.audio_format.has_value())
    {
        update.audio_format->byte_order = rav::AudioFormat::ByteOrder::be;
        update.audio_format->ordering = rav::AudioFormat::ChannelOrdering::interleaved;
        // Don't override num_channels, sample_rate and encoding
    }

    auto result = node_.update_sender_configuration (senderId, std::move (update)).get();
    if (!result)
    {
        RAV_ERROR ("Failed to update sender configuration: {}", result.error());
    }
}

bool AudioSenders::subscribe (Subscriber* subscriber)
{
    if (subscribers_.add (subscriber))
    {
        for (const auto& sender : senders_)
            subscriber->onAudioSenderUpdated (sender->getSenderId(), &sender->getState());
        return true;
    }
    return false;
}

bool AudioSenders::unsubscribe (Subscriber* subscriber)
{
    return subscribers_.remove (subscriber);
}

void AudioSenders::ravenna_sender_added (const rav::RavennaSender& sender)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);

    executor_.callAsync ([this, senderId = sender.get_id()] {
        RAV_ASSERT (findSender (senderId) == nullptr, "Receiver already exists");
        const auto& it = senders_.emplace_back (std::make_unique<Sender> (*this, senderId));
        it->prepareInput (deviceFormat_, 0); // TODO: Number of frames
        updateRealtimeSharedContext();
    });
}

void AudioSenders::ravenna_sender_removed (rav::Id sender_id)
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

void AudioSenders::audioDeviceIOCallbackWithContext (
    const float* const* inputChannelData,
    const int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    const int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    std::ignore = outputChannelData;
    std::ignore = numOutputChannels;
    std::ignore = context;

    const rav::AudioBufferView buffer (
        inputChannelData,
        static_cast<size_t> (numInputChannels),
        static_cast<size_t> (numSamples));

    const auto lock = realtimeSharedContext_.lock_realtime();

    for (const auto* sender : lock->senders)
        sender->processBlock (buffer);
}

void AudioSenders::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    deviceFormat_ = rav::AudioFormat {
        rav::AudioFormat::ByteOrder::le,
        rav::AudioEncoding::pcm_f32,
        rav::AudioFormat::ChannelOrdering::noninterleaved,
        static_cast<uint32_t> (device->getCurrentSampleRate()),
        static_cast<uint32_t> (device->getActiveInputChannels().countNumberOfSetBits()),
    };

    for (const auto& sender : senders_)
        sender->prepareInput (deviceFormat_, 0);
}

void AudioSenders::audioDeviceStopped() {}

void AudioSenders::audioDeviceError (const juce::String& errorMessage)
{
    RAV_ERROR ("Audio device error: {}", errorMessage.toStdString());
}

AudioSenders::Sender* AudioSenders::findSender (const rav::Id senderId) const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    for (const auto& sender : senders_)
        if (sender->getSenderId() == senderId)
            return sender.get();
    return nullptr;
}

void AudioSenders::updateRealtimeSharedContext()
{
    auto newContext = std::make_unique<RealtimeSharedContext>();
    for (const auto& sender : senders_)
        newContext->senders.push_back (sender.get());
    if (!realtimeSharedContext_.update (std::move (newContext)))
    {
        RAV_ERROR ("Failed to update realtime shared context");
    }
}

AudioSenders::Sender::Sender (AudioSenders& owner, const rav::Id senderId) : owner_ (owner), senderId_ (senderId)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    owner_.node_.subscribe_to_sender (senderId_, this).wait();
}

AudioSenders::Sender::~Sender()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    owner_.node_.remove_sender (senderId_).wait();
}

rav::Id AudioSenders::Sender::getSenderId() const
{
    return senderId_;
}

const AudioSenders::SenderState& AudioSenders::Sender::getState() const
{
    return state_;
}

void AudioSenders::Sender::ravenna_sender_configuration_updated (
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

void AudioSenders::Sender::prepareInput (const rav::AudioFormat inputFormat, [[maybe_unused]] uint32_t max_num_frames)
{
    state_.inputFormat = inputFormat;

    for (auto* subscriber : owner_.subscribers_)
        subscriber->onAudioSenderUpdated (senderId_, &state_);
}

void AudioSenders::Sender::processBlock (const rav::AudioBufferView<const float>& inputBuffer) const
{
    std::ignore = owner_.node_.send_audio_data_realtime (senderId_, inputBuffer, {}); // TODO: Timestamp
}
