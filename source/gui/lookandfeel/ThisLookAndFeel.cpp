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

#include "ThisLookAndFeel.hpp"

#include "Constants.hpp"
#include "juce_gui_extra/juce_gui_extra.h"

ThisLookAndFeel::ThisLookAndFeel()
{
    // ResizableWindow
    setColour (juce::ResizableWindow::backgroundColourId, Constants::Colours::windowBackground);

    // HyperLinkButton
    setColour (juce::HyperlinkButton::ColourIds::textColourId, Constants::Colours::blue);

    // ScrollBar
    setColour (juce::ScrollBar::ColourIds::thumbColourId, Constants::Colours::grey);

    // TextButton
    setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::buttonSuccess);
    setColour (juce::TextButton::ColourIds::buttonOnColourId, Constants::Colours::buttonSuccess);
    setColour (juce::TextButton::ColourIds::textColourOnId, Constants::Colours::black);
    setColour (juce::TextButton::ColourIds::textColourOffId, Constants::Colours::black);

    // TextEditor
    setColour (juce::TextEditor::ColourIds::backgroundColourId, Constants::Colours::rowBackground);
    setColour (juce::TextEditor::ColourIds::outlineColourId, Constants::Colours::grey);
    setColour (juce::TextEditor::ColourIds::focusedOutlineColourId, Constants::Colours::grey);

    // CaretComponent
    setColour (juce::CaretComponent::ColourIds::caretColourId, Constants::Colours::blue);

    // ComboBox
    setColour (juce::ComboBox::ColourIds::backgroundColourId, Constants::Colours::rowBackground);

    // PopupMenu
    setColour (juce::PopupMenu::ColourIds::backgroundColourId, Constants::Colours::rowBackground);

    // ListBox
    setColour (juce::ListBox::ColourIds::backgroundColourId, Constants::Colours::rowBackground);
}

void ThisLookAndFeel::drawButtonBackground (
    juce::Graphics& graphics,
    juce::Button& button,
    const juce::Colour& backgroundColour,
    const bool shouldDrawButtonAsHighlighted,
    const bool shouldDrawButtonAsDown)
{
    const auto b = button.getLocalBounds();
    constexpr auto cornerSize = 4.0f;

    if ((shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted) && !button.isMouseButtonDown())
        graphics.setColour (backgroundColour.brighter (0.2f));
    else
        graphics.setColour (backgroundColour);

    graphics.fillRoundedRectangle (b.toFloat(), cornerSize);
}

void ThisLookAndFeel::drawTextEditorOutline (juce::Graphics& graphics, const int width, const int height, juce::TextEditor& textEditor)
{
    if (dynamic_cast<juce::AlertWindow*> (textEditor.getParentComponent()) == nullptr)
    {
        if (textEditor.hasKeyboardFocus (true) && !textEditor.isReadOnly())
        {
            auto colour = textEditor.findColour (juce::TextEditor::focusedOutlineColourId);
            if (!textEditor.isEnabled())
                colour = colour.darker();
            graphics.setColour (colour);
            graphics.drawRect (0, 0, width, height, 2);
        }
        else
        {
            auto colour = textEditor.findColour (juce::TextEditor::outlineColourId);
            if (!textEditor.isEnabled())
                colour = colour.darker();
            graphics.setColour (colour);
            graphics.drawRect (0, 0, width, height);
        }
    }
}
