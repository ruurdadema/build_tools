/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "ThisLookAndFeel.hpp"

#include "juce_gui_extra/juce_gui_extra.h"

ThisLookAndFeel::ThisLookAndFeel()
{
    // ResizableWindow
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xFF1A1A1E));

    // HyperLinkButton
    setColour (juce::HyperlinkButton::ColourIds::textColourId, juce::Colour (0xFF27BAFD));

    // ScrollBar
    setColour (juce::ScrollBar::ColourIds::thumbColourId, juce::Colour (0xFF6B6B6B));

    // TextButton
    setColour (juce::TextButton::ColourIds::textColourOnId, juce::Colour (0xFF000000));
    setColour (juce::TextButton::ColourIds::textColourOffId, juce::Colour (0xFF000000));

    // TextEditor
    setColour (juce::TextEditor::ColourIds::backgroundColourId, juce::Colour (0xFF2C2C31));

    // CaretComponent
    setColour (juce::CaretComponent::ColourIds::caretColourId, juce::Colour (0xFF27BAFD));
}

void ThisLookAndFeel::drawButtonBackground (
    juce::Graphics& graphics,
    juce::Button& button,
    const juce::Colour& backgroundColour,
    const bool shouldDrawButtonAsHighlighted,
    const bool shouldDrawButtonAsDown)
{
    const auto b = button.getLocalBounds();
    constexpr auto cornerSize = 6.0f;

    if ((shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted) && !button.isMouseButtonDown())
        graphics.setColour (juce::Colour (0xFF3BE075));
    else
        graphics.setColour (juce::Colour (0xFF27B559));

    graphics.fillRoundedRectangle (b.toFloat(), cornerSize);
}
