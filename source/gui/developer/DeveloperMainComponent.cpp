/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "DeveloperMainComponent.hpp"

#include "application/Logging.hpp"
#include "gui/lookandfeel/Constants.hpp"

#include <csignal>

DeveloperMainComponent::DeveloperMainComponent()
{
    crashLabel_.setText ("Crash the application", juce::dontSendNotification);
    addAndMakeVisible (crashLabel_);

    crashByAbort_.setButtonText ("Abort");
    crashByAbort_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::red);
    crashByAbort_.onClick = [] {
        abort();
    };
    addAndMakeVisible (crashByAbort_);

    crashBySigsegv_.setButtonText ("SIGSEGV");
    crashBySigsegv_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::red);
    crashBySigsegv_.onClick = [] {
        raise (SIGSEGV);
    };
    addAndMakeVisible (crashBySigsegv_);

    crashByStackOverflow_.setButtonText ("StackOverflow");
    crashByStackOverflow_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::red);
    crashByStackOverflow_.onClick = [] {
        for (long long int i = 0; ++i; (&i)[i] = i)
        {
        }
    };
    addAndMakeVisible (crashByStackOverflow_);

    crashByException_.setButtonText ("Exception");
    crashByException_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::red);
    crashByException_.onClick = [] {
        throw std::runtime_error ("Exception has occurred");
    };
    addAndMakeVisible (crashByException_);
}

void DeveloperMainComponent::resized()
{
    auto b = getLocalBounds().reduced (10);
    crashLabel_.setBounds (b.removeFromTop (28));

    b.removeFromTop (10);

    constexpr auto w = 120;
    constexpr auto h = 28;
    auto row = b.removeFromTop (h);
    row.removeFromLeft (10);
    crashByAbort_.setBounds (row.removeFromLeft (w));
    row.removeFromLeft (10);
    crashBySigsegv_.setBounds (row.removeFromLeft (w));
    row.removeFromLeft (10);
    crashByStackOverflow_.setBounds (row.removeFromLeft (w));
    row.removeFromLeft (10);
    crashByException_.setBounds (row.removeFromLeft (w));
    row.removeFromLeft (10);
}
