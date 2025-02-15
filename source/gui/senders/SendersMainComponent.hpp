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

#include <juce_gui_basics/juce_gui_basics.h>

class SendersMainComponent : public juce::Component {
public:
    explicit SendersMainComponent(ApplicationContext& context);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    ApplicationContext& context_;
};
