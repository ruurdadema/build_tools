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

#include "ravennakit/ravenna/ravenna_node.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class StreamsContainer : public juce::Component, public rav::ravenna_node::subscriber
{
public:
    StreamsContainer();
    void resized() override;

    void resizeBasedOnContent();

    // rav::ravenna_node::subscriber overrides
    void on_receiver_updated (rav::id receiver_id) override;

private:
    static constexpr int kRowHeight = 138;
    static constexpr int kMargin = 10;

    class Row : public Component
    {
    public:
        explicit Row (rav::id receiverId);

        rav::id getId() const;

        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        juce::TextEditor delayEditor_;
        rav::id receiverId_;
    };
    juce::OwnedArray<Row> rows_;
};
