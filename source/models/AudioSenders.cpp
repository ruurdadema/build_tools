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

rav::Id AudioSenders::createSender (const std::string& sessionName) const
{
    return node_.create_sender (sessionName).get();
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
        update.audio_format->num_channels = deviceFormat_.num_channels;
        update.audio_format->sample_rate = deviceFormat_.sample_rate;
        // Don't override encoding
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
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    std::ignore = inputChannelData;
    std::ignore = numInputChannels;
    std::ignore = outputChannelData;
    std::ignore = numOutputChannels;
    std::ignore = numSamples;
    std::ignore = context;
}

void AudioSenders::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    deviceFormat_ = rav::AudioFormat {
        rav::AudioFormat::ByteOrder::le,
        rav::AudioEncoding::pcm_f32,
        rav::AudioFormat::ChannelOrdering::noninterleaved,
        static_cast<uint32_t> (device->getCurrentSampleRate()),
        static_cast<uint32_t> (device->getActiveOutputChannels().countNumberOfSetBits()),
    };
}

void AudioSenders::audioDeviceStopped() {}

void AudioSenders::audioDeviceError (const juce::String& errorMessage)
{
    RAV_ERROR ("Audio device error: {}", errorMessage.toStdString());
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
        state_.configuration = configuration;
        for (auto* subscriber : owner_.subscribers_)
            subscriber->onAudioSenderUpdated (sender_id, &state_);
    });
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
    for (const auto& stream : senders_)
        newContext->senders.push_back (stream.get());
    if (!realtimeSharedContext_.update (std::move (newContext)))
    {
        RAV_ERROR ("Failed to update realtime shared context");
    }
}
