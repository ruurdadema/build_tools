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
