/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "PtpMainComponent.hpp"

#include <juce_gui_extra/juce_gui_extra.h>

namespace
{
juce::Colour ptpStateToColour (const rav::ptp::State state)
{
    switch (state)
    {
    case rav::ptp::State::uncalibrated:
        return juce::Colours::orange;
    case rav::ptp::State::slave:
        return juce::Colours::green.brighter (0.5f);
    default:
        return juce::Colours::white;
    }
}
} // namespace

PtpMainComponent::PtpMainComponent ([[maybe_unused]] ApplicationContext& context) : ravenna_node_ (context.getRavennaNode())
{
    grandmasterIdTitle_.setText ("Grandmaster ID", juce::dontSendNotification);
    addAndMakeVisible (grandmasterIdTitle_);
    addAndMakeVisible (grandmasterIdValue_);

    parentPortIdTitle_.setText ("Parent Port ID", juce::dontSendNotification);
    addAndMakeVisible (parentPortIdTitle_);
    addAndMakeVisible (parentPortIdValue_);

    priority1Title_.setText ("Priority 1", juce::dontSendNotification);
    addAndMakeVisible (priority1Title_);
    addAndMakeVisible (priority1Value_);

    priority2Title_.setText ("Priority 2", juce::dontSendNotification);
    addAndMakeVisible (priority2Title_);
    addAndMakeVisible (priority2Value_);

    port1StatusTitle_.setText ("Port 1 Status", juce::dontSendNotification);
    addAndMakeVisible (port1StatusTitle_);

    addAndMakeVisible (port1StatusValue_);

    port2StatusTitle_.setText ("Port 2 Status", juce::dontSendNotification);
    addAndMakeVisible (port2StatusTitle_);

    addAndMakeVisible (port2StatusValue_);

    currentTimeTitle_.setText ("Current time (TAI)", juce::dontSendNotification);
    addAndMakeVisible (currentTimeTitle_);
    addAndMakeVisible (currentTimeValue_);

    domainTitle_.setText ("Domain", juce::dontSendNotification);
    addAndMakeVisible (domainTitle_);

    domainTextEditor_.setInputRestrictions (3, "0123456789");
    domainTextEditor_.onReturnKey = [this] {
        auto config = ptp_config_;
        config.domain_number = static_cast<uint8_t> (domainTextEditor_.getText().getIntValue());
        if (auto result = ravenna_node_.set_ptp_instance_configuration (config).get(); !result)
        {
            RAV_ERROR ("Failed to update PTP config: {}", result.error());
        }
        juce::TextEditor::unfocusAllComponents();
    };
    domainTextEditor_.onEscapeKey = [this] {
        domainTextEditor_.setText (juce::String (ptp_config_.domain_number), juce::dontSendNotification);
        juce::TextEditor::unfocusAllComponents();
    };
    domainTextEditor_.onFocusLost = [this] {
        domainTextEditor_.onReturnKey();
    };
    addAndMakeVisible (domainTextEditor_);

    reset();

    ravenna_node_.subscribe_to_ptp_instance (this).wait();

    startTimer (50);
}

PtpMainComponent::~PtpMainComponent()
{
    stopTimer();

    ravenna_node_.unsubscribe_from_ptp_instance (this).wait();
}

void PtpMainComponent::resized()
{
    auto b = getLocalBounds().reduced (20);

    auto left = b.removeFromLeft (200);
    auto right = b.removeFromLeft (300);

    auto h = 30;
    auto addRow = [&left, &right, &h] (Component& l, Component& r) {
        l.setBounds (left.removeFromTop (h));
        r.setBounds (right.removeFromTop (h));
    };

    addRow (grandmasterIdTitle_, grandmasterIdValue_);
    addRow (parentPortIdTitle_, parentPortIdValue_);
    addRow (priority1Title_, priority1Value_);
    addRow (priority2Title_, priority2Value_);
    addRow (port1StatusTitle_, port1StatusValue_);
    addRow (port2StatusTitle_, port2StatusValue_);
    addRow (currentTimeTitle_, currentTimeValue_);

    domainTitle_.setBounds (left.removeFromTop (h));
    domainTextEditor_.setBounds (right.removeFromTop (h).reduced (3).withWidth (80));
}

void PtpMainComponent::ptp_parent_changed (const rav::ptp::ParentDs& parent)
{
    executor_.callAsync ([this, parent] {
        grandmasterIdValue_.setText (parent.grandmaster_identity.to_string(), juce::dontSendNotification);
        parentPortIdValue_.setText (parent.parent_port_identity.clock_identity.to_string(), juce::dontSendNotification);
        priority1Value_.setText (juce::String (parent.grandmaster_priority1), juce::dontSendNotification);
        priority2Value_.setText (juce::String (parent.grandmaster_priority2), juce::dontSendNotification);
    });
}

void PtpMainComponent::ptp_port_changed_state (const rav::ptp::Port& port)
{
    auto portNumber = port.port_ds().port_identity.port_number;
    auto portState = port.port_ds().port_state;

    executor_.callAsync ([this, portNumber, portState] {
        if (portNumber == 1)
        {
            port1StatusValue_.setColour (juce::Label::ColourIds::textColourId, ptpStateToColour (portState));
            port1StatusValue_.setText (rav::ptp::to_string (portState), juce::dontSendNotification);
        }
        else if (portNumber == 2)
        {
            port2StatusValue_.setColour (juce::Label::ColourIds::textColourId, ptpStateToColour (portState));
            port2StatusValue_.setText (rav::ptp::to_string (portState), juce::dontSendNotification);
        }
        else
            RAV_ERROR ("Invalid port number {}", portNumber);
    });
}

void PtpMainComponent::ptp_port_removed (const uint16_t portNumber)
{
    executor_.callAsync ([this, portNumber] {
        if (portNumber == 1)
        {
            port1StatusValue_.setColour (juce::Label::ColourIds::textColourId, juce::Colours::grey);
            port1StatusValue_.setText ("disabled", juce::dontSendNotification);
        }
        else if (portNumber == 2)
        {
            port2StatusValue_.setColour (juce::Label::ColourIds::textColourId, juce::Colours::grey);
            port2StatusValue_.setText ("disabled", juce::dontSendNotification);
        }
        else
            RAV_ERROR ("Invalid port number {}", portNumber);
    });
}

void PtpMainComponent::ptp_configuration_updated (const rav::ptp::Instance::Configuration& config)
{
    executor_.callAsync ([this, config] {
        ptp_config_ = config;
        domainTextEditor_.setText (juce::String (ptp_config_.domain_number), juce::dontSendNotification);
    });
}

void PtpMainComponent::timerCallback()
{
    currentTimeValue_.setText (get_local_clock().now().to_rfc3339_tai(), juce::dontSendNotification);
}

void PtpMainComponent::reset()
{
    grandmasterIdValue_.setText ({}, juce::dontSendNotification);
    parentPortIdValue_.setText ({}, juce::dontSendNotification);

    priority1Value_.setText ({}, juce::dontSendNotification);
    priority2Value_.setText ({}, juce::dontSendNotification);

    port1StatusValue_.setColour (juce::Label::ColourIds::textColourId, juce::Colours::grey);
    port1StatusValue_.setText ("disabled", juce::dontSendNotification);

    port2StatusValue_.setColour (juce::Label::ColourIds::textColourId, juce::Colours::grey);
    port2StatusValue_.setText ("disabled", juce::dontSendNotification);

    currentTimeValue_.setText ({}, juce::dontSendNotification);
}
