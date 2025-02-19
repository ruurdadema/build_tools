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
#include "ravennakit/ravenna/ravenna_node.hpp"
#include "util/MessageThreadExecutor.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class ReceiversContainer : public juce::Component, public rav::ravenna_node::subscriber
{
public:
    ReceiversContainer (ApplicationContext& context);
    ~ReceiversContainer() override;

    void resized() override;

    void resizeBasedOnContent();

    // rav::ravenna_node::subscriber overrides
    void on_receiver_updated (rav::id receiver_id) override;

private:
    static constexpr int kRowHeight = 138;
    static constexpr int kMargin = 10;

    ApplicationContext& context_;

    class Row : public Component, public juce::Timer
    {
    public:
        explicit Row (rav::ravenna_node& node, rav::id receiverId);
        ~Row() override;

        rav::id getId() const;

        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        rav::ravenna_node& node_;
        juce::TextEditor delayEditor_;
        rav::id receiverId_;
        struct
        {
            juce::String dropped;
            juce::String duplicates;
            juce::String out_of_order;
            juce::String too_late;

        } packet_stats_;

        void timerCallback() override;
        void update();
    };

    juce::OwnedArray<Row> rows_;
    MessageThreadExecutor executor_;
};
