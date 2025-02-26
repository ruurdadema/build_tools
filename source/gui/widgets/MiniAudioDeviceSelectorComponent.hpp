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

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

class MiniAudioDeviceSelectorComponent : public juce::Component, public juce::ChangeListener
{
public:
    enum class Direction
    {
        Input,
        Output
    };

    MiniAudioDeviceSelectorComponent (juce::AudioDeviceManager& deviceManager, Direction direction);
    ~MiniAudioDeviceSelectorComponent() override;

    void resized() override;

    void changeListenerCallback ([[maybe_unused]] juce::ChangeBroadcaster* source) override;

private:
    juce::AudioDeviceManager& audioDeviceManager_;
    Direction direction_;
    juce::Label label_;
    juce::ComboBox comboBox_;
    juce::TextButton settingsButton_;

    void update();
};
