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

#include "SdpViewerComponent.hpp"

#include "gui/lookandfeel/Constants.hpp"
#include "gui/lookandfeel/ThisLookAndFeel.hpp"
#include "ravennakit/core/support.hpp"

SdpViewerComponent::SdpViewerComponent (const std::string& sdpText)
{
    applyButton_.onClick = [this] {
        if (onApply == nullptr)
            return;
        auto result = rav::sdp::parse_session_description (sdpTextEditor_.getText().toStdString());
        if (!result)
        {
            errorLabel_.setText (result.error(), juce::dontSendNotification);
            return;
        }
        errorLabel_.setText ("", juce::dontSendNotification);
        onApply (std::move (*result));
        if (auto* parent = findParentComponentOfClass<juce::DialogWindow>())
            parent->exitModalState (1);
    };
    applyButton_.setButtonText ("Apply");
    applyButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::green);
    addAndMakeVisible (applyButton_);

    closeButton_.onClick = [this] {
        if (auto* parent = findParentComponentOfClass<juce::DialogWindow>())
            parent->exitModalState (0);
    };
    closeButton_.setButtonText ("Close");
    closeButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::red);
    addAndMakeVisible (closeButton_);

    copyButton_.onClick = [this] {
        juce::SystemClipboard::copyTextToClipboard (sdpTextEditor_.getText());
    };
    copyButton_.setButtonText ("Copy");
    copyButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::grey);
    addAndMakeVisible (copyButton_);

    errorLabel_.setColour (juce::Label::ColourIds::textColourId, Constants::Colours::red);
    addAndMakeVisible (errorLabel_);

    sdpTextEditor_.setMultiLine (true);
    sdpTextEditor_.setReturnKeyStartsNewLine (true);
    sdpTextEditor_.setText (sdpText, juce::dontSendNotification);
    sdpTextEditor_.setTextToShowWhenEmpty ("Enter SDP text here...", juce::Colours::grey);
    addAndMakeVisible (sdpTextEditor_);

    setLookAndFeel (&rav::get_global_instance_of_type<ThisLookAndFeel>());
}

SdpViewerComponent::~SdpViewerComponent()
{
    setLookAndFeel (nullptr);
}

void SdpViewerComponent::resized()
{
    auto b = getLocalBounds().reduced (kMargin);
    auto bottom = b.removeFromBottom (27);
    closeButton_.setBounds (bottom.removeFromRight (71));
    bottom.removeFromRight (6);
    if (onApply)
    {
        applyButton_.setBounds (bottom.removeFromRight (71));
        bottom.removeFromRight (6);
    }
    copyButton_.setBounds (bottom.removeFromRight (71));
    b.removeFromBottom (kMargin);
    errorLabel_.setBounds (b.removeFromBottom (16));
    b.removeFromBottom (kMargin);
    sdpTextEditor_.setBounds (b);
}
