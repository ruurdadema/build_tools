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

#include <juce_gui_basics/juce_gui_basics.h>

namespace Constants::Colours
{
static const juce::Colour black { 0xFF000000 };
static const juce::Colour white { 0xFFFFFFFF };
static const juce::Colour dark { 0xFF1A1A1E };
static const juce::Colour green { 0xFF27B559 };
static const juce::Colour red { 0xFFFE5C5D };
static const juce::Colour blue { 0xFF27BAFD };
static const juce::Colour grey { 0xFF6B6B6B };
static const juce::Colour yellow { 0xFFFFBE12 };

static const juce::Colour text { white };
static const juce::Colour textDisabled { grey };
static const juce::Colour rowBackground { 0xFF242428 };
static const juce::Colour windowBackground { dark };
static const juce::Colour buttonSuccess { green };
static const juce::Colour warning { yellow };
static const juce::Colour error { red };
} // namespace Constants::Colours
