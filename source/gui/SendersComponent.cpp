/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "SendersComponent.hpp"

SendersComponent::SendersComponent() {}

void SendersComponent::paint (juce::Graphics& g)
{
    g.drawRect (getLocalBounds());
    g.drawText ("Senders", getLocalBounds(), juce::Justification::centred);
}

void SendersComponent::resized()
{
    Component::resized();
}