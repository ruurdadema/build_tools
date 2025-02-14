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

#include "widgets/HorizontalMenu.hpp"

#include <juce_gui_extra/juce_gui_extra.h>

class MainComponent final : public juce::Component
{
public:
    MainComponent();

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HorizontalMenu menu_;
    std::unique_ptr<Component> content_;

    void showContent (std::unique_ptr<Component> content);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
