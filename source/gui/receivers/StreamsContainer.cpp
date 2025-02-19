/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "StreamsContainer.hpp"

#include "juce_gui_extra/juce_gui_extra.h"

StreamsContainer::StreamsContainer() {}

void StreamsContainer::resized()
{
    auto b = getLocalBounds().reduced (kMargin);
    for (auto i = 0; i < rows_.size(); ++i)
    {
        rows_.getUnchecked (i)->setBounds (b.removeFromTop (kRowHeight));
        b.removeFromTop (kMargin);
    }
}

void StreamsContainer::resizeBasedOnContent()
{
    setSize (getWidth(), rows_.size() * kRowHeight + kMargin + kMargin * rows_.size());
}

void StreamsContainer::on_receiver_updated (rav::id receiver_id) {}

StreamsContainer::Row::Row (const rav::id receiverId) : receiverId_ (receiverId)
{
    delayEditor_.setInputRestrictions (10, "0123456789");
    addAndMakeVisible (delayEditor_);
}

rav::id StreamsContainer::Row::getId() const
{
    return receiverId_;
}

void StreamsContainer::Row::paint (juce::Graphics& g)
{
    constexpr float fontSize = 14.0f;
    constexpr int rowHeight = 20;

    g.setColour (juce::Colour (0xFF242428));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);

    auto b = getLocalBounds().reduced (kMargin + 5, kMargin);
    auto column1 = b.removeFromLeft (200);
    auto column2 = b.removeFromLeft (200);
    auto column3 = b.removeFromLeft (200);
    auto column4 = b.removeFromLeft (200);

    g.setColour (juce::Colour (0xFFFFFFFF));
    g.setFont (juce::FontOptions (fontSize, juce::Font::bold));
    g.drawText ("Anubus_610120_27", column1.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("Stats", column2.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("Receive interval", column3.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("Settings", column4.removeFromTop (rowHeight), juce::Justification::centredLeft);

    g.setFont (juce::FontOptions (fontSize, juce::Font::plain));
    g.drawText ("L24/48000/2", column1.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("239.1.16.51", column1.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("Status: ok", column1.removeFromTop (rowHeight), juce::Justification::centredLeft);

    g.drawText ("dropped: 0", column2.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("duplicates: 0", column2.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("out of order: 0", column2.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("too late: 0", column2.removeFromTop (rowHeight), juce::Justification::centredLeft);

    g.drawText ("avg: 1", column3.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("median: 1", column3.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("min: 0.8", column3.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("max: 2.1", column3.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("stddev: 0.09", column3.removeFromTop (rowHeight), juce::Justification::centredLeft);

    g.drawText ("delay (samples)", column4.removeFromTop (rowHeight), juce::Justification::centredLeft);
}

void StreamsContainer::Row::resized()
{
    auto b = getLocalBounds().reduced (kMargin);
    b.removeFromTop (18);
    delayEditor_.setBounds (b.withLeft (718).withHeight (24).withWidth (60));
}
