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
    emptyLabel_.setText ("No discovered RAVENNA sessions.", juce::dontSendNotification);
    emptyLabel_.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (emptyLabel_);

    nodeDiscoveryNotEnabledLabel_.setText ("Node discovery is not enabled", juce::dontSendNotification);
    nodeDiscoveryNotEnabledLabel_.setColour (juce::Label::textColourId, juce::Colours::grey);
    addChildComponent (nodeDiscoveryNotEnabledLabel_);

    enableNodeDiscoveryButton_.setButtonText ("enable");
    enableNodeDiscoveryButton_.setFont (juce::FontOptions (15.f), false, juce::Justification::left);
    enableNodeDiscoveryButton_.onClick = [this] {
        auto config = ravenna_node_config_;
        config.enable_dnssd_node_discovery = true;
        appContext_.getRavennaNode().set_configuration (config).wait();
    };
    addChildComponent (enableNodeDiscoveryButton_);

    sessionDiscoveryNotEnabledLabel_.setText ("Session discovery is not enabled", juce::dontSendNotification);
    sessionDiscoveryNotEnabledLabel_.setColour (juce::Label::textColourId, juce::Colours::grey);
    addChildComponent (sessionDiscoveryNotEnabledLabel_);

    enableSessionDiscoveryButton_.setButtonText ("enable");
    enableSessionDiscoveryButton_.setFont (juce::FontOptions (15.f), false, juce::Justification::left);
    enableSessionDiscoveryButton_.onClick = [this] {
        auto config = ravenna_node_config_;
        config.enable_dnssd_session_discovery = true;
        appContext_.getRavennaNode().set_configuration (config).wait();
    };
    addChildComponent (enableSessionDiscoveryButton_);

    updateGui();

    appContext_.getSessions().addSubscriber (this);
    appContext_.getRavennaNode().subscribe (this).wait();
}

DiscoveredContainer::~DiscoveredContainer()
{
    appContext_.getRavennaNode().unsubscribe (this).wait();
    appContext_.getSessions().removeSubscriber (this);
}

void DiscoveredContainer::resized()
{
    auto b = getLocalBounds().reduced (kMargin);

    bool addMargin = false;
    if (ravenna_node_config_.enable_dnssd_node_discovery && ravenna_node_config_.enable_dnssd_session_discovery && rows_.isEmpty())
    {
        emptyLabel_.setBounds (b.removeFromTop (kTextHeight));
        addMargin = true;
    }

    if (!ravenna_node_config_.enable_dnssd_node_discovery)
    {
        auto row = b.removeFromTop (kTextHeight);
        nodeDiscoveryNotEnabledLabel_.setBounds (row.removeFromLeft (197));
        enableNodeDiscoveryButton_.setBounds (row.withWidth (50));
        addMargin = true;
    }

    if (!ravenna_node_config_.enable_dnssd_session_discovery)
    {
        auto row = b.removeFromTop (kTextHeight);
        sessionDiscoveryNotEnabledLabel_.setBounds (row.removeFromLeft (211));
        enableSessionDiscoveryButton_.setBounds (row.withWidth (50));
        addMargin = true;
    }

    if (addMargin)
        b.removeFromTop (kMargin);

    for (auto i = 0; i < rows_.size(); ++i)
    {
        rows_.getUnchecked (i)->setBounds (b.removeFromTop (kRowHeight));
        b.removeFromTop (kMargin);
    }
}

void DiscoveredContainer::resizeToFitContent()
{
    auto calculateHeight = rows_.size() * kRowHeight + kMargin + kMargin * rows_.size();

    bool addMargin = false;
    if (ravenna_node_config_.enable_dnssd_node_discovery && ravenna_node_config_.enable_dnssd_session_discovery && rows_.isEmpty())
    {
        calculateHeight += kTextHeight;
        addMargin = true;
    }

    if (!ravenna_node_config_.enable_dnssd_node_discovery)
    {
        calculateHeight += kTextHeight;
        addMargin = true;
    }

    if (!ravenna_node_config_.enable_dnssd_session_discovery)
    {
        calculateHeight += kTextHeight;
        addMargin = true;
    }

    if (addMargin)
        calculateHeight += kMargin;

    setSize (getWidth(), std::max (calculateHeight, 100)); // Min to leave space for the empty label
    resized();
}

void DiscoveredContainer::onSessionUpdated (const std::string& sessionName, const DiscoveredSessionsModel::SessionState* state)
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
        updateGui();
    }
    else
    {
        for (auto i = 0; i < rows_.size(); ++i)
        {
            if (rows_.getUnchecked (i)->getSessionName() == juce::StringRef (sessionName))
            {
                rows_.remove (i);
                updateGui();
                return;
            }
        }
    }
}

void DiscoveredContainer::onNodeUpdated (const std::string& nodeName, const DiscoveredSessionsModel::NodeState* state)
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
        updateGui();
    }
    else
    {
        for (auto i = 0; i < rows_.size(); ++i)
        {
            if (rows_.getUnchecked (i)->getSessionName() == juce::StringRef (nodeName))
            {
                rows_.remove (i);
                updateGui();
                return;
            }
        }
    }
}

void DiscoveredContainer::ravenna_node_configuration_updated (const rav::RavennaNode::Configuration& configuration)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (appContext_.getRavennaNode());

    SafePointer weakThis (this);
    juce::MessageManager::callAsync ([weakThis, configuration] {
        if (!weakThis)
            return;
        weakThis->ravenna_node_config_ = configuration;
        weakThis->updateGui();
    });
}

DiscoveredContainer::Row::Row (ApplicationContext& context, WindowContext& windowContext, const Type type) : type_ (type)
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
        auto config = rav::RavennaReceiver::Configuration::default_config();
        config.session_name = getSessionName().toStdString();
        const auto result = context.getAudioReceivers().createReceiver (std::move (config));
        if (!result)
        {
            RAV_LOG_ERROR ("Failed to create receiver: {}", result.error());
            return;
        }
        RAV_LOG_TRACE ("Created receiver with id: {}", result->value());
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

    auto leftColumn = b.removeFromLeft (61);
    nameLabel_.setBounds (leftColumn.removeFromTop (20));
    descriptionLabel_.setBounds (leftColumn);

    sessionName_.setBounds (b.removeFromTop (20));
    description_.setBounds (b);
}

void DiscoveredContainer::updateGui()
{
    emptyLabel_.setVisible (
        ravenna_node_config_.enable_dnssd_node_discovery && ravenna_node_config_.enable_dnssd_session_discovery && rows_.isEmpty());

    nodeDiscoveryNotEnabledLabel_.setVisible (!ravenna_node_config_.enable_dnssd_node_discovery);
    enableNodeDiscoveryButton_.setVisible (!ravenna_node_config_.enable_dnssd_node_discovery);

    sessionDiscoveryNotEnabledLabel_.setVisible (!ravenna_node_config_.enable_dnssd_session_discovery);
    enableSessionDiscoveryButton_.setVisible (!ravenna_node_config_.enable_dnssd_session_discovery);

    resizeToFitContent();
}

void DiscoveredContainer::Row::update (const DiscoveredSessionsModel::SessionState& sessionState)
{
    nameLabel_.setText ("Session: ", juce::dontSendNotification);
    sessionName_.setText (sessionState.name, juce::dontSendNotification);
    description_.setText (sessionState.host, juce::dontSendNotification);
    createReceiverButton_.setVisible (true);
}

void DiscoveredContainer::Row::update (const DiscoveredSessionsModel::NodeState& nodeState)
{
    nameLabel_.setText ("Node: ", juce::dontSendNotification);
    sessionName_.setText (nodeState.name, juce::dontSendNotification);
    description_.setText (nodeState.host, juce::dontSendNotification);
    createReceiverButton_.setVisible (false);
}
