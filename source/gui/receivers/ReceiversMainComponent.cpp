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
    discoveredStreamsContainer_ (context),
    streamsContainer_ (context),
    miniDeviceSelector_ (context.getAudioDeviceManager(), MiniAudioDeviceSelectorComponent::Direction::Output)
{
    leftViewport_.setViewedComponent (&discoveredStreamsContainer_, false);
    addAndMakeVisible (leftViewport_);

    rightViewport_.setViewedComponent (&streamsContainer_, false);
    addAndMakeVisible (rightViewport_);

    addAndMakeVisible (miniDeviceSelector_);
}

void ReceiversMainComponent::paint (juce::Graphics&) {}

void ReceiversMainComponent::resized()
{
    auto b = getLocalBounds();

    miniDeviceSelector_.setBounds (b.removeFromBottom (50).reduced (10));
    const auto left = b.removeFromLeft (b.getWidth() / 4);
    const auto right = b;

    leftViewport_.setBounds (left);
    rightViewport_.setBounds (right);

    discoveredStreamsContainer_.setSize (leftViewport_.getWidth() - 10, 10);
    discoveredStreamsContainer_.resizeBasedOnContent();

    streamsContainer_.setSize (rightViewport_.getWidth() - 10, 10);
    streamsContainer_.resizeToFitContent();
}
