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
    emptyLabel_.setText ("Create a receiver on the \"Discovered\" page.", juce::dontSendNotification);
    emptyLabel_.setColour (juce::Label::textColourId, juce::Colours::grey);
    emptyLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (emptyLabel_);

    if (!context_.getAudioReceivers().subscribe (this))
    {
        RAV_ERROR ("Failed to add subscriber");
    }
}

ReceiversContainer::~ReceiversContainer()
{
    if (!context_.getAudioReceivers().unsubscribe (this))
    {
        RAV_ERROR ("Failed to remove subscriber");
    }
}

void ReceiversContainer::resized()
{
    auto b = getLocalBounds().reduced (kMargin);

    emptyLabel_.setBounds (b.reduced (kMargin));

    for (auto i = 0; i < rows_.size(); ++i)
    {
        rows_.getUnchecked (i)->setBounds (b.removeFromTop (kRowHeight));
        b.removeFromTop (kMargin);
    }
}

void ReceiversContainer::resizeToFitContent()
{
    const auto calculatedHeight = rows_.size() * kRowHeight + kMargin + kMargin * rows_.size();
    setSize (getWidth(), std::max (calculatedHeight, 100)); // Min to leave space for the empty label
}

void ReceiversContainer::onAudioReceiverUpdated (rav::Id receiverId, const AudioReceivers::ReceiverState* state)
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

        auto* row = rows_.add (std::make_unique<Row> (context_.getAudioReceivers(), receiverId));
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

ReceiversContainer::SessionInfoComponent::SessionInfoComponent (const juce::String& title)
{
    titleLabel_.setText (title, juce::dontSendNotification);
    titleLabel_.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel_);

    sessionInfoLabel_.setText ("Session: ", juce::dontSendNotification);
    addAndMakeVisible (sessionInfoLabel_);

    packetTimeLabel_.setText ("Packet time: ", juce::dontSendNotification);
    addAndMakeVisible (packetTimeLabel_);

    statusLabel_.setText ("Status: ", juce::dontSendNotification);
    addAndMakeVisible (statusLabel_);
}

void ReceiversContainer::SessionInfoComponent::update (const AudioReceivers::StreamState* state)
{
    if (state == nullptr)
        return;

    sessionInfoLabel_.setText ("Session: " + state->stream.session.to_string(), juce::dontSendNotification);
    packetTimeLabel_.setText (
        "Packet time: " + juce::String (state->stream.packet_time_frames),
        juce::dontSendNotification);
    statusLabel_.setText (
        juce::String ("Status: ") + rav::rtp::AudioReceiver::to_string (state->state),
        juce::dontSendNotification);
}

void ReceiversContainer::SessionInfoComponent::resized()
{
    auto b = getLocalBounds();
    titleLabel_.setBounds (b.removeFromTop (20));
    sessionInfoLabel_.setBounds (b.removeFromTop (20));
    packetTimeLabel_.setBounds (b.removeFromTop (20));
    statusLabel_.setBounds (b.removeFromTop (20));
}

ReceiversContainer::PacketStatsComponent::PacketStatsComponent()
{
    titleLabel_.setText ("Packet Stats", juce::dontSendNotification);
    titleLabel_.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (titleLabel_);

    addAndMakeVisible (jitterMaxLabel_);
    addAndMakeVisible (droppedLabel_);
    addAndMakeVisible (duplicatesLabel_);
    addAndMakeVisible (outOfOrderLabel_);
    addAndMakeVisible (tooLateLabel_);
}

void ReceiversContainer::PacketStatsComponent::update (const rav::rtp::AudioReceiver::SessionStats* stats)
{
    if (stats == nullptr)
        return;

    jitterMaxLabel_.setText (
        "Max interval (ms): " + juce::String (stats->packet_interval_stats.max),
        juce::dontSendNotification);
    droppedLabel_.setText ("Dropped: " + juce::String (stats->packet_stats.dropped), juce::dontSendNotification);
    duplicatesLabel_.setText (
        "Duplicates: " + juce::String (stats->packet_stats.duplicates),
        juce::dontSendNotification);
    outOfOrderLabel_.setText (
        "Out of Order: " + juce::String (stats->packet_stats.out_of_order),
        juce::dontSendNotification);
    tooLateLabel_.setText ("Too Late: " + juce::String (stats->packet_stats.too_late), juce::dontSendNotification);
}

void ReceiversContainer::PacketStatsComponent::resized()
{
    auto b = getLocalBounds();
    titleLabel_.setBounds (b.removeFromTop (20));
    jitterMaxLabel_.setBounds (b.removeFromTop (20));
    droppedLabel_.setBounds (b.removeFromTop (20));
    duplicatesLabel_.setBounds (b.removeFromTop (20));
    outOfOrderLabel_.setBounds (b.removeFromTop (20));
    tooLateLabel_.setBounds (b.removeFromTop (20));
}

ReceiversContainer::Row::Row (AudioReceivers& audioReceivers, const rav::Id receiverId) :
    audioReceivers_ (audioReceivers),
    receiverId_ (receiverId)
{
    constexpr float fontSize = 14.0f;

    sessionNameLabel_.setFont (juce::FontOptions (fontSize, juce::Font::bold));
    addAndMakeVisible (sessionNameLabel_);

    addAndMakeVisible (audioFormatLabel_);

    settingsLabel_.setFont (juce::FontOptions (fontSize, juce::Font::bold));
    settingsLabel_.setText ("Settings", juce::dontSendNotification);
    addAndMakeVisible (settingsLabel_);

    delaySettingLabel_.setText ("Delay (samples)", juce::dontSendNotification);
    addAndMakeVisible (delaySettingLabel_);

    delayEditor_.setInputRestrictions (10, "0123456789");
    delayEditor_.onReturnKey = [this] {
        const auto value = static_cast<uint32_t> (delayEditor_.getText().getIntValue());
        rav::RavennaReceiver::ConfigurationUpdate update;
        update.delay_frames = value;
        audioReceivers_.updateReceiverConfiguration (receiverId_, std::move (update));
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

    onOffButton_.setButtonText ("On");
    onOffButton_.setClickingTogglesState (true);
    onOffButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::grey);
    onOffButton_.setColour (juce::TextButton::ColourIds::buttonOnColourId, Constants::Colours::green);
    onOffButton_.onClick = [this] {
        rav::RavennaReceiver::ConfigurationUpdate update;
        update.enabled = onOffButton_.getToggleState();
        audioReceivers_.updateReceiverConfiguration (receiverId_, std::move (update));
    };
    addAndMakeVisible (onOffButton_);

    deleteButton_.setButtonText ("Delete");
    deleteButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::red);
    deleteButton_.onClick = [this] {
        audioReceivers_.removeReceiver (receiverId_);
    };
    addAndMakeVisible (deleteButton_);

    addAndMakeVisible (primarySessionInfo_);
    addAndMakeVisible (primaryPacketStats_);

    addAndMakeVisible (secondarySessionInfo_);
    addAndMakeVisible (secondaryPacketStats_);

    warningLabel_.setColour (juce::Label::ColourIds::textColourId, Constants::Colours::warning);
    addAndMakeVisible (warningLabel_);

    update();
    startTimer (1000);
}

rav::Id ReceiversContainer::Row::getId() const
{
    return receiverId_;
}

void ReceiversContainer::Row::update (const AudioReceivers::ReceiverState& state)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    sessionNameLabel_.setText (state.configuration.session_name, juce::dontSendNotification);

    if (state.inputFormat.is_valid() && state.inputFormat.sample_rate != state.outputFormat.sample_rate)
        warningLabel_.setText ("Warning: sample rate mismatch", juce::dontSendNotification);
    else
        warningLabel_.setText ({}, juce::dontSendNotification);

    const auto* pri = state.find_stream_for_rank (rav::Rank::primary());
    const auto* sec = state.find_stream_for_rank (rav::Rank::secondary());

    audioFormatLabel_.setText (state.inputFormat.to_string(), juce::dontSendNotification);

    primarySessionInfo_.update (pri);
    secondarySessionInfo_.update (sec);

    secondarySessionInfo_.setVisible (sec != nullptr);
    secondaryPacketStats_.setVisible (sec != nullptr);

    delay_ = state.configuration.delay_frames;

    if (!delayEditor_.hasKeyboardFocus (true))
        delayEditor_.setText (juce::String (state.configuration.delay_frames));

    onOffButton_.setToggleState (state.configuration.enabled, juce::dontSendNotification);
    onOffButton_.setButtonText (state.configuration.enabled ? "On" : "Off");

    repaint();
}

void ReceiversContainer::Row::paint (juce::Graphics& g)
{
    g.setColour (Constants::Colours::rowBackground);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);
}

void ReceiversContainer::Row::resized()
{
    constexpr int rowHeight = 20;
    constexpr int buttonHeight = 28;

    auto b = getLocalBounds().reduced (kMargin);

    auto buttonBounds = b.removeFromRight (89);
    onOffButton_.setBounds (buttonBounds.removeFromTop (buttonHeight));
    buttonBounds.removeFromTop (6);
    deleteButton_.setBounds (buttonBounds.removeFromTop (buttonHeight));
    buttonBounds.removeFromTop (6);
    showSdpButton_.setBounds (buttonBounds.removeFromTop (buttonHeight));

    constexpr int kColumnWidth = 250;

    auto leftSection = b.removeFromLeft (kColumnWidth * 2);
    auto rightSection = b;

    warningLabel_.setBounds (rightSection.removeFromBottom (16).withTrimmedLeft (1));

    auto column1 = leftSection.removeFromLeft (kColumnWidth);
    auto column2 = leftSection.removeFromLeft (kColumnWidth);
    auto column3 = rightSection.removeFromLeft (kColumnWidth);
    auto column4 = rightSection.removeFromLeft (kColumnWidth);

    sessionNameLabel_.setBounds (column1.removeFromTop (rowHeight));
    audioFormatLabel_.setBounds (column1.removeFromTop (rowHeight));

    settingsLabel_.setBounds (column2.removeFromTop (rowHeight));
    auto delayBounds = column2.removeFromTop (rowHeight);
    delaySettingLabel_.setBounds (delayBounds.removeFromLeft (115));
    delayEditor_.setBounds (delayBounds.withHeight (24).withWidth (60).translated (0, -1));

    column1.removeFromTop (17);
    column2.removeFromTop (17);

    primarySessionInfo_.setBounds (column1.removeFromTop (rowHeight * 4));
    secondarySessionInfo_.setBounds (column2.removeFromTop (rowHeight * 4));

    primaryPacketStats_.setBounds (column3.removeFromTop (rowHeight * 6));
    secondaryPacketStats_.setBounds (column4.removeFromTop (rowHeight * 6));
}

void ReceiversContainer::Row::timerCallback()
{
    update();
}

void ReceiversContainer::Row::update()
{
    TRACY_ZONE_SCOPED;

    const auto stats = audioReceivers_.getStatisticsForReceiver (receiverId_);

    primaryPacketStats_.update (&stats);
    secondaryPacketStats_.update (&stats);

    repaint();
}

void ReceiversContainer::updateGuiState()
{
    emptyLabel_.setVisible (rows_.isEmpty());
}
