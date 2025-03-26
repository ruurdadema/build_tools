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

#include <../../../cmake-build-debug/_deps/juce-src/modules/juce_gui_basics/juce_gui_basics.h>

#include "../lookandfeel/Constants.hpp"
#include "ravennakit/core/assert.hpp"

class AddNewSenderDialog : public juce::Component
{
public:
    AddNewSenderDialog()
    {
        text_.setText ("Please enter a name for the RAVENNA session:", juce::dontSendNotification);
        addAndMakeVisible (text_);

        sessionNameEditor_.onReturnKey = [this] {
            addButton_.triggerClick();
        };
        sessionNameEditor_.onEscapeKey = [this] {
            cancelButton_.triggerClick();
        };
        addAndMakeVisible (sessionNameEditor_);

        addButton_.setWantsKeyboardFocus (false);
        addAndMakeVisible (addButton_);

        cancelButton_.setWantsKeyboardFocus (false);
        cancelButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::grey);
        cancelButton_.onClick = [this] {
            if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
                dw->exitModalState (0);
        };
        addAndMakeVisible (cancelButton_);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        b.removeFromTop (10);

        text_.setBounds (b.removeFromTop (30).reduced (10));
        sessionNameEditor_.setBounds (b.removeFromTop (50).reduced (10));

        auto row = b.removeFromBottom (50).reduced (10);
        cancelButton_.setBounds (row.removeFromRight (100));
        row.removeFromRight (10);
        addButton_.setBounds (row.removeFromRight (100));
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Constants::Colours::windowBackground);
    }

    void onAdd (const std::function<void (juce::String)>& callback)
    {
        addButton_.onClick = [this, callback] {
            if (callback)
                callback (sessionNameEditor_.getText());
            if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
                dw->exitModalState (0);
        };
    }

    void onCancel (const std::function<void()>& callback)
    {
        addButton_.onClick = [this, callback] {
            if (callback)
                callback();
            if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
                dw->exitModalState (0);
        };
    }

private:
    juce::Label text_;
    juce::TextEditor sessionNameEditor_;
    juce::TextButton addButton_ { "Add" };
    juce::TextButton cancelButton_ { "Cancel" };
};
