#include "MainComponent.h"

#include "ravennakit/core/tracy.hpp"

MainComponent::MainComponent()
{
    setSize (600, 400);
}

void MainComponent::paint (juce::Graphics& g)
{
    TRACY_ZONE_SCOPED;

    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::FontOptions (16.0f));
    g.setColour (juce::Colours::white);
    g.drawText ("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized() {}
