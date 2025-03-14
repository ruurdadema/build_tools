/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "AudioReceivers.hpp"

#include "ravennakit/core/audio/audio_buffer_view.hpp"
#include "ravennakit/core/audio/audio_data.hpp"
#include "ravennakit/core/util/todo.hpp"

AudioReceivers::AudioReceivers (rav::ravenna_node& node) : node_ (node)
{
    node_.add_subscriber (this).wait();
}

AudioReceivers::~AudioReceivers()
{
    node_.remove_subscriber (this).wait();
}

rav::id AudioReceivers::createReceiver (const std::string& sessionName) const
{
    return node_.create_receiver (sessionName).get();
}

void AudioReceivers::removeReceiver (const rav::id receiverId) const
{
    node_.remove_receiver (receiverId).wait();
}

void AudioReceivers::setReceiverDelay (const rav::id receiverId, const uint32_t delaySamples) const
{
    if (!node_.set_receiver_delay (receiverId, delaySamples).get())
    {
        RAV_ERROR ("Failed to set receiver delay");
    }
}

std::optional<std::string> AudioReceivers::getSdpTextForReceiver (const rav::id receiverId) const
{
    return node_.get_sdp_text_for_receiver (receiverId).get();
}

rav::rtp_stream_receiver::stream_stats AudioReceivers::getStatisticsForReceiver (const rav::id receiverId) const
{
    return node_.get_stats_for_receiver (receiverId).get();
}

bool AudioReceivers::addSubscriber (Subscriber* subscriber)
{
    if (subscribers_.add (subscriber))
    {
        for (const auto& stream : receivers_)
            subscriber->onAudioReceiverUpdated (stream->getReceiverId(), &stream->getState());
        return true;
    }
    return false;
}

bool AudioReceivers::removeSubscriber (Subscriber* subscriber)
{
    return subscribers_.remove (subscriber);
}

void AudioReceivers::ravenna_receiver_added (const rav::ravenna_receiver& receiver)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);

    executor_.callAsync ([this, streamId = receiver.get_id(), sessionName = receiver.get_session_name()] {
        RAV_ASSERT (findRxStream (streamId) == nullptr, "Receiver already exists");
        const auto& it = receivers_.emplace_back (std::make_unique<Receiver> (*this, streamId, sessionName));
        if (targetFormat_.is_valid() && maxNumFramesPerBlock_ > 0)
            it->prepareOutput (targetFormat_, maxNumFramesPerBlock_);
        updateRealtimeSharedContext();
        for (const auto& subscriber : subscribers_)
            subscriber->onAudioReceiverUpdated (streamId, &it->getState());
    });
}

void AudioReceivers::ravenna_receiver_removed (const rav::id receiverId)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);

    executor_.callAsync ([this, receiverId] {
        // No need to unsubscribe from the receiver (via ravenna_node), as the stream no longer exists at this point.
        for (auto it = receivers_.begin(); it != receivers_.end(); ++it)
        {
            if ((*it)->getReceiverId() == receiverId)
            {
                std::unique_ptr<Receiver> tmp = std::move (*it); // Keep alive until after the context is updated
                receivers_.erase (it);
                updateRealtimeSharedContext();
                break;
            }
        }
        for (const auto& subscriber : subscribers_)
            subscriber->onAudioReceiverUpdated (receiverId, nullptr);
    });
}

void AudioReceivers::audioDeviceIOCallbackWithContext (
    [[maybe_unused]] const float* const* inputChannelData,
    [[maybe_unused]] const int numInputChannels,
    float* const* outputChannelData,
    const int numOutputChannels,
    const int numSamples,
    [[maybe_unused]] const juce::AudioIODeviceCallbackContext& context)
{
    TRACY_ZONE_SCOPED;

    // TODO: Synchronize with main thread

    RAV_ASSERT (numInputChannels >= 0, "Num input channels must be >= 0");
    RAV_ASSERT (numOutputChannels >= 0, "Num output channels must be >= 0");
    RAV_ASSERT (numSamples >= 0, "Num samples must be >= 0");

    rav::audio_buffer_view outputBuffer { outputChannelData,
                                          static_cast<uint32_t> (numOutputChannels),
                                          static_cast<uint32_t> (numSamples) };

    outputBuffer.clear();

    const auto lock = realtimeSharedContext_.lock_realtime();

    for (auto* receiver : lock->receivers)
        receiver->processBlock (outputBuffer);
}

void AudioReceivers::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    TRACY_ZONE_SCOPED;

    targetFormat_ = rav::audio_format {
        rav::audio_format::byte_order::le,
        rav::audio_encoding::pcm_f32,
        rav::audio_format::channel_ordering::noninterleaved,
        static_cast<uint32_t> (device->getCurrentSampleRate()),
        static_cast<uint32_t> (device->getActiveOutputChannels().countNumberOfSetBits()),
    };

    maxNumFramesPerBlock_ = static_cast<uint32_t> (device->getCurrentBufferSizeSamples());

    for (const auto& stream : receivers_)
        stream->prepareOutput (targetFormat_, maxNumFramesPerBlock_);
}

void AudioReceivers::audioDeviceStopped()
{
    TRACY_ZONE_SCOPED;
    RAV_TRACE ("Audio device stopped");
}

AudioReceivers::Receiver::Receiver (AudioReceivers& owner, const rav::id receiverId, std::string sessionName) :
    owner_ (owner),
    receiverId_ (receiverId)
{
    state_.sessionName = std::move (sessionName);

    JUCE_ASSERT_MESSAGE_THREAD;
    owner_.node_.add_receiver_subscriber (receiverId_, this).wait();
}

AudioReceivers::Receiver::~Receiver()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    owner_.node_.remove_receiver_subscriber (receiverId_, this).wait();
}

rav::id AudioReceivers::Receiver::getReceiverId() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return receiverId_;
}

const AudioReceivers::ReceiverState& AudioReceivers::Receiver::getState() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return state_;
}

void AudioReceivers::Receiver::prepareInput (const rav::audio_format& format)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    RAV_ASSERT (format.is_valid(), "Invalid format");
    state_.inputFormat = format;
    updateRealtimeSharedState();
}

void AudioReceivers::Receiver::prepareOutput (const rav::audio_format& format, const uint32_t maxNumFramesPerBlock)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    RAV_ASSERT (format.is_valid(), "Invalid format");
    RAV_ASSERT (maxNumFramesPerBlock > 0, "Num samples must be > 0");
    state_.outputFormat = format;
    updateRealtimeSharedState();
}

void AudioReceivers::Receiver::processBlock (const rav::audio_buffer_view<float>& outputBuffer)
{
    TRACY_ZONE_SCOPED;

    const auto state = realtimeSharedState_.lock_realtime();

    if (!state->inputFormat.is_valid())
        return;

    if (state->inputFormat.sample_rate != state->outputFormat.sample_rate)
    {
        // Sample rate mismatch, can't process
        return;
    }

    if (!owner_.node_.read_audio_data_realtime (receiverId_, outputBuffer, {}).has_value())
    {
        RAV_WARNING ("Failed to read data");
    }
}

void AudioReceivers::Receiver::rtp_stream_receiver_updated (const rav::rtp_stream_receiver::stream_updated_event& event)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (owner_.node_);

    executor_.callAsync ([this, event] {
        state_.delaySamples = event.delay_samples;
        state_.packetTimeFrames = event.packet_time_frames;
        state_.state = event.state;
        state_.session = event.session;
        if (event.selected_audio_format.is_valid() && state_.inputFormat != event.selected_audio_format)
            prepareInput (event.selected_audio_format);
        for (auto* subscriber : owner_.subscribers_)
            subscriber->onAudioReceiverUpdated (receiverId_, &state_);
    });
}

void AudioReceivers::Receiver::on_data_received ([[maybe_unused]] const rav::wrapping_uint32 timestamp)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (owner_.node_);
}

void AudioReceivers::Receiver::on_data_ready ([[maybe_unused]] const rav::wrapping_uint32 timestamp)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (owner_.node_);
}

void AudioReceivers::Receiver::updateRealtimeSharedState()
{
    auto newState = std::make_unique<ReceiverState> (state_);
    if (!realtimeSharedState_.update (std::move (newState)))
    {
        RAV_ERROR ("Failed to update realtime shared state");
    }
}

AudioReceivers::Receiver* AudioReceivers::findRxStream (const rav::id receiverId) const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    for (const auto& stream : receivers_)
        if (stream->getReceiverId() == receiverId)
            return stream.get();
    return nullptr;
}

void AudioReceivers::updateRealtimeSharedContext()
{
    auto newContext = std::make_unique<RealtimeSharedContext>();
    for (const auto& stream : receivers_)
        newContext->receivers.push_back (stream.get());
    if (!realtimeSharedContext_.update (std::move (newContext)))
    {
        RAV_ERROR ("Failed to update realtime shared context");
    }
}
