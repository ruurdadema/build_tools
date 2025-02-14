/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#pragma once

#include "DiscoveredStreamsContainer.hpp"
#include "StreamsContainer.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class ReceiversMainComponent : public juce::Component {
public:
    ReceiversMainComponent();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    DiscoveredStreamsContainer discoveredStreamsContainer_;
    juce::Viewport leftViewport_;

    StreamsContainer streamsContainer_;
    juce::Viewport rightViewport_;
};
