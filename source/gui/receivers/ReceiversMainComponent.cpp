/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "ReceiversMainComponent.hpp"

ReceiversMainComponent::ReceiversMainComponent (ApplicationContext& context) :
    streamsContainer_ (context),
    miniDeviceSelector_ (context.getAudioDeviceManager(), MiniAudioDeviceSelectorComponent::Direction::Output)
{
    viewport_.setViewedComponent (&streamsContainer_, false);
    addAndMakeVisible (viewport_);

    addAndMakeVisible (miniDeviceSelector_);
}

void ReceiversMainComponent::paint (juce::Graphics&) {}

void ReceiversMainComponent::resized()
{
    auto b = getLocalBounds();

    miniDeviceSelector_.setBounds (b.removeFromBottom (50).reduced (10));

    viewport_.setBounds (b);
    streamsContainer_.setSize (viewport_.getWidth(), 10);
    streamsContainer_.resizeToFitContent();
}
