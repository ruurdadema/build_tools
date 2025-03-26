/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "SendersContainer.hpp"

#include "AddNewSenderDialog.hpp"
#include "gui/lookandfeel/Constants.hpp"
#include "gui/lookandfeel/ThisLookAndFeel.hpp"
#include "ravennakit/core/support.hpp"

#include <juce_gui_extra/juce_gui_extra.h>

SendersContainer::SendersContainer (ApplicationContext& context) : context_ (context)
{
    addButton.onClick = [this] {
        auto content = std::make_unique<AddNewSenderDialog>();

        content->setSize (300, 120);
        content->setLookAndFeel (&rav::get_global_instance_of_type<ThisLookAndFeel>());
        content->onAdd ([this] (const juce::String& name) {
            std::ignore = context_.getAudioSenders().createSender (name.toStdString());
        });

        juce::DialogWindow::LaunchOptions options;
        options.dialogTitle = "Add new sender";
        options.content.setOwned (content.release());
        options.dialogBackgroundColour = Constants::Colours::windowBackground;
        options.componentToCentreAround = getTopLevelComponent();
        options.useNativeTitleBar = true;
        options.resizable = false;
        options.launchAsync();
    };
    addAndMakeVisible (addButton);

    if (!context_.getAudioSenders().subscribe (this))
    {
        RAV_ERROR ("Failed to add subscriber");
    }

    resizeToFitContent();
}

SendersContainer::~SendersContainer()
{
    if (!context_.getAudioSenders().unsubscribe (this))
    {
        RAV_ERROR ("Failed to remove subscriber");
    }
}

void SendersContainer::resizeToFitContent()
{
    setSize (getWidth(), rows_.size() * kRowHeight + kMargin + kMargin * rows_.size() + kMargin * 2 + kButtonHeight);
}

void SendersContainer::onAudioSenderUpdated (rav::Id senderId, const AudioSenders::SenderState* state)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (state != nullptr)
    {
        for (const auto& row : rows_)
        {
            if (row->getId() == senderId)
            {
                row->update (*state);
                return;
            }
        }

        auto* row = rows_.add (std::make_unique<Row> (context_.getAudioSenders(), senderId, state->sessionName));
        RAV_ASSERT (row != nullptr, "Failed to create row");
        row->update (*state);
        addAndMakeVisible (row);
        resizeToFitContent();
    }
    else
    {
        for (auto i = 0; i < rows_.size(); ++i)
        {
            if (rows_.getUnchecked (i)->getId() == senderId)
            {
                rows_.remove (i);
                resizeToFitContent();
                return;
            }
        }
    }
}

void SendersContainer::resized()
{
    auto b = getLocalBounds().reduced (kMargin);
    for (auto i = 0; i < rows_.size(); ++i)
    {
        rows_.getUnchecked (i)->setBounds (b.removeFromTop (kRowHeight));
        b.removeFromTop (kMargin);
    }

    b.removeFromTop (kMargin * 2);
    addButton.setBounds (b.withSizeKeepingCentre (200, kButtonHeight));
}

SendersContainer::Row::Row (AudioSenders& audioSenders, const rav::Id senderId, const std::string& name) :
    audioSenders_ (audioSenders),
    senderId_ (senderId)
{
    sessionNameLabel_.setText ("Session name:", juce::dontSendNotification);
    sessionNameLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (sessionNameLabel_);

    sessionNameEditor_.setIndents (10, 10);
    addAndMakeVisible (sessionNameEditor_);

    startStopButton_.setClickingTogglesState (true);
    startStopButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::green);
    startStopButton_.setColour (juce::TextButton::ColourIds::buttonOnColourId, Constants::Colours::red);
    startStopButton_.onClick = [] {
    };
    addAndMakeVisible (startStopButton_);

    deleteButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::red);
    deleteButton_.onClick = [this] {
        audioSenders_.removeSender (senderId_);
    };
    addAndMakeVisible (deleteButton_);
}

rav::Id SendersContainer::Row::getId() const
{
    return senderId_;
}

void SendersContainer::Row::update (const AudioSenders::SenderState& state)
{
    startStopButton_.setToggleState (state.active, juce::dontSendNotification);
    startStopButton_.setButtonText (state.active ? "Stop" : "Start");
}

void SendersContainer::Row::paint (juce::Graphics& g)
{
    g.setColour (Constants::Colours::rowBackground);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);
}

void SendersContainer::Row::resized()
{
    auto b = getLocalBounds().reduced (kMargin);

    auto buttons = b.withTrimmedTop (b.getHeight() - kButtonHeight);
    deleteButton_.setBounds (buttons.removeFromRight (65));
    buttons.removeFromRight (kMargin);
    startStopButton_.setBounds (buttons.removeFromRight (65));

    auto topRow = b.removeFromTop (20);
    b.removeFromTop (kMargin / 2);
    auto bottomRow = b;
    sessionNameLabel_.setBounds (topRow.removeFromLeft (200));
    sessionNameEditor_.setBounds (bottomRow.removeFromLeft (200));
}
