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

#include "application/ApplicationContext.hpp"
#include "models/AudioSendersModel.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class SendersContainer : public juce::Component, AudioSendersModel::Subscriber
{
public:
    explicit SendersContainer (ApplicationContext& context);
    ~SendersContainer() override;

    void resizeToFitContent();
    void onAudioSenderUpdated (rav::Id senderId, const AudioSendersModel::SenderState* state) override;

    void resized() override;

private:
    static constexpr int kRowHeight = 120;
    static constexpr int kMargin = 10;
    static constexpr int kButtonHeight = 30;

    class Row : public Component
    {
    public:
        explicit Row (AudioSendersModel& audioSenders, rav::Id senderId);

        rav::Id getId() const;

        void update (const AudioSendersModel::SenderState& state);
        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        AudioSendersModel& audioSenders_;
        rav::Id senderId_;

        juce::Label sessionNameLabel_;
        juce::TextEditor sessionNameEditor_;

        juce::Label txPortLabel_;
        juce::ComboBox txPortComboBox_;

        juce::Label primaryAddressLabel_;
        juce::TextEditor primaryAddressEditor_;

        juce::Label secondaryAddressLabel_;
        juce::TextEditor secondaryAddressEditor_;

        juce::Label ttlLabel_;
        juce::TextEditor ttlEditor_;

        juce::Label payloadTypeLabel_;
        juce::TextEditor payloadTypeEditor_;

        juce::Label numChannelsLabel_;
        juce::TextEditor numChannelsEditor_;

        juce::Label sampleRateLabel_;
        juce::ComboBox sampleRateComboBox_;

        juce::Label encodingLabel_;
        juce::ComboBox encodingComboBox_;

        juce::TextButton onOffButton_ { "Start" };
        juce::TextButton deleteButton_ { "Delete" };
        juce::TextButton showSdpButton_ { "SDP" };

        juce::Label statusMessage_;

        AudioSendersModel::SenderState senderState_;

        MessageThreadExecutor executor_;

        boost::asio::ip::address_v4 getDestinationAddress (uint8_t rank) const;
    };

    ApplicationContext& context_;
    juce::OwnedArray<Row> rows_;
    juce::TextButton createButton { "Create sender" };
};
