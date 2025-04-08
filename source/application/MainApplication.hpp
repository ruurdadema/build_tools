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

#include <memory>

#include "gui/MainComponent.hpp"
#include "gui/lookandfeel/ThisLookAndFeel.hpp"
#include "ravennakit/core/support.hpp"
#include "ravennakit/core/system.hpp"

#include <CLI/CLI.hpp>

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
    DiscoveredSessions& getSessions() override;
    juce::AudioDeviceManager& getAudioDeviceManager() override;
    AudioReceivers& getAudioReceivers() override;
    AudioSenders& getAudioSenders() override;

private:
    juce::AudioDeviceManager audioDeviceManager_;
    std::unique_ptr<rav::RavennaNode> ravennaNode_;
    std::unique_ptr<DiscoveredSessions> sessions_;
    std::unique_ptr<AudioReceivers> audioReceivers_;
    std::unique_ptr<AudioSenders> audioSenders_;
    std::vector<std::unique_ptr<juce::ResizableWindow>> mainWindows_;

    void addWindow();
};
