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
#include "ravennakit/core/util/todo.hpp"

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
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);
    RAV_ASSERT (findRxStream (receiver.get_id()) == nullptr, "Receiver already exists");

    std::lock_guard lock (mutex_);

    const auto& it = rxStreams_.emplace_back (std::make_unique<RxStream> (node_, receiver.get_id()));
    if (targetFormat_.is_valid() && maxNumFramesPerBlock_ > 0)
        it->prepareOutput (targetFormat_, maxNumFramesPerBlock_);
}

void AudioMixer::ravenna_receiver_removed (const rav::id receiverId)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);

    std::lock_guard lock (mutex_);

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
    std::lock_guard lock (mutex_);

    RAV_ASSERT (numInputChannels >= 0, "Num input channels must be >= 0");
    RAV_ASSERT (numOutputChannels >= 0, "Num output channels must be >= 0");
    RAV_ASSERT (numSamples >= 0, "Num samples must be >= 0");

    rav::audio_buffer_view outputBuffer { outputChannelData,
                                          static_cast<uint32_t> (numOutputChannels),
                                          static_cast<uint32_t> (numSamples) };

    outputBuffer.clear();

    for (const auto& stream : rxStreams_)
        stream->processBlock (outputBuffer);
}

void AudioMixer::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    auto work = [this, device] {
        targetFormat_ = rav::audio_format {
            rav::audio_format::byte_order::le,
            rav::audio_encoding::pcm_f32,
            rav::audio_format::channel_ordering::noninterleaved,
            static_cast<uint32_t> (device->getCurrentSampleRate()),
            static_cast<uint32_t> (device->getActiveOutputChannels().countNumberOfSetBits()),
        };

        maxNumFramesPerBlock_ = static_cast<uint32_t> (device->getCurrentBufferSizeSamples());

        std::lock_guard lock (mutex_);

        for (const auto& stream : rxStreams_)
            stream->prepareOutput (targetFormat_, maxNumFramesPerBlock_);
    };
    node_.dispatch (asio::use_future (work)).wait();
}

void AudioMixer::audioDeviceStopped()
{
    RAV_TRACE ("Audio device stopped");
}

AudioMixer::RxStream::RxStream (rav::ravenna_node& node, const rav::id receiverId) :
    node_ (node),
    receiverId_ (receiverId)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);

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
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);

    return receiverId_;
}

void AudioMixer::RxStream::prepareInput (const rav::audio_format& format)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);
    RAV_ASSERT (format.is_valid(), "Invalid format");

    inputFormat_ = format;
    allocateResources();
}

void AudioMixer::RxStream::prepareOutput (const rav::audio_format& format, const uint32_t maxNumFramesPerBlock)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);
    RAV_ASSERT (maxNumFramesPerBlock > 0, "Num samples must be > 0");
    RAV_ASSERT (format.is_valid(), "Invalid format");

    outputFormat_ = format;
    maxNumFramesPerBlock_ = maxNumFramesPerBlock;
    allocateResources();
}

void AudioMixer::RxStream::processBlock (const rav::audio_buffer_view<float>& outputBuffer)
{
    if (inputFormat_.sample_rate != outputFormat_.sample_rate)
    {
        RAV_WARNING ("Sample rate mismatch");
        return;
    }

    if (!timestamp_.has_value())
    {
        const auto mostRecentTimestamp = mostRecentDataReadyTimestamp_.load();
        if (!mostRecentTimestamp.has_value())
        {
            return; // No timestamp available
        }
        timestamp_ = mostRecentTimestamp;
    }

    RAV_ASSERT (timestamp_.has_value(), "Timestamp must be valid at this point");

    node_.realtime_read_audio_data (receiverId_, timestamp_->value(), outputBuffer);

    timestamp_ = timestamp_->operator+ (static_cast<uint32_t>(outputBuffer.num_frames()));
}

void AudioMixer::RxStream::stream_updated (const rav::rtp_stream_receiver::stream_updated_event& event)
{
    // TODO: Synchronize with processBlock()

    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);

    if (event.selected_audio_format.is_valid())
        prepareInput (event.selected_audio_format);
}

void AudioMixer::RxStream::on_data_received ([[maybe_unused]] const rav::wrapping_uint32 timestamp)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);
}

void AudioMixer::RxStream::on_data_ready (const rav::wrapping_uint32 timestamp)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);
    mostRecentDataReadyTimestamp_ = timestamp;
}

void AudioMixer::RxStream::allocateResources()
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);
    inputBuffer_.resize (inputFormat_.bytes_per_frame() * maxNumFramesPerBlock_);
    outputBuffer_.resize (outputFormat_.num_channels, maxNumFramesPerBlock_);
}

AudioMixer::RxStream* AudioMixer::findRxStream (const rav::id receiverId) const
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);
    for (const auto& stream : rxStreams_)
        if (stream->getReceiverId() == receiverId)
            return stream.get();
    return nullptr;
}
