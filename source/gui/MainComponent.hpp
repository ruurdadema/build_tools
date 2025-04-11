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
#include "widgets/HorizontalMenu.hpp"
#include "window/WindowContext.hpp"

#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent final : public juce::Component, public WindowContext
{
public:
    explicit MainComponent(ApplicationContext& context);

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
        juce::TextButton saveButton_ {"Save"};
        juce::TextButton loadButton_ {"Load"};
        juce::TextButton cloneWindowButton_ {"Clone Window"};
        std::unique_ptr<juce::FileChooser> chooser_;
    };

    ApplicationContext& context_;
    HorizontalMenu menu_;
    TopRightSection topRightSection_;

    std::unique_ptr<Component> content_;

    void showContent (std::unique_ptr<Component> content);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
