/*
 * Project: RAVENNAKIT demo application
 * Copyright (c) 2024-2025 Sound on Digital
 *
 * This file is part of the RAVENNAKIT demo application.
 *
 * The RAVENNAKIT demo application is licensed on a proprietary,
 * source-available basis. You are allowed to view, modify, and compile
 * this code for personal or internal evaluation only.
 *
 * You may NOT redistribute this file, any modified versions, or any
 * compiled binaries to third parties, and you may NOT use it for any
 * commercial purpose, except under a separate written agreement with
 * Sound on Digital.
 *
 * For the full license terms, see:
 *   LICENSE.md
 * or visit:
 *   https://ravennakit.com
 */

#pragma once

#include "ravennakit/core/audio/audio_buffer.hpp"
#include "ravennakit/core/audio/audio_buffer_view.hpp"
#include "ravennakit/ravenna/ravenna_node.hpp"
#include "util/AudioResampler.hpp"
#include "util/MessageThreadExecutor.hpp"

#include <juce_audio_devices/juce_audio_devices.h>

/**
 * High(er) level class connecting an audio device to a ravenna_node.
 */
class AudioReceiversModel : public rav::RavennaNode::Subscriber, public juce::AudioIODeviceCallback
{
public:
    struct StreamState
    {
        rav::rtp::AudioReceiver::StreamInfo stream;
        rav::rtp::AudioReceiver::StreamState state {};
    };

    struct ReceiverState
    {
        rav::RavennaReceiver::Configuration configuration;
        std::vector<StreamState> streams;
        rav::AudioFormat inputFormat;
        rav::AudioFormat outputFormat;
        uint32_t maxNumFramesPerBlock {};
    };

    class Subscriber
    {
    public:
        virtual ~Subscriber() = default;
        virtual void onAudioReceiverUpdated (const rav::Id receiverId, const ReceiverState* state)
        {
            std::ignore = receiverId;
            std::ignore = state;
        }

        virtual void onAudioReceiverStatsUpdated (
            const rav::Id receiverId,
            const size_t streamIndex,
            const rav::rtp::PacketStats::Counters& stats)
        {
            std::ignore = receiverId;
            std::ignore = streamIndex;
            std::ignore = stats;
        }
    };

    /**
     * Constructor.
     * @param node The ravenna_node to connect to.
     */
    explicit AudioReceiversModel (rav::RavennaNode& node);
    ~AudioReceiversModel() override;

    /**
     * Creates a receiver.
     * @param config The session name to create the receiver for.
     * @return A valid id of the newly created receiver, or an invalid id on failure.
     */
    [[nodiscard]] tl::expected<rav::Id, std::string> createReceiver (rav::RavennaReceiver::Configuration config) const;

    /**
     * Removes a receiver.
     * @param receiverId The receiver to remove.
     */
    void removeReceiver (rav::Id receiverId) const;

    /**
     * Updates the configuration of a sender.
     * @param senderId The id of the sender to update.
     * @param config The configuration changes to apply.
     */
    void updateReceiverConfiguration (rav::Id senderId, rav::RavennaReceiver::Configuration config) const;

    /**
     * Gets the packet statistics for a receiver.
     * @param receiverId The receiver to get the packet statistics for.
     * @return The packet statistics for the receiver, or an empty structure if the receiver doesn't exist.
     */
    [[nodiscard]] std::optional<std::string> getSdpTextForReceiver (rav::Id receiverId) const;

    /**
     * Adds a subscriber.
     * @param subscriber The subscriber to add.
     * @return true if the subscriber was added, or false if it was already in the list.
     */
    [[nodiscard]] bool subscribe (Subscriber* subscriber);

    /**
     * Removes a subscriber.
     * @param subscriber The subscriber to remove.
     * @return true if the subscriber was removed, or false if it was not in the list.
     */
    [[nodiscard]] bool unsubscribe (const Subscriber* subscriber);

    // rav::ravenna_node::subscriber overrides
    void ravenna_receiver_added (const rav::RavennaReceiver& receiver) override;
    void ravenna_receiver_removed (rav::Id receiverId) override;

    // public juce::AudioIODeviceCallback overrides
    void audioDeviceIOCallbackWithContext (
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    class Receiver : public rav::RavennaReceiver::Subscriber
    {
    public:
        struct RealtimeSharedState
        {
            rav::AudioFormat inputFormat;
            rav::AudioFormat outputFormat;
            uint32_t delayFrames {};
            std::unique_ptr<Resample, decltype (&resampleFree)> resampler { nullptr, &resampleFree };
            rav::AudioBuffer<float> resampleBuffer;
            std::optional<uint32_t> rtpTimestamp {};
        };

        explicit Receiver (AudioReceiversModel& owner, rav::Id receiverId);
        ~Receiver() override;

        void prepareInput (const rav::AudioFormat& format);
        void prepareOutput (const rav::AudioFormat& format, uint32_t maxNumFramesPerBlock);

        bool processBlock (rav::AudioBufferView<float>& outputBuffer, uint32_t rtpTimestamp, rav::ptp::Timestamp ptpTimestamp);

        // rav::rtp_stream_receiver::subscriber overrides
        void ravenna_receiver_parameters_updated (const rav::rtp::AudioReceiver::ReaderParameters& parameters) override;
        void ravenna_receiver_configuration_updated (
            const rav::RavennaReceiver& receiver,
            const rav::RavennaReceiver::Configuration& configuration) override;
        void ravenna_receiver_stream_state_updated (
            const rav::rtp::AudioReceiver::StreamInfo& stream_info,
            rav::rtp::AudioReceiver::StreamState state) override;
        void ravenna_receiver_stream_stats_updated (rav::Id receiver_id, size_t stream_index, const rav::rtp::PacketStats::Counters& stats)
            override;

        AudioReceiversModel& owner_;
        rav::Id receiverId_;
        ReceiverState state_;
        rav::RealtimeSharedObject<RealtimeSharedState> realtimeSharedState_;
        MessageThreadExecutor executor_;

        void updateRealtimeSharedState();
    };

    struct RealtimeSharedContext
    {
        std::vector<Receiver*> receivers;
    };

    rav::RavennaNode& node_;
    rav::ptp::Instance::Subscriber ptpSubscriber_;
    std::vector<std::unique_ptr<Receiver>> receivers_;
    rav::AudioFormat targetFormat_;
    uint32_t maxNumFramesPerBlock_ {};
    rav::SubscriberList<Subscriber> subscribers_;
    rav::RealtimeSharedObject<RealtimeSharedContext> realtimeSharedContext_;

    // Audio thread:
    std::optional<uint32_t> rtpTs_ {};
    rav::AudioBuffer<float> intermediateBuffer_ {};
    rav::AudioBuffer<float> resamplerBuffer_ {};
    std::unique_ptr<Resample, decltype (&resampleFree)> resampler_ { nullptr, &resampleFree };

    MessageThreadExecutor executor_; // Keep last so that it's destroyed first to prevent dangling pointers

    [[nodiscard]] Receiver* findReceiver (rav::Id receiverId) const;
    void updateRealtimeSharedContext();
};
