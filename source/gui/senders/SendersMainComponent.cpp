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

#include "gui/widgets/MiniAudioDeviceSelectorComponent.hpp"

SendersMainComponent::SendersMainComponent (ApplicationContext& context) :
    miniDeviceSelector_ (context.getAudioDeviceManager(), MiniAudioDeviceSelectorComponent::Direction::Input)
{
    addAndMakeVisible (miniDeviceSelector_);
}

void SendersMainComponent::paint (juce::Graphics&) {}

void SendersMainComponent::resized()
{
    auto b = getLocalBounds();
    miniDeviceSelector_.setBounds (b.removeFromBottom (50).reduced (10));
}
