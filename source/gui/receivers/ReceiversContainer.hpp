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

#include "application/ApplicationContext.hpp"
#include "models/AudioReceivers.hpp"
#include "util/MessageThreadExecutor.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class ReceiversContainer : public juce::Component, public AudioReceivers::Subscriber
{
public:
    explicit ReceiversContainer (ApplicationContext& context);
    ~ReceiversContainer() override;

    void resized() override;

    void resizeToFitContent();

    void onAudioReceiverUpdated (rav::Id receiverId, const AudioReceivers::ReceiverState* state) override;

private:
    static constexpr int kRowHeight = 138;
    static constexpr int kMargin = 10;

    class SdpViewer : public juce::Component
    {
    public:
        explicit SdpViewer (const std::string& sdpText);
        ~SdpViewer() override;
        void resized() override;
        void paint (juce::Graphics& g) override;

    private:
        juce::String sdpText_;
        juce::TextButton closeButton_ { "Close" };
        juce::TextButton copyButton_ { "Copy" };
    };

    class Row : public Component, public juce::Timer
    {
    public:
        explicit Row (AudioReceivers& audioReceivers, rav::Id receiverId);

        rav::Id getId() const;

        void update (const AudioReceivers::ReceiverState& state);
        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        AudioReceivers& audioReceivers_;
        juce::TextEditor delayEditor_;
        rav::Id receiverId_;
        uint32_t delay_ { 0 };
        juce::TextButton showSdpButton_ { "" };
        juce::TextButton onOffButton_ { "Start" };
        juce::TextButton deleteButton_ { "" };

        struct
        {
            juce::String sessionName;
            juce::String audioFormat { "..." };
            juce::String packetTimeFrames { "..." };
            juce::String address { "..." };
            juce::String state { "..." };
            juce::String warning {};
        } stream_;

        struct
        {
            juce::String dropped;
            juce::String duplicates;
            juce::String outOfOrder;
            juce::String tooLate;
        } packet_stats_;

        struct
        {
            juce::String avg;
            juce::String median;
            juce::String min;
            juce::String max;
            juce::String stddev;
        } interval_stats_;

        MessageThreadExecutor executor_;

        void timerCallback() override;
        void update();
    };

    ApplicationContext& context_;
    juce::OwnedArray<Row> rows_;
    MessageThreadExecutor executor_;
};
