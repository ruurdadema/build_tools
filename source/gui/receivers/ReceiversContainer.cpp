/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "ReceiversContainer.hpp"

#include "juce_gui_extra/juce_gui_extra.h"

ReceiversContainer::ReceiversContainer (ApplicationContext& context) : context_ (context)
{
    context_.getRavennaNode().subscribe (this).wait();
}

ReceiversContainer::~ReceiversContainer()
{
    context_.getRavennaNode().unsubscribe (this).wait();
}

void ReceiversContainer::resized()
{
    auto b = getLocalBounds().reduced (kMargin);
    for (auto i = 0; i < rows_.size(); ++i)
    {
        rows_.getUnchecked (i)->setBounds (b.removeFromTop (kRowHeight));
        b.removeFromTop (kMargin);
    }
}

void ReceiversContainer::resizeBasedOnContent()
{
    setSize (getWidth(), rows_.size() * kRowHeight + kMargin + kMargin * rows_.size());
}

void ReceiversContainer::on_receiver_updated (const rav::id receiver_id)
{
    executor_.callAsync ([this, receiver_id] {
        for (const auto& row : rows_)
        {
            if (row->getId() == receiver_id)
            {
                row->repaint();
                return;
            }
        }

        auto* row = rows_.add (std::make_unique<Row> (context_.getRavennaNode(), receiver_id));
        RAV_ASSERT (row != nullptr, "Failed to create row");
        addAndMakeVisible (row);
        resizeBasedOnContent();
    });
}

ReceiversContainer::Row::Row (rav::ravenna_node& node, const rav::id receiverId) :
    node_ (node),
    receiverId_ (receiverId)
{
    delayEditor_.setInputRestrictions (10, "0123456789");
    addAndMakeVisible (delayEditor_);

    update();
    startTimer (1000);
}

ReceiversContainer::Row::~Row() {}

rav::id ReceiversContainer::Row::getId() const
{
    return receiverId_;
}

void ReceiversContainer::Row::paint (juce::Graphics& g)
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

    g.drawText (packet_stats_.dropped, column2.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText (packet_stats_.duplicates, column2.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText (packet_stats_.out_of_order, column2.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText (packet_stats_.too_late, column2.removeFromTop (rowHeight), juce::Justification::centredLeft);

    g.drawText (interval_stats_.avg, column3.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText (interval_stats_.median, column3.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText (interval_stats_.min, column3.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText (interval_stats_.max, column3.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText (interval_stats_.stddev, column3.removeFromTop (rowHeight), juce::Justification::centredLeft);

    g.drawText ("delay (samples)", column4.removeFromTop (rowHeight), juce::Justification::centredLeft);
}

void ReceiversContainer::Row::resized()
{
    auto b = getLocalBounds().reduced (kMargin);
    b.removeFromTop (18);
    delayEditor_.setBounds (b.withLeft (718).withHeight (24).withWidth (60));
}

void ReceiversContainer::Row::timerCallback()
{
    update();
}

void ReceiversContainer::Row::update()
{
    const auto stats = node_.get_stats_for_stream (receiverId_).get();

    // Packet stats
    packet_stats_.dropped = "dropped: " + juce::String (stats.packet_stats.dropped);
    packet_stats_.duplicates = "duplicates: " + juce::String (stats.packet_stats.duplicates);
    packet_stats_.out_of_order = "out of order: " + juce::String (stats.packet_stats.out_of_order);
    packet_stats_.too_late = "too late: " + juce::String (stats.packet_stats.too_late);

    // Interval stats
    interval_stats_.avg = "avg: " + juce::String (stats.packet_interval_stats.average);
    interval_stats_.median = "median: " + juce::String (stats.packet_interval_stats.median);
    interval_stats_.min = "min: " + juce::String (stats.packet_interval_stats.min);
    interval_stats_.max = "max: " + juce::String (stats.packet_interval_stats.max);
    interval_stats_.stddev = "stddev: " + juce::String (stats.packet_interval_stats.stddev);

    repaint();
}
