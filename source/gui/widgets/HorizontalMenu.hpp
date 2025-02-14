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

#include <utility>

class HorizontalMenu : public juce::Component
{
public:
    HorizontalMenu();

    void addItem (const juce::String& name, const juce::String& path, std::function<void()> onClick);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class MenuItem : public Component
    {
    public:
        MenuItem (const juce::String& name, const juce::String& path, std::function<void()> onClick);

        void triggerClick();
        void setActive (bool active);
        const juce::String& getPath() const;
        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        bool active_ = false;
        juce::String path_;
        juce::HyperlinkButton button_;
    };

    juce::OwnedArray<MenuItem> buttons_;
};
