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

#include "gui/lookandfeel/Constants.hpp"
#include "gui/lookandfeel/ThisLookAndFeel.hpp"
#include "juce_gui_extra/juce_gui_extra.h"
#include "ravennakit/core/chrono/high_resolution_clock.hpp"
#include "ravennakit/core/support.hpp"
#include "ravennakit/core/util/todo.hpp"

ReceiversContainer::ReceiversContainer (ApplicationContext& context) : context_ (context)
{
    if (!context_.getAudioReceivers().addSubscriber (this))
    {
        RAV_ERROR ("Failed to add subscriber");
    }
}

ReceiversContainer::~ReceiversContainer()
{
    if (!context_.getAudioReceivers().removeSubscriber (this))
    {
        RAV_ERROR ("Failed to remove subscriber");
    }
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

void ReceiversContainer::resizeToFitContent()
{
    setSize (getWidth(), rows_.size() * kRowHeight + kMargin + kMargin * rows_.size());
}

void ReceiversContainer::onAudioReceiverUpdated (rav::id receiverId, const AudioReceivers::ReceiverState* state)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (state != nullptr)
    {
        for (const auto& row : rows_)
        {
            if (row->getId() == receiverId)
            {
                row->update (*state);
                return;
            }
        }

        auto* row = rows_.add (std::make_unique<Row> (context_.getAudioReceivers(), receiverId, state->sessionName));
        RAV_ASSERT (row != nullptr, "Failed to create row");
        row->update (*state);
        addAndMakeVisible (row);
        resizeToFitContent();
    }
    else
    {
        for (auto i = 0; i < rows_.size(); ++i)
        {
            if (rows_.getUnchecked (i)->getId() == receiverId)
            {
                rows_.remove (i);
                resizeToFitContent();
                return;
            }
        }
    }
}

ReceiversContainer::SdpViewer::SdpViewer (const std::string& sdpText) : sdpText_ (sdpText)
{
    closeButton_.onClick = [this] {
        if (auto* parent = findParentComponentOfClass<juce::DialogWindow>())
            parent->exitModalState (0);
    };
    closeButton_.setButtonText ("Close");
    closeButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::red);
    addAndMakeVisible (closeButton_);

    copyButton_.onClick = [this] {
        juce::SystemClipboard::copyTextToClipboard (sdpText_);
    };
    copyButton_.setButtonText ("Copy");
    copyButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::grey);
    addAndMakeVisible (copyButton_);

    setLookAndFeel (&rav::get_global_instance_of_type<ThisLookAndFeel>());
}

ReceiversContainer::SdpViewer::~SdpViewer()
{
    setLookAndFeel (nullptr);
}

void ReceiversContainer::SdpViewer::resized()
{
    auto b = getLocalBounds().reduced (kMargin);
    auto bottom = b.removeFromBottom (27);
    closeButton_.setBounds (bottom.removeFromRight (71));
    bottom.removeFromRight (6);
    copyButton_.setBounds (bottom.removeFromRight (71));
}

void ReceiversContainer::SdpViewer::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().reduced (10);
    g.setColour (Constants::Colours::text);
    g.drawMultiLineText (sdpText_, b.getX(), b.getY() + 10, b.getWidth(), juce::Justification::topLeft);
}

ReceiversContainer::Row::Row (AudioReceivers& audioReceivers, const rav::id receiverId, const std::string& name) :
    audioReceivers_ (audioReceivers),
    receiverId_ (receiverId)
{
    stream_.streamName = name;

    delayEditor_.setInputRestrictions (10, "0123456789");
    delayEditor_.onReturnKey = [this] {
        const auto value = static_cast<uint32_t> (delayEditor_.getText().getIntValue());
        audioReceivers_.setReceiverDelay (receiverId_, value);
    };
    delayEditor_.onEscapeKey = [this] {
        delayEditor_.setText (juce::String (delay_));
        unfocusAllComponents();
    };
    delayEditor_.onFocusLost = [this] {
        delayEditor_.setText (juce::String (delay_));
    };
    addAndMakeVisible (delayEditor_);

    showSdpButton_.setButtonText ("Show SDP");
    showSdpButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::grey);
    showSdpButton_.onClick = [this] {
        auto result = audioReceivers_.getSdpTextForReceiver (receiverId_);

        auto content = std::make_unique<SdpViewer> (result ? std::move (*result) : "SDP text not available");
        content->setSize (400, 400);

        juce::DialogWindow::LaunchOptions o;
        o.dialogTitle = "SDP Text";
        o.content.setOwned (content.release());
        o.componentToCentreAround = getTopLevelComponent();
        o.dialogBackgroundColour = Constants::Colours::windowBackground;
        o.escapeKeyTriggersCloseButton = true;
        o.useNativeTitleBar = true;
        o.resizable = true;
        o.useBottomRightCornerResizer = false;
        if (auto* dialog = o.launchAsync())
            dialog->setResizeLimits (400, 400, 99999, 99999);
    };
    addAndMakeVisible (showSdpButton_);

    deleteButton_.setButtonText ("Delete");
    deleteButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::red);
    deleteButton_.onClick = [this] {
        audioReceivers_.removeReceiver (receiverId_);
    };
    addAndMakeVisible (deleteButton_);

    update();
    startTimer (1000);
}

rav::id ReceiversContainer::Row::getId() const
{
    return receiverId_;
}

void ReceiversContainer::Row::update (const AudioReceivers::ReceiverState& state)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    stream_.audioFormat = state.inputFormat.to_string();
    stream_.packetTimeFrames = "ptime: " + juce::String (state.packetTimeFrames);
    stream_.address = state.session.connection_address.to_string();
    stream_.state = juce::String ("State: ") + rav::rtp_stream_receiver::to_string (state.state);

    if (state.inputFormat.sample_rate != state.outputFormat.sample_rate)
        stream_.warning = "Warning: sample rate mismatch";
    else
        stream_.warning.clear();

    delay_ = state.delaySamples;
    if (!delayEditor_.hasKeyboardFocus (true))
        delayEditor_.setText (juce::String (state.delaySamples));
    repaint();
}

void ReceiversContainer::Row::paint (juce::Graphics& g)
{
    constexpr float fontSize = 14.0f;
    constexpr int rowHeight = 20;

    g.setColour (Constants::Colours::rowBackground);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);

    auto b = getLocalBounds().reduced (kMargin + 5, kMargin);
    auto column1 = b.removeFromLeft (250);
    auto column2 = b.removeFromLeft (200);
    auto column3 = b.removeFromLeft (200);
    auto column4 = b.removeFromLeft (200);

    g.setColour (Constants::Colours::text);
    g.setFont (juce::FontOptions (fontSize, juce::Font::bold));
    g.drawText (stream_.streamName, column1.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("Stats", column2.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("Receive interval", column3.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText ("Settings", column4.removeFromTop (rowHeight), juce::Justification::centredLeft);

    g.setFont (juce::FontOptions (fontSize, juce::Font::plain));
    g.drawText (stream_.audioFormat, column1.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText (stream_.packetTimeFrames, column1.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText (stream_.address, column1.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText (stream_.state, column1.removeFromTop (rowHeight), juce::Justification::centredLeft);

    g.setColour(Constants::Colours::warning);
    g.drawText (stream_.warning, column1.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.setColour (Constants::Colours::text);

    g.drawText (packet_stats_.dropped, column2.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText (packet_stats_.duplicates, column2.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText (packet_stats_.outOfOrder, column2.removeFromTop (rowHeight), juce::Justification::centredLeft);
    g.drawText (packet_stats_.tooLate, column2.removeFromTop (rowHeight), juce::Justification::centredLeft);

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
    delayEditor_.setBounds (b.withLeft (768).withHeight (24).withWidth (60));

    auto bottom = b.removeFromBottom (27);
    deleteButton_.setBounds (bottom.removeFromRight (65));
    bottom.removeFromRight (6);
    showSdpButton_.setBounds (bottom.removeFromRight (89));
}

void ReceiversContainer::Row::timerCallback()
{
    update();
}

void ReceiversContainer::Row::update()
{
    TRACY_ZONE_SCOPED;

    const auto stats = audioReceivers_.getStatisticsForReceiver (receiverId_);

    // Packet stats
    packet_stats_.dropped = "dropped: " + juce::String (stats.packet_stats.dropped);
    packet_stats_.duplicates = "duplicates: " + juce::String (stats.packet_stats.duplicates);
    packet_stats_.outOfOrder = "out of order: " + juce::String (stats.packet_stats.out_of_order);
    packet_stats_.tooLate = "too late: " + juce::String (stats.packet_stats.too_late);

    // Interval stats
    interval_stats_.avg = "avg: " + juce::String (stats.packet_interval_stats.average);
    interval_stats_.median = "median: " + juce::String (stats.packet_interval_stats.median);
    interval_stats_.min = "min: " + juce::String (stats.packet_interval_stats.min);
    interval_stats_.max = "max: " + juce::String (stats.packet_interval_stats.max);
    interval_stats_.stddev = "stddev: " + juce::String (stats.packet_interval_stats.stddev);

    repaint();
}
