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

class SettingsMainComponent : public juce::Component, public rav::RavennaNode::Subscriber
{
public:
    explicit SettingsMainComponent (ApplicationContext& context);
    ~SettingsMainComponent() override;

    void resized() override;

    void network_interface_config_updated (const rav::RavennaConfig::NetworkInterfaceConfig& config) override;

private:
    [[maybe_unused]] ApplicationContext& context_;
    juce::Label networkSettingsLabel_;
    juce::Label primaryNetworkInterfaceLabel_;
    juce::ComboBox primaryNetworkInterfaceComboBox_;
    juce::Label secondaryNetworkInterfaceLabel_;
    juce::ComboBox secondaryNetworkInterfaceComboBox_;
    std::vector<rav::NetworkInterface> networkInterfaces_;

    void updateNetworkInterfaces();
    void selectNetworkInterfaces() const;
};
