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

#include "application/ApplicationContext.hpp"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

class SettingsMainComponent : public juce::Component
{
public:
    explicit SettingsMainComponent (ApplicationContext& context);

    void resized() override;

private:
    [[maybe_unused]] ApplicationContext& context_;
};
