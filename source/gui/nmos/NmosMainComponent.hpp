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
