/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "DiscoveredSessionsContainer.hpp"

#include "gui/lookandfeel/Constants.hpp"

DiscoveredSessionsContainer::DiscoveredSessionsContainer (ApplicationContext& context) : context_ (context)
{
    context_.getRavennaNode().add_subscriber (this).wait();
}

DiscoveredSessionsContainer::~DiscoveredSessionsContainer()
{
    context_.getRavennaNode().remove_subscriber (this).wait();
}

void DiscoveredSessionsContainer::paint (juce::Graphics&) {}

void DiscoveredSessionsContainer::resized()
{
    auto b = getLocalBounds().reduced (kMargin);
    for (auto i = 0; i < rows_.size(); ++i)
    {
        rows_.getUnchecked (i)->setBounds (b.removeFromTop (kRowHeight));
        b.removeFromTop (kMargin);
    }
}

void DiscoveredSessionsContainer::resizeBasedOnContent()
{
    setSize (getWidth(), rows_.size() * kRowHeight + kMargin + kMargin * rows_.size());
}

void DiscoveredSessionsContainer::ravenna_session_discovered (const rav::dnssd::dnssd_browser::service_resolved& event)
{
    executor_.callAsync ([this, desc = event.description] {
        for (auto* row : rows_)
        {
            if (row->getSessionName() == juce::StringRef (desc.name))
            {
                row->update (desc);
                return;
            }
        }

        auto* row = rows_.add (std::make_unique<Row> (context_, desc));
        RAV_ASSERT (row != nullptr, "Failed to create row");
        addAndMakeVisible (row);
        resizeBasedOnContent();
    });
}

void DiscoveredSessionsContainer::ravenna_session_removed (const rav::dnssd::dnssd_browser::service_removed& event)
{
    executor_.callAsync ([this, name = event.description.name] {
        for (auto i = 0; i < rows_.size(); ++i)
        {
            if (rows_.getUnchecked (i)->getSessionName() == juce::StringRef (name))
            {
                rows_.remove (i);
                resizeBasedOnContent();
                return;
            }
        }
    });
}

DiscoveredSessionsContainer::Row::Row (
    ApplicationContext& context,
    const rav::dnssd::service_description& serviceDescription)
{
    sessionName_.setText (serviceDescription.name, juce::dontSendNotification);
    sessionName_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (sessionName_);

    description_.setText (serviceDescription.host_target, juce::dontSendNotification);
    description_.setJustificationType (juce::Justification::topLeft);
    description_.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (description_);

    startButton_.setButtonText ("Play");
    startButton_.onClick = [&context, name = serviceDescription.name] {
        const auto id = context.getRavennaNode().create_receiver (name).get();
        RAV_TRACE ("Created receiver with id: {}", id.value());
    };
    addAndMakeVisible (startButton_);
}

juce::String DiscoveredSessionsContainer::Row::getSessionName() const
{
    return sessionName_.getText();
}

void DiscoveredSessionsContainer::Row::update (const rav::dnssd::service_description& serviceDescription)
{
    sessionName_.setText (serviceDescription.name, juce::dontSendNotification);
    description_.setText (serviceDescription.host_target, juce::dontSendNotification);
}

void DiscoveredSessionsContainer::Row::resized()
{
    auto b = getLocalBounds().reduced (kMargin);

    startButton_.setBounds (b.removeFromRight (60));
    sessionName_.setBounds (b.removeFromTop (20));
    description_.setBounds (b);
}

void DiscoveredSessionsContainer::Row::paint (juce::Graphics& g)
{
    g.setColour (Constants::Colours::rowBackground);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);
}
