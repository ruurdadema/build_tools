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
    [[nodiscard]] virtual std::string getApplicationStateJson() = 0;
    [[nodiscard]] virtual const juce::File& getApplicationStateFile() = 0;
};
