/*
 * Project: RAVENNAKIT demo application
 * Copyright (c) 2024-2025 Sound on Digital
 *
 * This file is part of the RAVENNAKIT demo application.
 *
 * The RAVENNAKIT demo application is licensed on a proprietary,
 * source-available basis. You are allowed to view, modify, and compile
 * this code for personal or internal evaluation only.
 *
 * You may NOT redistribute this file, any modified versions, or any
 * compiled binaries to third parties, and you may NOT use it for any
 * commercial purpose, except under a separate written agreement with
 * Sound on Digital.
 *
 * For the full license terms, see:
 *   LICENSE.md
 * or visit:
 *   https://ravennakit.com
 */

#pragma once

#include "DiscoveredContainer.hpp"
#include "application/ApplicationContext.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class DiscoveredMainComponent : public juce::Component
{
public:
    explicit DiscoveredMainComponent (ApplicationContext& appContext, WindowContext& windowContext);

    void resized() override;

private:
    DiscoveredContainer discoveredContainer_;
    juce::Viewport viewport_;
};
