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

#include <juce_gui_basics/juce_gui_basics.h>

#include <utility>

class HorizontalMenu : public juce::Component
{
public:
    HorizontalMenu();

    void addItem (const juce::String& name, const juce::String& path, std::function<void()> onClick);

    bool navigateTo (const juce::String& path);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class MenuItem : public Component
    {
    public:
        MenuItem (const juce::String& name, const juce::String& path, std::function<void()> onClick);

        float getIdealWidth() const;
        void triggerClick();
        void setActive (bool active);
        const juce::String& getPath() const;
        void paint (juce::Graphics& g) override;
        void resized() override;

    private:
        juce::FontOptions font_ { 16.f };
        bool active_ = false;
        juce::String path_;
        juce::HyperlinkButton button_;
    };

    juce::OwnedArray<MenuItem> buttons_;
};
