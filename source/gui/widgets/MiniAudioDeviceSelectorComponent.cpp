/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "MiniAudioDeviceSelectorComponent.hpp"

#include "gui/lookandfeel/Constants.hpp"
#include "gui/lookandfeel/ThisLookAndFeel.hpp"
#include "ravennakit/core/log.hpp"
#include "ravennakit/core/support.hpp"

#include <juce_audio_utils/gui/juce_AudioDeviceSelectorComponent.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace
{
constexpr auto kNoneDevice = "< < none > >";
}

MiniAudioDeviceSelectorComponent::MiniAudioDeviceSelectorComponent (
    juce::AudioDeviceManager& deviceManager,
    const Direction direction) :
    audioDeviceManager_ (deviceManager),
    direction_ (direction)
{
    label_.setText (direction == Direction::Input ? "Input device" : "Output device", juce::dontSendNotification);
    label_.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (label_);

    comboBox_.onChange = [this] {
        if (comboBox_.getSelectedItemIndex() == comboBox_.getNumItems() - 1)
        {
            audioDeviceManager_.closeAudioDevice();
            return;
        }
        auto setup = audioDeviceManager_.getAudioDeviceSetup();
        direction_ == Direction::Input ? setup.inputDeviceName = comboBox_.getText()
                                       : setup.outputDeviceName = comboBox_.getText();
        if (const auto result = audioDeviceManager_.setAudioDeviceSetup (setup, true); result.isNotEmpty())
        {
            RAV_ERROR ("Failed to select audio device: {}", result.toRawUTF8());
        }
    };
    addAndMakeVisible (comboBox_);

    settingsButton_.setButtonText ("Settings");
    settingsButton_.onClick = [this] {
        auto content = std::make_unique<
            juce::AudioDeviceSelectorComponent> (audioDeviceManager_, 0, 128, 0, 128, false, false, false, false);

        juce::DialogWindow::LaunchOptions o;
        o.dialogTitle = "SDP Text";
        o.content.setOwned (content.release());
        o.componentToCentreAround = getTopLevelComponent();
        o.dialogBackgroundColour = Constants::Colours::windowBackground;
        o.escapeKeyTriggersCloseButton = true;
        o.useNativeTitleBar = true;
        o.resizable = true;
        o.useBottomRightCornerResizer = false;
        if (auto* dialog = o.launchAsync())
        {
            dialog->centreWithSize (600, 100);
            dialog->setLookAndFeel (&rav::get_global_instance_of_type<ThisLookAndFeel>());
        }
    };
    addAndMakeVisible (settingsButton_);

    audioDeviceManager_.addChangeListener (this);
    update();
}

MiniAudioDeviceSelectorComponent::~MiniAudioDeviceSelectorComponent()
{
    audioDeviceManager_.removeChangeListener (this);
}

void MiniAudioDeviceSelectorComponent::resized()
{
    static constexpr int comboBoxWidth = 240;

    auto b = getLocalBounds();
    const auto lr = (b.getWidth() - comboBoxWidth) / 2;
    label_.setBounds (b.removeFromLeft (lr).withTrimmedRight (7));
    comboBox_.setBounds (b.removeFromLeft (comboBoxWidth));
    settingsButton_.setBounds (b.withWidth (100).withTrimmedLeft (8));
}

void MiniAudioDeviceSelectorComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == &audioDeviceManager_)
        update();
}

void MiniAudioDeviceSelectorComponent::update()
{
    comboBox_.clear();

    const auto* type = audioDeviceManager_.getCurrentDeviceTypeObject();
    if (type == nullptr)
        return;

    const auto setup = audioDeviceManager_.getAudioDeviceSetup();
    const auto currentDeviceName = direction_ == Direction::Input ? setup.inputDeviceName : setup.outputDeviceName;
    auto devices = type->getDeviceNames (direction_ == Direction::Input);

    for (const auto& device : devices)
    {
        comboBox_.addItem (device, comboBox_.getNumItems() + 1);
        if (currentDeviceName.isNotEmpty() && device == currentDeviceName)
            comboBox_.setSelectedId (comboBox_.getNumItems());
    }

    // Add none device
    comboBox_.addItem (kNoneDevice, comboBox_.getNumItems() + 1);
    if (currentDeviceName.isEmpty())
        comboBox_.setSelectedId (comboBox_.getNumItems(), juce::dontSendNotification);
}
