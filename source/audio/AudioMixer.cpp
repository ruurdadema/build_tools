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
#include "ravennakit/core/audio/audio_data.hpp"

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

    const auto& it = rxStreams_.emplace_back (std::make_unique<RxStream> (node_, receiver.get_id()));
    if (targetFormat_.is_valid() && maxNumFramesPerBlock_ > 0)
        it->prepareOutput (targetFormat_, maxNumFramesPerBlock_);
}

void AudioMixer::ravenna_receiver_removed (const rav::id receiverId)
{
    // No need to unsubscribe from the receiver (via ravenna_node), as the stream no longer exists at this point.

    for (auto it = rxStreams_.begin(); it != rxStreams_.end(); ++it)
    {
        if ((*it)->getReceiverId() == receiverId)
        {
            rxStreams_.erase (it);
            break;
        }
    }
}

void AudioMixer::audioDeviceIOCallbackWithContext (
    [[maybe_unused]] const float* const* inputChannelData,
    [[maybe_unused]] const int numInputChannels,
    float* const* outputChannelData,
    const int numOutputChannels,
    const int numSamples,
    [[maybe_unused]] const juce::AudioIODeviceCallbackContext& context)
{
    RAV_ASSERT (numInputChannels >= 0, "Num input channels must be >= 0");
    RAV_ASSERT (numOutputChannels >= 0, "Num output channels must be >= 0");
    RAV_ASSERT (numSamples >= 0, "Num samples must be >= 0");

    rav::audio_buffer_view outputBuffer { outputChannelData,
                                          static_cast<size_t> (numOutputChannels),
                                          static_cast<size_t> (numSamples) };

    outputBuffer.clear();

    for (const auto& stream : rxStreams_)
        stream->processBlock (outputBuffer);
}

void AudioMixer::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    targetFormat_ = rav::audio_format {
        rav::audio_format::byte_order::le,
        rav::audio_encoding::pcm_f32,
        rav::audio_format::channel_ordering::noninterleaved,
        static_cast<uint32_t> (device->getCurrentSampleRate()),
        static_cast<uint32_t> (device->getActiveOutputChannels().countNumberOfSetBits()),
    };

    maxNumFramesPerBlock_ = static_cast<uint32_t> (device->getCurrentBufferSizeSamples());
    for (const auto& stream : rxStreams_)
    {
        stream->prepareOutput (targetFormat_, maxNumFramesPerBlock_);
    }
}

void AudioMixer::audioDeviceStopped() {}
AudioMixer::RxStream::RxStream (rav::ravenna_node& node, const rav::id receiverId) :
    node_ (node),
    receiverId_ (receiverId)
{
    node_.add_receiver_subscriber (receiverId_, this).wait();
    node_.add_receiver_data_callback (receiverId_, this).wait();
}

AudioMixer::RxStream::~RxStream()
{
    node_.remove_receiver_data_callback (receiverId_, this).wait();
    node_.remove_receiver_subscriber (receiverId_, this).wait();
}

rav::id AudioMixer::RxStream::getReceiverId() const
{
    return receiverId_;
}

void AudioMixer::RxStream::prepareInput (const rav::audio_format& format)
{
    RAV_ASSERT (format.is_valid(), "Invalid format");

    inputFormat_ = format;
    allocateResources();
}

void AudioMixer::RxStream::prepareOutput (const rav::audio_format& format, const uint32_t maxNumFramesPerBlock)
{
    RAV_ASSERT (maxNumFramesPerBlock > 0, "Num samples must be > 0");
    RAV_ASSERT (format.is_valid(), "Invalid format");

    outputFormat_ = format;
    maxNumFramesPerBlock_ = maxNumFramesPerBlock;
    allocateResources();
}

void AudioMixer::RxStream::processBlock (rav::audio_buffer_view<float> outputBuffer)
{
    auto timestamp = timestamp_;

    if (inputFormat_.sample_rate != outputFormat_.sample_rate)
    {
        RAV_WARNING("Sample rate mismatch");
        return;
    }

    if (!timestamp.has_value())
    {
        const auto mostRecentTimestamp = mostRecentTimestamp_.load();
        if (!mostRecentTimestamp.has_value())
        {
            return; // No timestamp available
        }
        timestamp = mostRecentTimestamp;
    }

    RAV_ASSERT (timestamp.has_value(), "Timestamp must be valid at this point");

    if (inputFormat_.byte_order != rav::audio_format::byte_order::be)
    {
        RAV_ERROR ("Unexpected byte order");
        return;
    }

    if (inputFormat_.ordering != rav::audio_format::channel_ordering::interleaved)
    {
        RAV_ERROR ("Unexpected channel ordering");
        return;
    }

    if (inputFormat_.num_channels != outputBuffer.num_channels())
    {
        RAV_ERROR ("Channel mismatch");
        return;
    }

    // TODO: Read data, convert and sum into output buffer.
    node_.realtime_read_data (receiverId_, timestamp->value(), inputBuffer_.data(), inputBuffer_.size());

    if (inputFormat_.encoding == rav::audio_encoding::pcm_s16)
    {
        const auto result = rav::audio_data::convert<
            int16_t,
            rav::audio_data::byte_order::be,
            rav::audio_data::interleaving::interleaved,
            float,
            rav::audio_data::byte_order::ne> (
            reinterpret_cast<int16_t*> (inputBuffer_.data()),
            maxNumFramesPerBlock_,
            outputBuffer_.num_channels(),
            outputBuffer_.data());
        if (!result)
        {
            RAV_WARNING ("Failed to convert audio data");
        }
    }
    else if (inputFormat_.encoding == rav::audio_encoding::pcm_s24)
    {
        const auto result = rav::audio_data::convert<
            rav::int24_t,
            rav::audio_data::byte_order::be,
            rav::audio_data::interleaving::interleaved,
            float,
            rav::audio_data::byte_order::ne> (
            reinterpret_cast<rav::int24_t*> (inputBuffer_.data()),
            maxNumFramesPerBlock_,
            outputBuffer_.num_channels(),
            outputBuffer_.data());
        if (!result)
        {
            RAV_WARNING ("Failed to convert audio data");
        }
    }
    else
    {
        RAV_ERROR ("Unsupported encoding");
        return;
    }

    outputBuffer.copy_from (0, maxNumFramesPerBlock_, outputBuffer_.data(), outputBuffer_.num_channels());

    timestamp_ = timestamp->operator+ (maxNumFramesPerBlock_);
}

void AudioMixer::RxStream::stream_updated (const rav::rtp_stream_receiver::stream_updated_event& event)
{
    if (event.selected_audio_format.is_valid())
    {
        executor_.callAsync ([this, format = event.selected_audio_format] {
            prepareInput (format);
        });
    }
}

void AudioMixer::RxStream::on_data_received ([[maybe_unused]] rav::wrapping_uint32 timestamp) {}

void AudioMixer::RxStream::on_data_ready (const rav::wrapping_uint32 timestamp)
{
    mostRecentTimestamp_ = timestamp;
}

void AudioMixer::RxStream::allocateResources()
{
    inputBuffer_.resize (inputFormat_.bytes_per_frame() * maxNumFramesPerBlock_);
    outputBuffer_.resize (outputFormat_.num_channels, maxNumFramesPerBlock_);
}

AudioMixer::RxStream* AudioMixer::findRxStream (const rav::id receiverId) const
{
    for (const auto& stream : rxStreams_)
        if (stream->getReceiverId() == receiverId)
            return stream.get();
    return nullptr;
}
