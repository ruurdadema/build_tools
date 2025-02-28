/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "AudioMixer.hpp"

AudioMixer::AudioMixer (rav::ravenna_node& node) : node_ (node)
{
    node_.add_subscriber (this).wait();
}

AudioMixer::~AudioMixer()
{
    node_.remove_subscriber (this).wait();
}

void AudioMixer::ravenna_receiver_added (const rav::ravenna_receiver& receiver)
{
    RAV_ASSERT (findStreamState (receiver.get_id()) == nullptr, "Receiver already exists");

    streams_.emplace_back (StreamState { receiver.get_id() });
    node_.add_receiver_subscriber (receiver.get_id(), this).wait();
}

void AudioMixer::ravenna_receiver_removed (const rav::id receiver_id)
{
    // No need to unsubscribe from the receiver (via ravenna_node), as the stream no longer exists at this point.

    for (auto it = streams_.begin(); it != streams_.end(); ++it)
    {
        if (it->receiverId == receiver_id)
        {
            streams_.erase (it);
            break;
        }
    }
}

void AudioMixer::stream_updated (const rav::rtp_stream_receiver::stream_updated_event& event)
{
    executor_.callAsync ([this, event] {
        auto* state = findStreamState (event.receiver_id);
        if (state == nullptr)
        {
            RAV_ERROR ("Receiver state not found");
            return;
        }

        state->receiverId = event.receiver_id;
        state->selectedAudioFormat = event.selected_audio_format;
    });
}

void AudioMixer::on_data_received (const rav::wrapping_uint32 timestamp) {}

void AudioMixer::on_data_ready (const rav::wrapping_uint32 timestamp) {}

void AudioMixer::audioDeviceIOCallbackWithContext (
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    node_.realtime_read_data (rav::id (0), 0, nullptr, 0);
}

void AudioMixer::audioDeviceAboutToStart (juce::AudioIODevice* device) {}

void AudioMixer::audioDeviceStopped() {}

AudioMixer::StreamState* AudioMixer::findStreamState (const rav::id receiverId)
{
    for (auto& state : streams_)
    {
        if (state.receiverId == receiverId)
            return &state;
    }
    return nullptr;
}
