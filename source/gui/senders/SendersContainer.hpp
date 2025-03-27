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
#include "models/AudioSenders.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class SendersContainer : public juce::Component, AudioSenders::Subscriber
{
public:
    explicit SendersContainer (ApplicationContext& context);
    ~SendersContainer() override;

    void resizeToFitContent();
    void onAudioSenderUpdated (rav::Id senderId, const AudioSenders::SenderState* state) override;

    void resized() override;

private:
    static constexpr int kRowHeight = 80;
    static constexpr int kMargin = 10;
    static constexpr int kButtonHeight = 30;

    class Row : public Component
    {
    public:
        explicit Row (AudioSenders& audioSenders, rav::Id senderId);

        rav::Id getId() const;

        void update (const AudioSenders::SenderState& state);
        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        AudioSenders& audioSenders_;
        rav::Id senderId_;
        juce::Label sessionNameLabel_;
        juce::TextEditor sessionNameEditor_;

        juce::Label addressLabel_;
        juce::TextEditor addressEditor_;

        juce::Label ttlLabel_;
        juce::TextEditor ttlEditor_;

        juce::Label payloadTypeLabel_;
        juce::TextEditor payloadTypeEditor_;

        juce::Label formatLabel_;
        juce::ComboBox formatComboBox_;

        juce::TextButton startStopButton_ { "Start" };
        juce::TextButton deleteButton_ { "Delete" };

        AudioSenders::SenderState senderState_;

        MessageThreadExecutor executor_;
    };

    ApplicationContext& context_;
    juce::OwnedArray<Row> rows_;
    juce::TextButton addButton { "Add sender" };
};
