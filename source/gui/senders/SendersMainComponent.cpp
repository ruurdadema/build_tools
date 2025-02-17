/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "SendersMainComponent.hpp"

SendersMainComponent::SendersMainComponent (ApplicationContext& context)
{
    std::ignore = context;
}

void SendersMainComponent::paint (juce::Graphics& g)
{
    g.drawRect (getLocalBounds());
    g.drawText ("Senders", getLocalBounds(), juce::Justification::centred);
}

void SendersMainComponent::resized()
{
    Component::resized();
}