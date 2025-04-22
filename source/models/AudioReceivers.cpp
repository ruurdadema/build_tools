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

const AudioReceivers::StreamState* AudioReceivers::ReceiverState::find_stream_for_rank (const rav::Rank rank) const
{
    for (auto& stream : streams)
    {
        if (stream.stream.rank == rank)
            return &stream;
    }
    return nullptr;
}

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
    rav::RavennaReceiver::ConfigurationUpdate config;
    config.session_name = sessionName;
    config.enabled = true;
    return node_.create_receiver (config).get();
}

void AudioReceivers::removeReceiver (const rav::Id receiverId) const
{
    node_.remove_receiver (receiverId).wait();
}

void AudioReceivers::updateReceiverConfiguration (
    const rav::Id senderId,
    rav::RavennaReceiver::ConfigurationUpdate update) const
{
    auto result = node_.update_receiver_configuration (senderId, std::move (update)).get();
    if (!result)
    {
        RAV_ERROR ("Failed to update receiver configuration: {}", result.error());
    }
}

std::optional<std::string> AudioReceivers::getSdpTextForReceiver (const rav::Id receiverId) const
{
    return node_.get_sdp_text_for_receiver (receiverId).get();
}

rav::rtp::AudioReceiver::SessionStats AudioReceivers::getStatisticsForReceiver (const rav::Id receiverId) const
{
    return node_.get_stats_for_receiver (receiverId).get();
}

bool AudioReceivers::subscribe (Subscriber* subscriber)
{
    if (subscribers_.add (subscriber))
    {
        for (const auto& receiver : receivers_)
        {
            subscriber->onAudioReceiverUpdated (receiver->getReceiverId(), &receiver->getState());
        }
        return true;
    }
    return false;
}

bool AudioReceivers::unsubscribe (const Subscriber* subscriber)
{
    return subscribers_.remove (subscriber);
}

void AudioReceivers::ravenna_receiver_added (const rav::RavennaReceiver& receiver)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);

    executor_.callAsync ([this, streamId = receiver.get_id()] {
        RAV_ASSERT (findReceiver (streamId) == nullptr, "Receiver already exists");
        const auto& it = receivers_.emplace_back (std::make_unique<Receiver> (*this, streamId));
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

    std::optional<uint32_t> atTimestamp;
    for (auto* receiver : lock->receivers)
    {
        atTimestamp = receiver->processBlock (intermediateBuffer, atTimestamp);
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

    RAV_TRACE ("Audio device about to start");
}

void AudioReceivers::audioDeviceStopped()
{
    TRACY_ZONE_SCOPED;
    RAV_TRACE ("Audio device stopped");
}

AudioReceivers::Receiver::Receiver (AudioReceivers& owner, const rav::Id receiverId) :
    owner_ (owner),
    receiverId_ (receiverId)
{
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

std::optional<uint32_t> AudioReceivers::Receiver::processBlock (
    const rav::AudioBufferView<float>& outputBuffer,
    const std::optional<uint32_t> atTimestamp)
{
    TRACY_ZONE_SCOPED;

    outputBuffer.clear();

    const auto state = realtimeSharedState_.lock_realtime();

    if (!state->inputFormat.is_valid())
        return std::nullopt;

    if (state->inputFormat.sample_rate != state->outputFormat.sample_rate)
    {
        // Sample rate mismatch, can't process
        return std::nullopt;
    }

    return owner_.node_.read_audio_data_realtime (receiverId_, outputBuffer, atTimestamp);
}

void AudioReceivers::Receiver::ravenna_receiver_parameters_updated (const rav::rtp::AudioReceiver::Parameters& parameters)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (owner_.node_);

    executor_.callAsync ([this, parameters] {
        state_.streams.clear();

        for (const auto& stream : parameters.streams)
            state_.streams.push_back (StreamState { stream, {} });

        if (parameters.audio_format.is_valid() && state_.inputFormat != parameters.audio_format)
            prepareInput (parameters.audio_format);

        for (auto* subscriber : owner_.subscribers_)
            subscriber->onAudioReceiverUpdated (receiverId_, &state_);
    });
}

void AudioReceivers::Receiver::ravenna_receiver_configuration_updated (
    rav::Id receiver_id,
    const rav::RavennaReceiver::Configuration& configuration)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (owner_.node_);

    executor_.callAsync ([this, receiver_id, configuration] {
        RAV_ASSERT (receiver_id == receiverId_, "Receiver ID mismatch");
        state_.configuration = configuration;
        for (auto* subscriber : owner_.subscribers_)
            subscriber->onAudioReceiverUpdated (receiver_id, &state_);
    });
}

void AudioReceivers::Receiver::ravenna_receiver_stream_state_updated (
    const rav::rtp::AudioReceiver::Stream& stream,
    const rav::rtp::AudioReceiver::State state)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (owner_.node_);

    executor_.callAsync ([this, stream, state] {
        bool changed = false;
        for (auto& streamState : state_.streams)
        {
            if (streamState.stream.session == stream.session)
            {
                streamState.state = state;
                changed = true;
            }
        }

        if (!changed)
            return;

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
