/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "NmosMainComponent.hpp"

#include "gui/lookandfeel/Constants.hpp"
#include "ravennakit/nmos/detail/nmos_operating_mode.hpp"

NmosMainComponent::NmosMainComponent (ApplicationContext& context) : applicationContext_ (context)
{
    nmosSettingsLabel_.setText ("NMOS Settings", juce::dontSendNotification);
    nmosSettingsLabel_.setFont (juce::FontOptions (16.f, juce::Font::bold));
    nmosSettingsLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (nmosSettingsLabel_);

    nmosEnabledLabel_.setText ("NMOS Enabled", juce::dontSendNotification);
    nmosEnabledToggle_.onClick = [this] {
        rav::nmos::Node::ConfigurationUpdate update;
        update.enabled = nmosEnabledToggle_.getToggleState();
        applicationContext_.getRavennaNode().update_nmos_configuration (update).wait();
    };
    addAndMakeVisible (nmosEnabledLabel_);

    addAndMakeVisible (nmosEnabledToggle_);

    operationModeLabel_.setText ("Operation Mode", juce::dontSendNotification);
    addAndMakeVisible (operationModeLabel_);

    operationModeComboBox_.addItem ("mDNS", static_cast<int> (rav::nmos::OperationMode::mdns_p2p) + 1);
    operationModeComboBox_.addItem ("Manual", static_cast<int> (rav::nmos::OperationMode::manual) + 1);
    operationModeComboBox_.addItem ("Peer to peer", static_cast<int> (rav::nmos::OperationMode::p2p) + 1);
    operationModeComboBox_.onChange = [this] {
        rav::nmos::Node::ConfigurationUpdate update;
        update.operation_mode = static_cast<rav::nmos::OperationMode> (operationModeComboBox_.getSelectedId() - 1);
        applicationContext_.getRavennaNode().update_nmos_configuration (update).wait();
    };
    addAndMakeVisible (operationModeComboBox_);

    registryAddressLabel_.setText ("Registry address", juce::dontSendNotification);
    addAndMakeVisible (registryAddressLabel_);

    registryAddressEditor_.onReturnKey = [this] {
        rav::nmos::Node::ConfigurationUpdate update;
        update.registry_address = registryAddressEditor_.getText().toStdString();
        applicationContext_.getRavennaNode().update_nmos_configuration (std::move (update)).wait();
        juce::TextEditor::unfocusAllComponents();
    };
    registryAddressEditor_.onEscapeKey = [this] {
        registryAddressEditor_.setText (nmosConfiguration_.registry_address, juce::dontSendNotification);
        juce::TextEditor::unfocusAllComponents();
    };
    registryAddressEditor_.onFocusLost = [this] {
        registryAddressEditor_.setText (nmosConfiguration_.registry_address, juce::dontSendNotification);
    };
    registryAddressEditor_.setIndents (6, 6);
    addAndMakeVisible (registryAddressEditor_);

    nmosStatusLabel_.setText ("NMOS Status", juce::dontSendNotification);
    nmosStatusLabel_.setFont (juce::FontOptions (16.f, juce::Font::bold));
    nmosStatusLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (nmosStatusLabel_);

    nmosRegisteredLabel_.setText ("Registered", juce::dontSendNotification);
    addAndMakeVisible (nmosRegisteredLabel_);

    nmosRegisteredValueLabel_.setText ("No", juce::dontSendNotification);
    addAndMakeVisible (nmosRegisteredValueLabel_);

    nmosRegistryNameLabel_.setText ("Registry Name", juce::dontSendNotification);
    addAndMakeVisible (nmosRegistryNameLabel_);

    nmosRegistryNameValueLabel_.setText ("(not registered)", juce::dontSendNotification);
    addAndMakeVisible (nmosRegistryNameValueLabel_);

    nmosRegistryAddressLabel_.setText ("Registry Address", juce::dontSendNotification);
    addAndMakeVisible (nmosRegistryAddressLabel_);

    nmosRegistryAddressValueLabel_.setText ("(not registered)", juce::dontSendNotification);
    addAndMakeVisible (nmosRegistryAddressValueLabel_);

    nmosErrorLabel_.setText ("Error", juce::dontSendNotification);
    addAndMakeVisible (nmosErrorLabel_);

    nmosErrorValueLabel_.setText ("Some really bad error", juce::dontSendNotification);
    addAndMakeVisible (nmosErrorValueLabel_);
    nmosErrorValueLabel_.setColour (juce::Label::textColourId, juce::Colours::red);

    applicationContext_.getRavennaNode().subscribe (this).wait();
}

NmosMainComponent::~NmosMainComponent()
{
    applicationContext_.getRavennaNode().unsubscribe (this).wait();
}

void NmosMainComponent::resized()
{
    auto b = getLocalBounds().reduced (10);
    nmosSettingsLabel_.setBounds (b.removeFromTop (28));

    constexpr int left = 150;
    constexpr int width = 380;

    auto row = b.removeFromTop (28);
    nmosEnabledLabel_.setBounds (row.removeFromLeft (left));
    nmosEnabledToggle_.setBounds (row.withWidth (width));

    b.removeFromTop (10);

    row = b.removeFromTop (28);
    operationModeLabel_.setBounds (row.removeFromLeft (left));
    operationModeComboBox_.setBounds (row.withWidth (width));

    if (registryAddressEditor_.isVisible())
    {
        b.removeFromTop (10);

        row = b.removeFromTop (28);
        registryAddressLabel_.setBounds (row.removeFromLeft (left));
        registryAddressEditor_.setBounds (row.withWidth (width));
    }

    b.removeFromTop (20);

    nmosStatusLabel_.setBounds (b.removeFromTop (28));

    row = b.removeFromTop (28);
    nmosRegisteredLabel_.setBounds (row.removeFromLeft (left));
    nmosRegisteredValueLabel_.setBounds (row.withWidth (width));

    row = b.removeFromTop (28);
    nmosRegistryNameLabel_.setBounds (row.removeFromLeft (left));
    nmosRegistryNameValueLabel_.setBounds (row.withWidth (width));

    row = b.removeFromTop (28);
    nmosRegistryAddressLabel_.setBounds (row.removeFromLeft (left));
    nmosRegistryAddressValueLabel_.setBounds (row.withWidth (width));

    if (nmosErrorValueLabel_.isVisible())
    {
        row = b.removeFromTop (28);
        nmosErrorLabel_.setBounds (row.removeFromLeft (left));
        nmosErrorValueLabel_.setBounds (row.withWidth (width));
    }
}

void NmosMainComponent::nmos_node_config_updated (const rav::nmos::Node::Configuration& config)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (applicationContext_.getRavennaNode());
    executor_.callAsync ([this, config] {
        nmosConfiguration_ = std::move (config);
        nmosEnabledToggle_.setToggleState (nmosConfiguration_.enabled, juce::dontSendNotification);
        operationModeComboBox_.setSelectedId (
            static_cast<int> (nmosConfiguration_.operation_mode) + 1,
            juce::dontSendNotification);
        registryAddressLabel_.setVisible (nmosConfiguration_.operation_mode == rav::nmos::OperationMode::manual);
        registryAddressEditor_.setVisible (nmosConfiguration_.operation_mode == rav::nmos::OperationMode::manual);
        registryAddressEditor_.setText (nmosConfiguration_.registry_address, juce::dontSendNotification);
        resized();
    });
}

void NmosMainComponent::nmos_node_status_changed (const rav::nmos::Node::Status& status)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (applicationContext_.getRavennaNode());
    executor_.callAsync ([this, status] {
        nmosRegisteredValueLabel_.setText (status.registered ? "Yes" : "No", juce::dontSendNotification);
        nmosRegistryNameValueLabel_.setText (status.registry_server_name, juce::dontSendNotification);
        nmosRegistryAddressValueLabel_.setText (status.registry_server_address, juce::dontSendNotification);

        if (!status.error_message.empty())
        {
            nmosErrorValueLabel_.setText (status.error_message, juce::dontSendNotification);
            nmosErrorValueLabel_.setVisible (true);
            nmosErrorLabel_.setVisible (true);
        }
        else
        {
            nmosErrorValueLabel_.setText ({}, juce::dontSendNotification);
            nmosErrorValueLabel_.setVisible (false);
            nmosErrorLabel_.setVisible (false);
        }
        resized();
    });
}
