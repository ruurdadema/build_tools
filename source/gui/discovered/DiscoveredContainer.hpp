/*
 * Project: RAVENNAKIT demo application
 * Copyright (c) 2024-2025 Sound on Digital
 *
 * This file is part of the RAVENNAKIT demo application.
 *
 * The RAVENNAKIT demo application is licensed on a proprietary,
 * source-available basis. You are allowed to view, modify, and compile
 * this code for personal or internal evaluation only.
 *
 * You may NOT redistribute this file, any modified versions, or any
 * compiled binaries to third parties, and you may NOT use it for any
 * commercial purpose, except under a separate written agreement with
 * Sound on Digital.
 *
 * For the full license terms, see:
 *   LICENSE.md
 * or visit:
 *   https://ravennakit.com
 */

#pragma once

#include "application/ApplicationContext.hpp"
#include "gui/window/WindowContext.hpp"
#include "ravennakit/ravenna/ravenna_node.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class DiscoveredContainer : public juce::Component, public DiscoveredSessionsModel::Subscriber, public rav::RavennaNode::Subscriber
{
public:
    explicit DiscoveredContainer (ApplicationContext& context, WindowContext& windowContext);
    ~DiscoveredContainer() override;

    void resized() override;
    void resizeToFitContent();

    void onSessionUpdated (const std::string& sessionName, const DiscoveredSessionsModel::SessionState* state) override;
    void onNodeUpdated (const std::string& nodeName, const DiscoveredSessionsModel::NodeState* state) override;

    void ravenna_node_configuration_updated (const rav::RavennaNode::Configuration& configuration) override;

private:
    static constexpr int kRowHeight = 60;
    static constexpr int kMargin = 10;
    static constexpr int kTextHeight = 28;

    class Row : public Component
    {
    public:
        enum class Type
        {
            Session,
            Node
        };

        Row (ApplicationContext& context, WindowContext& windowContext, Type type);

        void update (const DiscoveredSessionsModel::SessionState& sessionState);
        void update (const DiscoveredSessionsModel::NodeState& nodeState);

        [[nodiscard]] Type getType() const
        {
            return type_;
        }

        [[nodiscard]] juce::String getSessionName() const
        {
            return sessionName_.getText();
        }

        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        Type type_;

        juce::Label nameLabel_;
        juce::Label sessionName_;
        juce::Label descriptionLabel_;
        juce::Label description_;
        juce::TextButton createReceiverButton_ { "" };
    };

    ApplicationContext& appContext_;
    WindowContext& windowContext_;
    juce::OwnedArray<Row> rows_;
    juce::Label emptyLabel_;
    juce::Label nodeDiscoveryNotEnabledLabel_;
    juce::HyperlinkButton enableNodeDiscoveryButton_;
    juce::Label sessionDiscoveryNotEnabledLabel_;
    juce::HyperlinkButton enableSessionDiscoveryButton_;
    rav::RavennaNode::Configuration ravenna_node_config_;

    void updateGui();
};
