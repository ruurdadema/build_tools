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

#include <juce_gui_basics/juce_gui_basics.h>

class WindowContext
{
public:
    virtual ~WindowContext() = default;
    virtual bool navigateTo (const juce::String& path) = 0;
};
