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
class AudioMixer : public rav::ravenna_node::subscriber, public juce::AudioIODeviceCallback
{
public:
    /**
     * Constructor.
     * @param node The ravenna_node to connect to.
     */
    explicit AudioMixer (rav::ravenna_node& node);
    ~AudioMixer() override;

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
    class RxStream : public rav::rtp_stream_receiver::data_callback, public rav::rtp_stream_receiver::subscriber
    {
    public:
        explicit RxStream (rav::ravenna_node& node, rav::id receiverId);
        ~RxStream() override;

        [[nodiscard]] rav::id getReceiverId() const;

        void prepareInput (const rav::audio_format& format);
        void prepareOutput (const rav::audio_format& format, uint32_t maxNumFramesPerBlock);

        void processBlock (const rav::audio_buffer_view<float>& outputBuffer);

        // rav::rtp_stream_receiver::subscriber overrides
        void stream_updated (const rav::rtp_stream_receiver::stream_updated_event& event) override;

        // rav::rtp_stream_receiver::data_callback overrides
        void on_data_received (rav::wrapping_uint32 timestamp) override;
        void on_data_ready (rav::wrapping_uint32 timestamp) override;

    private:
        rav::ravenna_node& node_;
        rav::id receiverId_;
        rav::audio_format inputFormat_;
        rav::audio_format outputFormat_;
        uint32_t maxNumFramesPerBlock_ {};
        std::vector<uint8_t> inputBuffer_;
        rav::audio_buffer<float> outputBuffer_;
        std::atomic<std::optional<rav::wrapping_uint32>> mostRecentDataReadyTimestamp_{};
        std::optional<rav::wrapping_uint32> timestamp_{};

        void allocateResources();
    };

    rav::ravenna_node& node_;
    std::vector<std::unique_ptr<RxStream>> rxStreams_;
    MessageThreadExecutor executor_;
    rav::audio_format targetFormat_;
    uint32_t maxNumFramesPerBlock_ {};
    std::mutex mutex_;

    [[nodiscard]] RxStream* findRxStream (rav::id receiverId) const;
};
