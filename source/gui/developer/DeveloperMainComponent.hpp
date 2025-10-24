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

class DeveloperMainComponent : public juce::Component
{
public:
    DeveloperMainComponent();

    void resized() override;

private:
    juce::Label crashLabel_;

    juce::TextButton crashByAbort_;
    juce::TextButton crashBySigsegv_;
    juce::TextButton crashByStackOverflow_;
    juce::TextButton crashByException_;

    juce::Label loggingLabel_;
    juce::TextButton revealLogs_;
};
