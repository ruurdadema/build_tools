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
        return juce::Colours::green.brighter(0.5f);
    default:
        return juce::Colours::white;
    }
}
} // namespace

PtpMainComponent::PtpMainComponent ([[maybe_unused]] ApplicationContext& context) :
    ravenna_node_ (context.getRavennaNode())
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

    port1StatusValue_.setText ("disabled", juce::dontSendNotification);
    addAndMakeVisible (port1StatusValue_);

    port2StatusTitle_.setText ("Port 2 Status", juce::dontSendNotification);
    addAndMakeVisible (port2StatusTitle_);

    port2StatusValue_.setText ("disabled", juce::dontSendNotification);
    addAndMakeVisible (port2StatusValue_);

    currentTimeTitle_.setText ("Current time (TAI)", juce::dontSendNotification);
    addAndMakeVisible (currentTimeTitle_);
    addAndMakeVisible (currentTimeValue_);

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
    using Tr = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;

    juce::Grid grid;
    grid.templateRows = { Tr (Fr (1)), Tr (Fr (1)), Tr (Fr (1)), Tr (Fr (1)),
                          Tr (Fr (1)), Tr (Fr (1)), Tr (Fr (1)), Tr (Fr (1)) };
    grid.templateColumns = { Tr (Fr (1)), Tr (Fr (3)) };
    grid.items = {
        juce::GridItem (grandmasterIdTitle_), juce::GridItem (grandmasterIdValue_), juce::GridItem (parentPortIdTitle_),
        juce::GridItem (parentPortIdValue_),  juce::GridItem (priority1Title_),     juce::GridItem (priority1Value_),
        juce::GridItem (priority2Title_),     juce::GridItem (priority2Value_),     juce::GridItem (port1StatusTitle_),
        juce::GridItem (port1StatusValue_),   juce::GridItem (port2StatusTitle_),   juce::GridItem (port2StatusValue_),
        juce::GridItem (currentTimeTitle_),   juce::GridItem (currentTimeValue_),
    };

    grid.performLayout (getLocalBounds().reduced (20).withWidth (580).withHeight (220));
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

void PtpMainComponent::timerCallback()
{
    currentTimeValue_.setText (get_local_clock().now().to_rfc3339_tai(), juce::dontSendNotification);
}
