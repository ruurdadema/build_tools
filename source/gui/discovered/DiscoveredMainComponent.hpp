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

#include "DiscoveredContainer.hpp"
#include "application/ApplicationContext.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class DiscoveredMainComponent : public juce::Component
{
public:
    explicit DiscoveredMainComponent (ApplicationContext& context);

    void resized() override;

private:
    DiscoveredContainer discoveredContainer_;
    juce::Viewport viewport_;
};
