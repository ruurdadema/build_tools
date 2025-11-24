/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
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
