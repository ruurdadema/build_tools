/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "DiscoveredMainComponent.hpp"

DiscoveredMainComponent::DiscoveredMainComponent (ApplicationContext& appContext, WindowContext& windowContext) :
    discoveredContainer_ (appContext, windowContext)
{
    viewport_.setViewedComponent (&discoveredContainer_, false);
    viewport_.setScrollBarsShown (true, false, true, false);
    addAndMakeVisible (viewport_);
}

void DiscoveredMainComponent::resized()
{
    const auto b = getLocalBounds();

    viewport_.setBounds (b);

    discoveredContainer_.setSize (viewport_.getWidth(), 10);
    discoveredContainer_.resizeToFitContent();
}
