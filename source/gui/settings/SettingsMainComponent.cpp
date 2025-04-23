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

    SafePointer safeThis (this);
    juce::MessageManager::callAsync ([safeThis, config] {
        if (!safeThis)
            return;

        const auto networkInterfaces = rav::NetworkInterfaceList::get_system_interfaces();

        if (config.primary)
        {
            if (auto* iface = networkInterfaces.get_interface (*config.primary))
            {
                safeThis->primaryNetworkInterfaceComboBox_.setText (
                    iface->get_extended_display_name(),
                    juce::dontSendNotification);
            }
            else
            {
                safeThis->primaryNetworkInterfaceComboBox_.setText (
                    *config.primary + " (not found)",
                    juce::dontSendNotification);
            }
        }

        if (config.secondary)
        {
            if (auto* iface = networkInterfaces.get_interface (*config.secondary))
            {
                safeThis->secondaryNetworkInterfaceComboBox_.setText (
                    iface->get_extended_display_name(),
                    juce::dontSendNotification);
            }
            else
            {
                safeThis->secondaryNetworkInterfaceComboBox_.setText (
                    *config.secondary + " (not found)",
                    juce::dontSendNotification);
            }
        }
    });
}

void SettingsMainComponent::updateNetworkInterfaces()
{
    const auto systemInterfaces = rav::NetworkInterfaceList::get_system_interfaces().get_interfaces();

    juce::Array<rav::NetworkInterface::Identifier> ids;

    primaryNetworkInterfaceComboBox_.clear();
    secondaryNetworkInterfaceComboBox_.clear();

    for (auto& iface : systemInterfaces)
    {
        if (iface.get_type() != rav::NetworkInterface::Type::wired_ethernet)
            continue;
        if (iface.get_display_name().empty())
            continue;
        ids.add (iface.get_identifier());
        auto extended = iface.get_extended_display_name();
        primaryNetworkInterfaceComboBox_.addItem (extended, ids.size());
        secondaryNetworkInterfaceComboBox_.addItem (extended, ids.size());
    }

    networkInterfaces_ = std::move (ids);
}

void SettingsMainComponent::selectNetworkInterfaces() const
{
    rav::RavennaConfig::NetworkInterfaceConfig config;

    auto id = primaryNetworkInterfaceComboBox_.getSelectedId();
    RAV_ASSERT (id <= networkInterfaces_.size(), "Invalid id");
    config.primary = id > 0 ? networkInterfaces_[id - 1] : std::optional<rav::NetworkInterface::Identifier>();

    id = secondaryNetworkInterfaceComboBox_.getSelectedId();
    RAV_ASSERT (id <= networkInterfaces_.size(), "Invalid id");
    config.secondary = id > 0 ? networkInterfaces_[id - 1]
                                        : std::optional<rav::NetworkInterface::Identifier>();

    context_.getRavennaNode().set_network_interface_config (config).wait();
}
