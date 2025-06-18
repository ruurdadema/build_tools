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

#include <juce_gui_basics/juce_gui_basics.h>

class NmosMainComponent : public juce::Component, public rav::RavennaNode::Subscriber
{
public:
    explicit NmosMainComponent (ApplicationContext& context);
    ~NmosMainComponent() override;

    void resized() override;

    void nmos_node_config_updated (const rav::nmos::Node::Configuration& config) override;
    void nmos_node_status_changed (rav::nmos::Node::Status status, const rav::nmos::Node::StatusInfo& registry_info) override;

private:
    ApplicationContext& applicationContext_;

    juce::Label nmosSettingsLabel_;

    juce::Label nmosEnabledLabel_;
    juce::ToggleButton nmosEnabledToggle_;

    juce::Label operationModeLabel_;
    juce::ComboBox operationModeComboBox_;

    juce::Label registryAddressLabel_;
    juce::TextEditor registryAddressEditor_;

    juce::Label nmosVersionLabel_;
    juce::ComboBox nmosVersionComboBox_;

    juce::Label nmosLabelLabel_;
    juce::TextEditor nmosLabelEditor_;

    juce::Label nmosDescriptionLabel_;
    juce::TextEditor nmosDescriptionEditor_;

    juce::Label nmosApiPortLabel_;
    juce::TextEditor nmosApiPortEditor_;

    juce::Label nmosStatusTitleLabel_;

    juce::Label nmosStatusLabel_;
    juce::Label nmosStatusValueLabel_;

    juce::Label nmosRegistryNameLabel_;
    juce::Label nmosRegistryNameValueLabel_;

    juce::Label nmosRegistryAddressLabel_;
    juce::HyperlinkButton nmosRegistryAddressValue_;

    rav::nmos::Node::Configuration nmosConfiguration_;

    MessageThreadExecutor executor_;

    void update_api_port_editor (uint16_t port);
};
