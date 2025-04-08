/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "DiscoveredContainer.hpp"

#include "gui/lookandfeel/Constants.hpp"
#include <juce_gui_extra/juce_gui_extra.h>

DiscoveredContainer::DiscoveredContainer (ApplicationContext& context, WindowContext& windowContext) :
    appContext_ (context),
    windowContext_ (windowContext)
{
    appContext_.getSessions().addSubscriber (this);
}

DiscoveredContainer::~DiscoveredContainer()
{
    appContext_.getSessions().removeSubscriber (this);
}

void DiscoveredContainer::resized()
{
    auto b = getLocalBounds().reduced (kMargin);
    for (auto i = 0; i < rows_.size(); ++i)
    {
        rows_.getUnchecked (i)->setBounds (b.removeFromTop (kRowHeight));
        b.removeFromTop (kMargin);
    }
}

void DiscoveredContainer::resizeToFitContent()
{
    setSize (getWidth(), rows_.size() * kRowHeight + kMargin + kMargin * rows_.size());
}

void DiscoveredContainer::onSessionUpdated (const std::string& sessionName, const RavennaSessions::SessionState* state)
{
    if (state != nullptr)
    {
        for (auto* row : rows_)
        {
            if (row->getSessionName() == juce::StringRef (state->name))
            {
                row->update (*state);
                return;
            }
        }

        auto* row = rows_.add (std::make_unique<Row> (appContext_, windowContext_, Row::Type::Session));
        RAV_ASSERT (row != nullptr, "Failed to create row");
        row->update (*state);
        addAndMakeVisible (row);
        resizeToFitContent();
    }
    else
    {
        for (auto i = 0; i < rows_.size(); ++i)
        {
            if (rows_.getUnchecked (i)->getSessionName() == juce::StringRef (sessionName))
            {
                rows_.remove (i);
                resizeToFitContent();
                return;
            }
        }
    }
}

void DiscoveredContainer::onNodeUpdated (const std::string& nodeName, const RavennaSessions::NodeState* state)
{
    if (state != nullptr)
    {
        for (auto* row : rows_)
        {
            if (row->getSessionName() == juce::StringRef (state->name))
            {
                row->update (*state);
                return;
            }
        }

        auto* row = rows_.add (std::make_unique<Row> (appContext_, windowContext_, Row::Type::Node));
        RAV_ASSERT (row != nullptr, "Failed to create row");
        row->update (*state);
        addAndMakeVisible (row);
        resizeToFitContent();
    }
    else
    {
        for (auto i = 0; i < rows_.size(); ++i)
        {
            if (rows_.getUnchecked (i)->getSessionName() == juce::StringRef (nodeName))
            {
                rows_.remove (i);
                resizeToFitContent();
                return;
            }
        }
    }
}

DiscoveredContainer::Row::Row (ApplicationContext& context, WindowContext& windowContext, const Type type) :
    type_ (type)
{
    nameLabel_.setText (type == Type::Session ? "Session" : "Node", juce::dontSendNotification);
    nameLabel_.setJustificationType (juce::Justification::topRight);
    addAndMakeVisible (nameLabel_);

    sessionName_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (sessionName_);

    descriptionLabel_.setText ("Host:", juce::dontSendNotification);
    descriptionLabel_.setJustificationType (juce::Justification::topRight);
    addAndMakeVisible (descriptionLabel_);

    description_.setJustificationType (juce::Justification::topLeft);
    description_.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (description_);

    createReceiverButton_.setButtonText ("Create Receiver");
    createReceiverButton_.onClick = [this, &context, &windowContext] {
        const auto id = context.getAudioReceivers().createReceiver (getSessionName().toStdString());
        RAV_TRACE ("Created receiver with id: {}", id.value());
        windowContext.navigateTo ("receivers");
    };
    addAndMakeVisible (createReceiverButton_);
}

void DiscoveredContainer::Row::paint (juce::Graphics& g)
{
    g.setColour (Constants::Colours::rowBackground);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);
}

void DiscoveredContainer::Row::resized()
{
    auto b = getLocalBounds().reduced (kMargin);

    if (createReceiverButton_.isVisible())
        createReceiverButton_.setBounds (b.removeFromRight (152));

    auto leftColumn = b.removeFromLeft (100);
    nameLabel_.setBounds (leftColumn.removeFromTop (20));
    descriptionLabel_.setBounds (leftColumn);

    sessionName_.setBounds (b.removeFromTop (20));
    description_.setBounds (b);
}

void DiscoveredContainer::Row::update (const RavennaSessions::SessionState& sessionState)
{
    nameLabel_.setText ("Session name: ", juce::dontSendNotification);
    sessionName_.setText (sessionState.name, juce::dontSendNotification);
    description_.setText (sessionState.host, juce::dontSendNotification);
    createReceiverButton_.setVisible (true);
}

void DiscoveredContainer::Row::update (const RavennaSessions::NodeState& nodeState)
{
    nameLabel_.setText ("Node name: ", juce::dontSendNotification);
    sessionName_.setText (nodeState.name, juce::dontSendNotification);
    description_.setText (nodeState.host, juce::dontSendNotification);
    createReceiverButton_.setVisible (false);
}
