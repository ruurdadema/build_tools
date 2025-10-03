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

#include "models/AudioReceiversModel.hpp"
#include "models/AudioSendersModel.hpp"
#include "models/DiscoveredSessionsModel.hpp"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

class ApplicationContext
{
public:
    virtual ~ApplicationContext() = default;
    virtual void closeWindow (juce::Component* window) = 0;
    virtual void cloneWindow() = 0;
    [[nodiscard]] virtual rav::RavennaNode& getRavennaNode() = 0;
    [[nodiscard]] virtual DiscoveredSessionsModel& getSessions() = 0;
    [[nodiscard]] virtual AudioReceiversModel& getAudioReceivers() = 0;
    [[nodiscard]] virtual AudioSendersModel& getAudioSenders() = 0;
    [[nodiscard]] virtual juce::AudioDeviceManager& getAudioDeviceManager() = 0;
    [[nodiscard]] virtual tl::expected<void, std::string> saveToFile (const juce::File& file) = 0;
    [[nodiscard]] virtual tl::expected<void, std::string> loadFromFile (const juce::File& file) = 0;
};
