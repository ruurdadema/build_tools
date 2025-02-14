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

ThisLookAndFeel::ThisLookAndFeel()
{
    // ResizableWindow
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xFF1A1A1E));

    // HyperLinkButton
    setColour (juce::HyperlinkButton::ColourIds::textColourId, juce::Colour (0xFF27BAFD));
}
