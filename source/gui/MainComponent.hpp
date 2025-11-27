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
#include "widgets/HorizontalMenu.hpp"
#include "window/WindowContext.hpp"

#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent final : public juce::Component, public WindowContext
{
public:
    explicit MainComponent (ApplicationContext& context);

    void paint (juce::Graphics&) override;
    void resized() override;

    bool navigateTo (const juce::String& path) override;

private:
    class TopRightSection final : public juce::Component
    {
    public:
        explicit TopRightSection (ApplicationContext& context);

        void resized() override;
        void paint (juce::Graphics& g) override;

    private:
        juce::TextButton saveButton_ { "Save" };
        juce::TextButton loadButton_ { "Load" };
        juce::TextButton cloneWindowButton_ { "Clone Window" };
        std::unique_ptr<juce::FileChooser> chooser_;
    };

    ApplicationContext& context_;
    HorizontalMenu menu_;
    TopRightSection topRightSection_;

    std::unique_ptr<Component> content_;

    void showContent (std::unique_ptr<Component> content);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
