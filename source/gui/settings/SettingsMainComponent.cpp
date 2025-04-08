/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "SettingsMainComponent.hpp"

SettingsMainComponent::SettingsMainComponent (ApplicationContext& context) : context_ (context)
{
    networkSettingsLabel_.setText ("Network Settings", juce::dontSendNotification);
    networkSettingsLabel_.setFont (juce::FontOptions (16.f, juce::Font::bold));
    networkSettingsLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (networkSettingsLabel_);

    primaryNetworkInterfaceLabel_.setText ("Primary Network Interface", juce::dontSendNotification);
    addAndMakeVisible (primaryNetworkInterfaceLabel_);

    primaryNetworkInterfaceComboBox_.setTextWhenNothingSelected ("Primary Network Interface");
    primaryNetworkInterfaceComboBox_.onChange = [this] {
        selectNetworkInterfaces();
    };
    addAndMakeVisible (primaryNetworkInterfaceComboBox_);

    secondaryNetworkInterfaceLabel_.setText ("Secondary Network Interface", juce::dontSendNotification);
    addAndMakeVisible (secondaryNetworkInterfaceLabel_);

    secondaryNetworkInterfaceComboBox_.setTextWhenNothingSelected ("Secondary Network Interface");
    secondaryNetworkInterfaceComboBox_.onChange = [this] {
        selectNetworkInterfaces();
    };
    addAndMakeVisible (secondaryNetworkInterfaceComboBox_);

    updateNetworkInterfaces();

    context_.getRavennaNode().subscribe (this).wait();
}

SettingsMainComponent::~SettingsMainComponent()
{
    context_.getRavennaNode().unsubscribe (this).wait();
}

void SettingsMainComponent::resized()
{
    auto b = getLocalBounds().reduced (10);
    networkSettingsLabel_.setBounds (b.removeFromTop (28));

    constexpr int left = 200;
    constexpr int width = 380;

    auto row = b.removeFromTop (28);
    primaryNetworkInterfaceLabel_.setBounds (row.removeFromLeft (left));
    primaryNetworkInterfaceComboBox_.setBounds (row.withWidth (width));

    b.removeFromTop (10);

    row = b.removeFromTop (28);
    secondaryNetworkInterfaceLabel_.setBounds (row.removeFromLeft (left));
    secondaryNetworkInterfaceComboBox_.setBounds (row.withWidth (width));
}

void SettingsMainComponent::network_interface_config_updated (const rav::RavennaConfig::NetworkInterfaceConfig& config)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (context_.getRavennaNode());

    SafePointer weakThis (this);
    juce::MessageManager::callAsync ([weakThis, config] {
        if (!weakThis)
            return;

        if (config.primary_interface)
        {
            weakThis->primaryNetworkInterfaceComboBox_.setText (
                config.primary_interface->get_extended_display_name(),
                juce::dontSendNotification);
        }

        if (config.secondary_interface)
        {
            weakThis->secondaryNetworkInterfaceComboBox_.setText (
                config.secondary_interface->get_extended_display_name(),
                juce::dontSendNotification);
        }
    });
}

void SettingsMainComponent::updateNetworkInterfaces()
{
    auto ifaces = rav::NetworkInterface::get_all();
    if (!ifaces)
    {
        juce::NativeMessageBox::showMessageBoxAsync (
            juce::AlertWindow::WarningIcon,
            "Network Interfaces",
            "Failed to retrieve network interfaces: " + std::to_string (ifaces.error()));
        return;
    }

    networkInterfaces_ = std::move (*ifaces);

    primaryNetworkInterfaceComboBox_.clear();
    secondaryNetworkInterfaceComboBox_.clear();

    for (size_t i = 0; i < networkInterfaces_.size(); ++i)
    {
        auto& iface = networkInterfaces_[i];
        if (iface.get_type() != rav::NetworkInterface::Type::wired_ethernet)
            continue;
        if (iface.get_display_name().empty())
            continue;
        auto extended = iface.get_extended_display_name();
        primaryNetworkInterfaceComboBox_.addItem (extended, static_cast<int> (i + 1));
        secondaryNetworkInterfaceComboBox_.addItem (extended, static_cast<int> (i + 1));
    }
}

void SettingsMainComponent::selectNetworkInterfaces() const
{
    rav::RavennaConfig::NetworkInterfaceConfig config;

    auto id = primaryNetworkInterfaceComboBox_.getSelectedId();
    config.primary_interface = id > 0 ? networkInterfaces_.at (static_cast<uint32_t> (id - 1))
                                      : std::optional<rav::NetworkInterface>();

    id = secondaryNetworkInterfaceComboBox_.getSelectedId();
    config.secondary_interface = id > 0 ? networkInterfaces_.at (static_cast<uint32_t> (id - 1))
                                        : std::optional<rav::NetworkInterface>();

    context_.getRavennaNode().set_network_interface_config (config).wait();
}
