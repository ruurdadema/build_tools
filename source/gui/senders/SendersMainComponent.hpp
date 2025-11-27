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

#include "SendersContainer.hpp"
#include "application/ApplicationContext.hpp"
#include "gui/widgets/MiniAudioDeviceSelectorComponent.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class SendersMainComponent : public juce::Component
{
public:
    explicit SendersMainComponent (ApplicationContext& context);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    SendersContainer sendersContainer_;
    juce::Viewport viewport_;

    MiniAudioDeviceSelectorComponent miniDeviceSelector_;
};
