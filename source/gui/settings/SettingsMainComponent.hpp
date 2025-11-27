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

#include "application/ApplicationContext.hpp"
#include "application/DiagnosticsFile.hpp"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

class SettingsMainComponent : public juce::Component, public rav::RavennaNode::Subscriber
{
public:
    explicit SettingsMainComponent (ApplicationContext& context);
    ~SettingsMainComponent() override;

    void resized() override;

    void network_interface_config_updated (const rav::NetworkInterfaceConfig& config) override;
    void ravenna_node_configuration_updated (const rav::RavennaNode::Configuration& configuration) override;

private:
    [[maybe_unused]] ApplicationContext& context_;
    rav::RavennaNode::Configuration node_configuration_;

    juce::Label networkSettingsLabel_;

    juce::Label primaryNetworkInterfaceLabel_;
    juce::ComboBox primaryNetworkInterfaceComboBox_;

    juce::Label secondaryNetworkInterfaceLabel_;
    juce::ComboBox secondaryNetworkInterfaceComboBox_;

    juce::Array<rav::NetworkInterface::Identifier> networkInterfaces_;

    juce::Label bonjourSettingsLabel_;

    juce::Label enableSessionAdvertisementLabel_;
    juce::ToggleButton enableSessionAdvertisement_;

    juce::Label enableNodeDiscoveryLabel_;
    juce::ToggleButton enableNodeDiscovery_;

    juce::Label enableSessionDiscoveryLabel_;
    juce::ToggleButton enableSessionDiscovery_;

    juce::Label buildInfoTitleLabel_;

    juce::Label applicationVersionTitleLabel_;
    juce::Label applicationVersionLabel_;

    juce::Label diagnosticsTitleLabel_;
    juce::TextButton saveDiagnosticsButton_;
    juce::TextButton revealLogsButton_;
    std::unique_ptr<DiagnosticsFile> diagnosticsFile_;

    void updateNetworkInterfaces();
    void selectNetworkInterfaces() const;
    void saveDiagnosticsFile();
};
