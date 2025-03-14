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
#include "ravennakit/rtp/rtp_stream_receiver.hpp"
#include "util/MessageThreadExecutor.hpp"

#include <juce_audio_devices/juce_audio_devices.h>

/**
 * This is a high level class connecting an audio device to a ravenna_node.
 */
class AudioReceivers : public rav::ravenna_node::subscriber, public juce::AudioIODeviceCallback
{
public:
    struct ReceiverState
    {
        std::string sessionName;
        rav::audio_format inputFormat;
        rav::audio_format outputFormat;
        rav::rtp_session session;
        uint16_t packetTimeFrames = 0;
        uint32_t delaySamples = 0;
        rav::rtp_stream_receiver::receiver_state state {};
    };

    class Subscriber
    {
    public:
        virtual ~Subscriber() = default;
        virtual void onAudioReceiverUpdated (
            [[maybe_unused]] rav::id streamId,
            [[maybe_unused]] const ReceiverState* state)
        {
        }
    };

    /**
     * Constructor.
     * @param node The ravenna_node to connect to.
     */
    explicit AudioReceivers (rav::ravenna_node& node);
    ~AudioReceivers() override;

    /**
     * Creates a receiver.
     * @param sessionName The session name to create the receiver for.
     * @return true if the receiver was created, or false if it already exists.
     */
    [[nodiscard]] rav::id createReceiver (const std::string& sessionName) const;

    /**
     * Removes a receiver.
     * @param receiverId The receiver to remove.
     */
    void removeReceiver (rav::id receiverId) const;

    /**
     * Sets the delay for a receiver.
     * @param receiverId The receiver to set the delay for.
     * @param delaySamples The delay in samples.
     */
    void setReceiverDelay (rav::id receiverId, uint32_t delaySamples) const;

    /**
     * Gets the packet statistics for a receiver.
     * @param receiverId The receiver to get the packet statistics for.
     * @return The packet statistics for the receiver, or an empty structure if the receiver doesn't exist.
     */
    [[nodiscard]] std::optional<std::string> getSdpTextForReceiver (rav::id receiverId) const;

    /**
     * Gets the packet statistics for a receiver.
     * @param receiverId The receiver to get the packet statistics for.
     * @return The packet statistics for the receiver, or an empty structure if the receiver doesn't exist.
     */
    [[nodiscard]] rav::rtp_stream_receiver::stream_stats getStatisticsForReceiver (rav::id receiverId) const;

    /**
     * Adds a subscriber to the audio mixer.
     * @param subscriber The subscriber to add.
     * @return true if the subscriber was added, or false if it was already in the list.
     */
    [[nodiscard]] bool addSubscriber (Subscriber* subscriber);

    /**
     * Removes a subscriber from the audio mixer.
     * @param subscriber The subscriber to remove.
     * @return true if the subscriber was removed, or false if it was not in the list.
     */
    [[nodiscard]] bool removeSubscriber (Subscriber* subscriber);

    // rav::ravenna_node::subscriber overrides
    void ravenna_receiver_added (const rav::ravenna_receiver& receiver) override;
    void ravenna_receiver_removed (rav::id receiverId) override;

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
    class Receiver : public rav::rtp_stream_receiver::subscriber
    {
    public:
        explicit Receiver (AudioReceivers& owner, rav::id receiverId, std::string sessionName);
        ~Receiver() override;

        [[nodiscard]] rav::id getReceiverId() const;
        [[nodiscard]] const ReceiverState& getState() const;

        void prepareInput (const rav::audio_format& format);
        void prepareOutput (const rav::audio_format& format, uint32_t maxNumFramesPerBlock);

        void processBlock (const rav::audio_buffer_view<float>& outputBuffer) const;

        // rav::rtp_stream_receiver::subscriber overrides
        void rtp_stream_receiver_updated (const rav::rtp_stream_receiver::stream_updated_event& event) override;

        // rav::rtp_stream_receiver::data_callback overrides
        void on_data_received (rav::wrapping_uint32 timestamp) override;
        void on_data_ready (rav::wrapping_uint32 timestamp) override;

    private:
        AudioReceivers& owner_;
        rav::id receiverId_;
        ReceiverState state_;
        MessageThreadExecutor executor_;
    };

    rav::ravenna_node& node_;
    std::vector<std::unique_ptr<Receiver>> receivers_;
    MessageThreadExecutor executor_;
    rav::audio_format targetFormat_;
    uint32_t maxNumFramesPerBlock_ {};
    rav::subscriber_list<Subscriber> subscribers_;

    [[nodiscard]] Receiver* findRxStream (rav::id receiverId) const;
};
