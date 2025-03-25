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

    executor_.callAsync ([this, streamId = sender.get_id(), sessionName = sender.get_session_name()] {
        RAV_ASSERT (findSender (streamId) == nullptr, "Receiver already exists");
        const auto& it = senders_.emplace_back (std::make_unique<Sender> (*this, streamId, sessionName));
        updateRealtimeSharedContext();
        for (const auto& subscriber : subscribers_)
            subscriber->onAudioSenderUpdated (streamId, &it->getState());
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
}

void AudioSenders::audioDeviceAboutToStart (juce::AudioIODevice* device) {}

void AudioSenders::audioDeviceStopped() {}

void AudioSenders::audioDeviceError (const juce::String& errorMessage)
{
    AudioIODeviceCallback::audioDeviceError (errorMessage);
}

AudioSenders::Sender::Sender (AudioSenders& owner, const rav::Id senderId, std::string sessionName) :
    owner_ (owner),
    senderId_ (senderId)
{
    state_.sessionName = std::move (sessionName);
}

rav::Id AudioSenders::Sender::getSenderId() const
{
    return senderId_;
}

const AudioSenders::SenderState& AudioSenders::Sender::getState() const
{
    return state_;
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
