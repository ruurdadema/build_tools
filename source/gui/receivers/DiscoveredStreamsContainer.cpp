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
    context_.getRavennaNode().subscribe (this);

    for (auto i = 0; i < 10; ++i)
    {
        auto* row = rows_.add (std::make_unique<Row> ("Stream " + juce::String (i + 1), "Stream hostname"));
        addAndMakeVisible (row);
    }
}

DiscoveredStreamsContainer::~DiscoveredStreamsContainer()
{
    context_.getRavennaNode().unsubscribe (this);
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
    RAV_TRACE ("Stream discovered: {}", event.description.to_string());
}

DiscoveredStreamsContainer::Row::Row (const juce::String& streamName, const juce::String& streamDescription) :
    streamName_ (streamName, streamName),
    streamDescription_ (streamDescription, streamDescription)
{
    streamName_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (streamName_);

    streamDescription_.setJustificationType (juce::Justification::topLeft);
    streamDescription_.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (streamDescription_);

    startButton_.setButtonText ("Play");
    addAndMakeVisible (startButton_);
}

void DiscoveredStreamsContainer::Row::resized()
{
    auto b = getLocalBounds().reduced (kMargin);

    startButton_.setBounds (b.removeFromRight (60));
    streamName_.setBounds (b.removeFromTop (20));
    streamDescription_.setBounds (b);
}

void DiscoveredStreamsContainer::Row::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xFF242428));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);
}
