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

#include "ravennakit/core/util/safe_function.hpp"

#include <juce_gui_extra/juce_gui_extra.h>

class EulaAcceptWindow : public juce::ResizableWindow
{
public:
    rav::SafeFunction<void(const juce::String& eulaHash)> onEulaAccepted;
    rav::SafeFunction<void()> onEulaDeclined;

    explicit EulaAcceptWindow (const juce::String& name);
    ~EulaAcceptWindow() override;

    static juce::String getEulaHash();
};
