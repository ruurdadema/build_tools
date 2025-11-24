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
