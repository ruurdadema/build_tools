/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#pragma once

#include "ravennakit/core/audio/audio_buffer.hpp"
#include "ravennakit/core/audio/audio_buffer_view.hpp"
#include "ravennakit/ravenna/ravenna_node.hpp"
#include "util/MessageThreadExecutor.hpp"

#include <juce_audio_devices/juce_audio_devices.h>

/**
 * High(er) level class connecting an audio device to a ravenna_node.
 */
class AudioReceivers : public rav::RavennaNode::Subscriber, public juce::AudioIODeviceCallback
{
public:
    struct StreamState
    {
        rav::rtp::AudioReceiver::Stream stream;
        rav::rtp::AudioReceiver::State state;
    };

    struct ReceiverState
    {
        rav::RavennaReceiver::Configuration configuration;
        std::vector<StreamState> streams;
        rav::AudioFormat inputFormat;
        rav::AudioFormat outputFormat;

        const AudioReceivers::StreamState* find_stream_for_rank (rav::Rank rank) const;
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
    };

    /**
     * Constructor.
     * @param node The ravenna_node to connect to.
     */
    explicit AudioReceivers (rav::RavennaNode& node);
    ~AudioReceivers() override;

    /**
     * Creates a receiver.
     * @param sessionName The session name to create the receiver for.
     * @return A valid id of the newly created receiver, or an invalid id on failure.
     */
    [[nodiscard]] rav::Id createReceiver (const std::string& sessionName) const;

    /**
     * Removes a receiver.
     * @param receiverId The receiver to remove.
     */
    void removeReceiver (rav::Id receiverId) const;

    /**
     * Updates the configuration of a sender.
     * @param senderId The id of the sender to update.
     * @param update The configuration changes to apply.
     */
    void updateReceiverConfiguration (rav::Id senderId, rav::RavennaReceiver::ConfigurationUpdate update) const;

    /**
     * Gets the packet statistics for a receiver.
     * @param receiverId The receiver to get the packet statistics for.
     * @return The packet statistics for the receiver, or an empty structure if the receiver doesn't exist.
     */
    [[nodiscard]] std::optional<std::string> getSdpTextForReceiver (rav::Id receiverId) const;

    /**
     * Gets the packet statistics for a receiver.
     * @param receiverId The receiver to get the packet statistics for.
     * @return The packet statistics for the receiver, or an empty structure if the receiver doesn't exist.
     */
    [[nodiscard]] rav::rtp::AudioReceiver::SessionStats getStatisticsForReceiver (rav::Id receiverId) const;

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
        explicit Receiver (AudioReceivers& owner, rav::Id receiverId);
        ~Receiver() override;

        [[nodiscard]] rav::Id getReceiverId() const;
        [[nodiscard]] const ReceiverState& getState() const;

        void prepareInput (const rav::AudioFormat& format);
        void prepareOutput (const rav::AudioFormat& format, uint32_t maxNumFramesPerBlock);

        std::optional<uint32_t> processBlock (
            const rav::AudioBufferView<float>& outputBuffer,
            std::optional<uint32_t> atTimestamp);

        // rav::rtp_stream_receiver::subscriber overrides
        void ravenna_receiver_streams_updated (const std::vector<rav::rtp::AudioReceiver::Stream>& streams) override;
        void ravenna_receiver_configuration_updated (
            rav::Id receiver_id,
            const rav::RavennaReceiver::Configuration& configuration) override;
        void ravenna_receiver_stream_state_updated (
            const rav::rtp::AudioReceiver::Stream& stream,
            rav::rtp::AudioReceiver::State state) override;
        void on_data_received (rav::WrappingUint32 timestamp) override;
        void on_data_ready (rav::WrappingUint32 timestamp) override;

    private:
        AudioReceivers& owner_;
        rav::Id receiverId_;
        ReceiverState state_;
        rav::RealtimeSharedObject<ReceiverState> realtimeSharedState_;
        MessageThreadExecutor executor_;

        void updateRealtimeSharedState();
    };

    struct RealtimeSharedContext
    {
        std::vector<Receiver*> receivers;
    };

    rav::RavennaNode& node_;
    std::vector<std::unique_ptr<Receiver>> receivers_;
    rav::AudioFormat targetFormat_;
    uint32_t maxNumFramesPerBlock_ {};
    rav::AudioBuffer<float> intermediateBuffer_ {};
    rav::SubscriberList<Subscriber> subscribers_;
    rav::RealtimeSharedObject<RealtimeSharedContext> realtimeSharedContext_;
    MessageThreadExecutor executor_; // Keep last so that it's destroyed first to prevent dangling pointers

    [[nodiscard]] Receiver* findReceiver (rav::Id receiverId) const;
    void updateRealtimeSharedContext();
};
