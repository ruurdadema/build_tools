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
#include "models/AudioReceiversModel.hpp"
#include "util/MessageThreadExecutor.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class ReceiversContainer : public juce::Component, public AudioReceiversModel::Subscriber
{
public:
    explicit ReceiversContainer (ApplicationContext& context);
    ~ReceiversContainer() override;

    void resized() override;

    void resizeToFitContent();

    void onAudioReceiverUpdated (rav::Id receiverId, const AudioReceiversModel::ReceiverState* state) override;
    void onAudioReceiverStatsUpdated (rav::Id receiverId, size_t streamIndex, const rav::rtp::PacketStats::Counters& stats) override;

private:
    static constexpr int kRowHeight = 158;
    static constexpr int kMargin = 10;
    static constexpr int kButtonHeight = 30;

    class SdpViewer : public Component
    {
    public:
        rav::SafeFunction<void (rav::sdp::SessionDescription sdp)> onApply;

        explicit SdpViewer (const std::string& sdpText);
        ~SdpViewer() override;
        void resized() override;

    private:
        juce::TextEditor sdpTextEditor_;
        juce::TextButton closeButton_ { "Close" };
        juce::TextButton applyButton_ { "Apply" };
        juce::TextButton copyButton_ { "Copy" };
        juce::Label errorLabel_ { "error" };
    };

    class SessionInfoComponent : public Component
    {
    public:
        explicit SessionInfoComponent (const juce::String& title);
        void update (const AudioReceiversModel::StreamState* state);
        void resized() override;

    private:
        juce::Label titleLabel_ { "title" };
        juce::Label sessionInfoLabel_ { "session_info" };
        juce::Label packetTimeLabel_ { " packet_time" };
        juce::Label statusLabel_ { "status" };
    };

    class PacketStatsComponent : public Component
    {
    public:
        PacketStatsComponent();
        void update (const rav::rtp::PacketStats::Counters& stats);
        void resized() override;

    private:
        juce::Label titleLabel_ { "title" };
        juce::Label jitterMaxLabel_ { "jitter_max" };
        juce::Label droppedLabel_ { "dropped" };
        juce::Label duplicatesLabel_ { "duplicates" };
        juce::Label outOfOrderLabel_ { "out_of_order" };
        juce::Label tooLateLabel_ { "too_late" };
    };

    class Row : public Component
    {
    public:
        explicit Row (AudioReceiversModel& audioReceivers, rav::Id receiverId);

        rav::Id getId() const;

        void update (const AudioReceiversModel::ReceiverState& state);
        void update (size_t streamIndex, const rav::rtp::PacketStats::Counters& stats);
        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        AudioReceiversModel& audioReceivers_;
        juce::TextEditor delayEditor_;
        rav::Id receiverId_;
        rav::RavennaReceiver::Configuration configuration_;

        // Session
        juce::Label sessionNameLabel_ { "session_name" };
        juce::Label audioFormatLabel_ { "audio_format" };
        juce::Label settingsLabel_ { "settings" };
        juce::Label delaySettingLabel_ { "delay" };

        // Buttons
        juce::TextButton showSdpButton_ { "" };
        juce::TextButton onOffButton_ { "Start" };
        juce::TextButton deleteButton_ { "" };

        // Warning
        juce::Label warningLabel_ { "warning" };

        SessionInfoComponent primarySessionInfo_ { "Primary" };
        PacketStatsComponent primaryPacketStats_;

        SessionInfoComponent secondarySessionInfo_ { "Secondary" };
        PacketStatsComponent secondaryPacketStats_;

        MessageThreadExecutor executor_;
    };

    ApplicationContext& context_;
    juce::OwnedArray<Row> rows_;
    juce::TextButton createButton { "Create receiver" };

    MessageThreadExecutor executor_;
};
