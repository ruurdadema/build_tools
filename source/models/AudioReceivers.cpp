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

AudioReceivers::AudioReceivers (rav::RavennaNode& node) : node_ (node)
{
    node_.subscribe (this).wait();
}

AudioReceivers::~AudioReceivers()
{
    node_.unsubscribe (this).wait();
}

rav::Id AudioReceivers::createReceiver (const std::string& sessionName) const
{
    return node_.create_receiver (sessionName).get();
}

void AudioReceivers::removeReceiver (const rav::Id receiverId) const
{
    node_.remove_receiver (receiverId).wait();
}

void AudioReceivers::setReceiverDelay (const rav::Id receiverId, const uint32_t delaySamples) const
{
    if (!node_.set_receiver_delay (receiverId, delaySamples).get())
    {
        RAV_ERROR ("Failed to set receiver delay");
    }
}

std::optional<std::string> AudioReceivers::getSdpTextForReceiver (const rav::Id receiverId) const
{
    return node_.get_sdp_text_for_receiver (receiverId).get();
}

rav::rtp::StreamReceiver::StreamStats AudioReceivers::getStatisticsForReceiver (const rav::Id receiverId) const
{
    return node_.get_stats_for_receiver (receiverId).get();
}

bool AudioReceivers::subscribe (Subscriber* subscriber)
{
    if (subscribers_.add (subscriber))
    {
        for (const auto& receiver : receivers_)
            subscriber->onAudioReceiverUpdated (receiver->getReceiverId(), &receiver->getState());
        return true;
    }
    return false;
}

bool AudioReceivers::unsubscribe (Subscriber* subscriber)
{
    return subscribers_.remove (subscriber);
}

void AudioReceivers::ravenna_receiver_added (const rav::RavennaReceiver& receiver)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);

    executor_.callAsync ([this, streamId = receiver.get_id(), sessionName = receiver.get_session_name()] {
        RAV_ASSERT (findReceiver (streamId) == nullptr, "Receiver already exists");
        const auto& it = receivers_.emplace_back (std::make_unique<Receiver> (*this, streamId, sessionName));
        if (targetFormat_.is_valid() && maxNumFramesPerBlock_ > 0)
            it->prepareOutput (targetFormat_, maxNumFramesPerBlock_);
        updateRealtimeSharedContext();
        for (const auto& subscriber : subscribers_)
            subscriber->onAudioReceiverUpdated (streamId, &it->getState());
    });
}

void AudioReceivers::ravenna_receiver_removed (const rav::Id receiverId)
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

    rav::AudioBufferView outputBuffer { outputChannelData,
                                          static_cast<uint32_t> (numOutputChannels),
                                          static_cast<uint32_t> (numSamples) };

    outputBuffer.clear();

    const auto intermediateBuffer = intermediateBuffer_.with_num_channels (static_cast<size_t> (numOutputChannels))
                                  .with_num_frames (static_cast<size_t> (numSamples));

    const auto lock = realtimeSharedContext_.lock_realtime();

    for (auto* receiver : lock->receivers)
    {
        receiver->processBlock (intermediateBuffer);
        outputBuffer.add (intermediateBuffer);
    }
}

void AudioReceivers::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    TRACY_ZONE_SCOPED;

    RAV_ASSERT (device != nullptr, "Device expected to be not null");

    targetFormat_ = rav::AudioFormat {
        rav::AudioFormat::ByteOrder::le,
        rav::AudioEncoding::pcm_f32,
        rav::AudioFormat::ChannelOrdering::noninterleaved,
        static_cast<uint32_t> (device->getCurrentSampleRate()),
        static_cast<uint32_t> (device->getActiveOutputChannels().countNumberOfSetBits()),
    };

    maxNumFramesPerBlock_ = static_cast<uint32_t> (device->getCurrentBufferSizeSamples());

    intermediateBuffer_.resize (
        static_cast<size_t> (device->getActiveOutputChannels().countNumberOfSetBits()),
        maxNumFramesPerBlock_);

    for (const auto& stream : receivers_)
        stream->prepareOutput (targetFormat_, maxNumFramesPerBlock_);
}

void AudioReceivers::audioDeviceStopped()
{
    TRACY_ZONE_SCOPED;
    RAV_TRACE ("Audio device stopped");
}

AudioReceivers::Receiver::Receiver (AudioReceivers& owner, const rav::Id receiverId, std::string sessionName) :
    owner_ (owner),
    receiverId_ (receiverId)
{
    state_.sessionName = std::move (sessionName);

    JUCE_ASSERT_MESSAGE_THREAD;
    owner_.node_.subscribe_to_receiver (receiverId_, this).wait();
}

AudioReceivers::Receiver::~Receiver()
{
    JUCE_ASSERT_MESSAGE_THREAD;
    owner_.node_.unsubscribe_from_receiver (receiverId_, this).wait();
}

rav::Id AudioReceivers::Receiver::getReceiverId() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return receiverId_;
}

const AudioReceivers::ReceiverState& AudioReceivers::Receiver::getState() const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return state_;
}

void AudioReceivers::Receiver::prepareInput (const rav::AudioFormat& format)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    RAV_ASSERT (format.is_valid(), "Invalid format");
    state_.inputFormat = format;
    updateRealtimeSharedState();
}

void AudioReceivers::Receiver::prepareOutput (const rav::AudioFormat& format, const uint32_t maxNumFramesPerBlock)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    RAV_ASSERT (format.is_valid(), "Invalid format");
    RAV_ASSERT (maxNumFramesPerBlock > 0, "Num samples must be > 0");
    state_.outputFormat = format;
    updateRealtimeSharedState();
}

void AudioReceivers::Receiver::processBlock (const rav::AudioBufferView<float>& outputBuffer)
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
        outputBuffer.clear(); // Receiver not (yet) available, make sure we don't output garbage
    }
}

void AudioReceivers::Receiver::rtp_stream_receiver_updated (const rav::rtp::StreamReceiver::StreamUpdatedEvent& event)
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

void AudioReceivers::Receiver::on_data_received ([[maybe_unused]] const rav::WrappingUint32 timestamp)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (owner_.node_);
}

void AudioReceivers::Receiver::on_data_ready ([[maybe_unused]] const rav::WrappingUint32 timestamp)
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

AudioReceivers::Receiver* AudioReceivers::findReceiver (const rav::Id receiverId) const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    for (const auto& receiver : receivers_)
        if (receiver->getReceiverId() == receiverId)
            return receiver.get();
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
