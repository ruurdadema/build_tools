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

#include "audio/AudioReceivers.hpp"
#include "ravennakit/ravenna/ravenna_node.hpp"

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>

class ApplicationContext
{
public:
    virtual ~ApplicationContext() = default;
    virtual rav::ravenna_node& getRavennaNode() = 0;
    virtual void closeWindow(juce::Component* window) = 0;
    virtual void cloneWindow() = 0;
    virtual AudioReceivers& getAudioReceivers() = 0;
    virtual juce::AudioDeviceManager& getAudioDeviceManager() = 0;
};
