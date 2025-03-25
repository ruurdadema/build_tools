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
        addNewSenderDialog_ = std::make_unique<AddNewSenderDialog>();

        addNewSenderDialog_->setSize (300, 120);
        addNewSenderDialog_->setLookAndFeel (&rav::get_global_instance_of_type<ThisLookAndFeel>());

        juce::DialogWindow::LaunchOptions options;
        options.dialogTitle = "Add new sender";
        options.content.setOwned (addNewSenderDialog_.get());
        options.dialogBackgroundColour = Constants::Colours::windowBackground;
        options.componentToCentreAround = getTopLevelComponent();
        options.useNativeTitleBar = true;
        options.resizable = false;
        auto* dialog = options.launchAsync();

        addNewSenderDialog_->onCancel ([this, dialog] {
            dialog->exitModalState (0);
            addNewSenderDialog_.reset();
        });

        addNewSenderDialog_->onAdd ([this, dialog] (const juce::String& name) {
            std::ignore = context_.getAudioSenders().createSender (name.toStdString());
            dialog->exitModalState (0);
            addNewSenderDialog_.reset();
        });
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
    sessionNameLabel_.setText (name, juce::dontSendNotification);
    addAndMakeVisible (sessionNameLabel_);

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
    std::ignore = state;
}

void SendersContainer::Row::paint (juce::Graphics& g)
{
    g.setColour (Constants::Colours::rowBackground);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);
}

void SendersContainer::Row::resized()
{
    auto b = getLocalBounds().reduced (kMargin);
    b.removeFromTop (18);

    sessionNameLabel_.setBounds (b.removeFromTop (20));

    auto bottom = b.removeFromBottom (27);
    deleteButton_.setBounds (bottom.removeFromRight (65));
}
