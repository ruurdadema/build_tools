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

#include "gui/MainComponent.hpp"
#include "gui/lookandfeel/ThisLookAndFeel.hpp"
#include "gui/window/EulaAcceptWindow.hpp"
#include "models/AudioReceiversModel.hpp"
#include "ravennakit/core/support.hpp"
#include "ravennakit/core/system.hpp"

#include <memory>

class MainApplication final : public juce::JUCEApplication, public ApplicationContext
{
public:
    MainApplication();

    // juce::JUCEApplication overrides
    const juce::String getApplicationName() override;
    const juce::String getApplicationVersion() override;
    bool moreThanOneInstanceAllowed() override;
    void initialise (const juce::String& commandLine) override;
    void shutdown() override;
    void systemRequestedQuit() override;
    void anotherInstanceStarted (const juce::String& commandLine) override;
    void unhandledException (const std::exception* e, const juce::String& sourceFilename, int lineNumber) override;

    // ApplicationContext overrides
    void cloneWindow() override;
    void closeWindow (juce::Component* window) override;
    rav::RavennaNode& getRavennaNode() override;
    DiscoveredSessionsModel& getSessions() override;
    juce::AudioDeviceManager& getAudioDeviceManager() override;
    AudioReceiversModel& getAudioReceivers() override;
    AudioSendersModel& getAudioSenders() override;
    tl::expected<void, std::string> saveToFile (const juce::File& file) override;
    tl::expected<void, std::string> loadFromFile (const juce::File& file) override;
    [[nodiscard]] std::string getApplicationStateJson() override;
    [[nodiscard]] const juce::File& getApplicationStateFile() override;

private:
    juce::AudioDeviceManager audioDeviceManager_;
    std::unique_ptr<rav::RavennaNode> ravennaNode_;
    std::unique_ptr<DiscoveredSessionsModel> sessions_;
    std::unique_ptr<AudioReceiversModel> audioReceivers_;
    std::unique_ptr<AudioSendersModel> audioSenders_;
    std::vector<std::unique_ptr<juce::ResizableWindow>> mainWindows_;
    std::unique_ptr<EulaAcceptWindow> eulaAcceptWindow_;

    void addWindow();
    boost::json::object toBoostJson() const;
    bool restoreFromBoostJson (const boost::json::value& json);
    [[nodiscard]] static const juce::File& getEulaAcceptedFile();
};
