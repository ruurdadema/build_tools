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

class DiscoveredStreamsContainer : public juce::Component
{
public:
    DiscoveredStreamsContainer();

    void paint (juce::Graphics& g) override;
    void resized() override;

    void resizeBasedOnContent();

private:
    static constexpr int kRowHeight = 60;
    static constexpr int kMargin = 10;

    class Row : public Component
    {
    public:
        Row(const juce::String& stream_name, const juce::String& stream_description);
        void resized() override;
        void paint (juce::Graphics& g) override;

    private:
        juce::Label streamName_;
        juce::Label streamDescription_;
        juce::TextButton startButton_{"Start"};
    };

    juce::OwnedArray<Row> rows_;
};
