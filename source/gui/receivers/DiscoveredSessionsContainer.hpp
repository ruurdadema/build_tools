/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#pragma once

#include "application/ApplicationContext.hpp"
#include "util/MessageThreadExecutor.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class DiscoveredSessionsContainer : public juce::Component, public RavennaSessions::Subscriber
{
public:
    explicit DiscoveredSessionsContainer (ApplicationContext& context);
    ~DiscoveredSessionsContainer() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void resizeBasedOnContent();

    // Sessions::Subscriber overrides
    void onSessionUpdated (const std::string& sessionName, const RavennaSessions::SessionState* state) override;

private:
    static constexpr int kRowHeight = 60;
    static constexpr int kMargin = 10;

    ApplicationContext& context_;

    class Row : public Component
    {
    public:
        explicit Row (ApplicationContext& context, const RavennaSessions::SessionState& session);

        juce::String getSessionName() const;

        void update (const RavennaSessions::SessionState& sessionState);
        void resized() override;
        void paint (juce::Graphics& g) override;

    private:
        juce::Label sessionName_;
        juce::Label description_;
        juce::TextButton startButton_{""};
    };

    juce::OwnedArray<Row> rows_;
    MessageThreadExecutor executor_;
};
