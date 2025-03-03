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

#include "ravennakit/core/audio/audio_format_converter.hpp"
#include "ravennakit/ravenna/ravenna_node.hpp"
#include "ravennakit/rtp/rtp_stream_receiver.hpp"
#include "util/MessageThreadExecutor.hpp"

#include <juce_audio_devices/juce_audio_devices.h>

/**
 * This is a high level class connecting an audio device to a ravenna_node.
 */
class AudioMixer :
    public rav::ravenna_node::subscriber,
    public rav::rtp_stream_receiver::subscriber,
    public rav::rtp_stream_receiver::data_callback,
    public juce::AudioIODeviceCallback
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

    // rav::rtp_stream_receiver::subscriber overrides
    void stream_updated (const rav::rtp_stream_receiver::stream_updated_event& event) override;

    // rav::rtp_stream_receiver::data_callback overrides
    void on_data_received (rav::wrapping_uint32 timestamp) override;
    void on_data_ready (rav::wrapping_uint32 timestamp) override;

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
    class RxStream
    {
    public:
        explicit RxStream (const rav::id receiverId) : receiverId_ (receiverId) {}

        [[nodiscard]] rav::id getReceiverId() const;

        void prepareInput (rav::audio_format format);
        void prepareOutput (rav::audio_format format, int maxNumFramesPerBlock);

    private:
        rav::id receiverId_;
        rav::audio_format_converter formatConverter_;
        std::vector<uint8_t> inputBuffer_;
        uint32_t numFramesPerBlock_ {};

        void allocateResources();
    };

    rav::ravenna_node& node_;
    std::vector<RxStream> rxStreams_;
    MessageThreadExecutor executor_;

    RxStream* findRxStream (rav::id receiverId);
};
