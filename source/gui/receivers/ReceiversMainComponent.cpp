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

ReceiversMainComponent::ReceiversMainComponent()
{
    leftViewport_.setViewedComponent (&discoveredStreamsContainer_, false);
    addAndMakeVisible (leftViewport_);

    rightViewport_.setViewedComponent (&streamsContainer_, false);
    addAndMakeVisible (rightViewport_);
}

void ReceiversMainComponent::paint (juce::Graphics& g)
{
}

void ReceiversMainComponent::resized()
{
    auto b = getLocalBounds();
    leftViewport_.setBounds (b.removeFromLeft (b.getWidth() / 4));
    rightViewport_.setBounds (b);

    discoveredStreamsContainer_.setSize(leftViewport_.getWidth() - 10, 10);
    discoveredStreamsContainer_.resizeBasedOnContent();

    streamsContainer_.setSize(rightViewport_.getWidth() - 10, 10);
    streamsContainer_.resizeBasedOnContent();
}
