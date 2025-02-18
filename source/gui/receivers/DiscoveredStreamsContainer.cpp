/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "DiscoveredStreamsContainer.hpp"

DiscoveredStreamsContainer::DiscoveredStreamsContainer (ApplicationContext& context) : context_ (context)
{
    context_.getRavennaNode().subscribe_to_browser (this);
}

DiscoveredStreamsContainer::~DiscoveredStreamsContainer()
{
    context_.getRavennaNode().unsubscribe_from_browser (this);
}

void DiscoveredStreamsContainer::paint (juce::Graphics&) {}

void DiscoveredStreamsContainer::resized()
{
    auto b = getLocalBounds().reduced (kMargin);
    for (auto i = 0; i < rows_.size(); ++i)
    {
        rows_.getUnchecked (i)->setBounds (b.removeFromTop (kRowHeight));
        b.removeFromTop (kMargin);
    }
}

void DiscoveredStreamsContainer::resizeBasedOnContent()
{
    setSize (getWidth(), rows_.size() * kRowHeight + kMargin + kMargin * rows_.size());
}

void DiscoveredStreamsContainer::ravenna_session_discovered (const rav::dnssd::dnssd_browser::service_resolved& event)
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

        auto* row = rows_.add (std::make_unique<Row> (desc));
        RAV_ASSERT (row != nullptr, "Failed to create row");
        addAndMakeVisible (row);
        resizeBasedOnContent();
    });
}

void DiscoveredStreamsContainer::ravenna_session_removed (const rav::dnssd::dnssd_browser::service_removed& event)
{
    executor_.callAsync ([this, name = event.description.name] {
        for (auto i = 0; i < rows_.size(); ++i)
        {
            if (rows_.getUnchecked (i)->getSessionName() == juce::StringRef(name))
            {
                rows_.remove (i);
                resizeBasedOnContent();
                return;
            }
        }
    });
}

DiscoveredStreamsContainer::Row::Row (const rav::dnssd::service_description& serviceDescription)
{
    sessionName_.setText (serviceDescription.name, juce::dontSendNotification);
    sessionName_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (sessionName_);

    description_.setText (serviceDescription.host_target, juce::dontSendNotification);
    description_.setJustificationType (juce::Justification::topLeft);
    description_.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (description_);

    startButton_.setButtonText ("Play");
    addAndMakeVisible (startButton_);
}

juce::String DiscoveredStreamsContainer::Row::getSessionName() const
{
    return sessionName_.getText();
}

void DiscoveredStreamsContainer::Row::update (const rav::dnssd::service_description& serviceDescription)
{
    sessionName_.setText (serviceDescription.name, juce::dontSendNotification);
    description_.setText (serviceDescription.host_target, juce::dontSendNotification);
}

void DiscoveredStreamsContainer::Row::resized()
{
    auto b = getLocalBounds().reduced (kMargin);

    startButton_.setBounds (b.removeFromRight (60));
    sessionName_.setBounds (b.removeFromTop (20));
    description_.setBounds (b);
}

void DiscoveredStreamsContainer::Row::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xFF242428));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);
}
