/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "MainComponent.hpp"

#include "ravennakit/core/tracy.hpp"

MainComponent::MainComponent()
{
    button_.setButtonText ("Click me!");
    button_.setFont (juce::FontOptions(16.f), true);
    button_.setMouseCursor (juce::MouseCursor::NormalCursor);
    button_.onClick = [] {
        DBG("Clicked");
    };
    addAndMakeVisible (button_);

    addAndMakeVisible (menu_);
}

void MainComponent::paint (juce::Graphics& g)
{
    TRACY_ZONE_SCOPED;

    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::FontOptions (16.0f));
    g.setColour (juce::Colours::white);
    g.drawText ("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    auto b = getLocalBounds();

    menu_.setBounds (b.removeFromTop (80));
    button_.setBounds (b.removeFromTop (40).reduced (8));
}
