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

#include <juce_gui_basics/juce_gui_basics.h>

class StreamsContainer : public juce::Component
{
public:
    StreamsContainer();
    void resized() override;

    void resizeBasedOnContent();

private:
    static constexpr int kRowHeight = 138;
    static constexpr int kMargin = 10;

    class Row : public Component
    {
    public:
        Row();
        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        juce::TextEditor delayEditor_;
    };
    juce::OwnedArray<Row> rows_;
};