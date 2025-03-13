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

#include "models/AudioReceivers.hpp"
#include "models/RavennaSessions.hpp"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

class ApplicationContext
{
public:
    virtual ~ApplicationContext() = default;
    virtual void closeWindow(juce::Component* window) = 0;
    virtual void cloneWindow() = 0;
    virtual RavennaSessions& getSessions() = 0;
    virtual AudioReceivers& getAudioReceivers() = 0;
    virtual juce::AudioDeviceManager& getAudioDeviceManager() = 0;
};
