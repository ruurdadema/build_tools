/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "HorizontalMenu.hpp"

HorizontalMenu::HorizontalMenu()

{
    receiversButton_.setButtonText ("Receivers");
    receiversButton_.setFont (juce::FontOptions (16.f), false);
    addAndMakeVisible (receiversButton_);

    sendersButton_.setButtonText ("Senders");
    sendersButton_.setFont (juce::FontOptions (16.f), false);
    addAndMakeVisible (sendersButton_);
}

void HorizontalMenu::paint (juce::Graphics& g)
{
    g.drawRect (getLocalBounds(), 1);
}

void HorizontalMenu::resized()
{
    auto b = getLocalBounds();
    receiversButton_.setBounds (b.removeFromLeft (100));
    sendersButton_.setBounds (b.removeFromLeft (100));
}
