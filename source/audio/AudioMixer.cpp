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

#include "ravennakit/core/audio/audio_buffer_view.hpp"

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
    RAV_ASSERT (findRxStream (receiver.get_id()) == nullptr, "Receiver already exists");

    rxStreams_.emplace_back (receiver.get_id());
    node_.add_receiver_subscriber (receiver.get_id(), this).wait();
}

void AudioMixer::ravenna_receiver_removed (const rav::id receiverId)
{
    // No need to unsubscribe from the receiver (via ravenna_node), as the stream no longer exists at this point.

    for (auto it = rxStreams_.begin(); it != rxStreams_.end(); ++it)
    {
        if (it->getReceiverId() == receiverId)
        {
            rxStreams_.erase (it);
            break;
        }
    }
}

void AudioMixer::stream_updated (const rav::rtp_stream_receiver::stream_updated_event& event)
{
    executor_.callAsync ([this, event] {
        auto* stream = findRxStream (event.receiver_id);
        if (stream == nullptr)
        {
            RAV_ERROR ("Receiver state not found");
            return;
        }

        stream->prepareInput (event.selected_audio_format);
    });
}

void AudioMixer::on_data_received (const rav::wrapping_uint32 timestamp) {}

void AudioMixer::on_data_ready (const rav::wrapping_uint32 timestamp) {}

void AudioMixer::audioDeviceIOCallbackWithContext (
    const float* const* inputChannelData,
    const int numInputChannels,
    float* const* outputChannelData,
    const int numOutputChannels,
    const int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    RAV_ASSERT (numInputChannels >= 0, "Num input channels must be >= 0");
    RAV_ASSERT (numOutputChannels >= 0, "Num output channels must be >= 0");
    RAV_ASSERT (numSamples >= 0, "Num samples must be >= 0");

    rav::audio_buffer_view outputBuffer { outputChannelData,
                                          static_cast<size_t> (numOutputChannels),
                                          static_cast<size_t> (numSamples) };

    outputBuffer.clear();

    for (auto& stream : rxStreams_)
    {
        // RAV_ASSERT (streamState.formatConverter.get_source_format().is_valid(), "Invalid source format");
        // RAV_ASSERT (streamState.formatConverter.get_target_format().is_valid(), "Invalid target format");
        // RAV_ASSERT (
        //     streamState.formatConverter.get_target_format().num_channels == numOutputChannels,
        //     "Invalid number of output channels");

        // Read input from receiver via ravenna_node
        // Convert samples to float
        // Convert sample rate (including drift correction)
        // Mix samples

        // streamState.formatConverter.convert ({}, outputChannelData, numOutputChannels, numSamples);
    }
}

void AudioMixer::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    const rav::audio_format targetFormat { rav::audio_format::byte_order::le,
                                           rav::audio_encoding::pcm_f32,
                                           static_cast<uint32_t> (device->getCurrentSampleRate()),
                                           static_cast<uint32_t> (
                                               device->getActiveOutputChannels().countNumberOfSetBits()),
                                           rav::audio_format::channel_ordering::noninterleaved };

    const auto bufferSize = device->getCurrentBufferSizeSamples();

    for (auto& stream : rxStreams_)
    {
        stream.prepareOutput (targetFormat, bufferSize);
    }
}

void AudioMixer::audioDeviceStopped() {}

rav::id AudioMixer::RxStream::getReceiverId() const
{
    return receiverId_;
}

void AudioMixer::RxStream::prepareInput (const rav::audio_format format)
{
    formatConverter_.set_source_format (format);
    allocateResources();
}

void AudioMixer::RxStream::prepareOutput (const rav::audio_format format, const int maxNumFramesPerBlock)
{
    RAV_ASSERT (maxNumFramesPerBlock >= 0, "Num samples must be >= 0");
    RAV_ASSERT (format.is_valid(), "Invalid format");

    formatConverter_.set_target_format (format);
    numFramesPerBlock_ = static_cast<uint32_t> (maxNumFramesPerBlock);
    allocateResources();
}

void AudioMixer::RxStream::allocateResources() {}

AudioMixer::RxStream* AudioMixer::findRxStream (const rav::id receiverId)
{
    for (auto& state : rxStreams_)
        if (state.getReceiverId() == receiverId)
            return &state;
    return nullptr;
}
