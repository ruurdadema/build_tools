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

#include "ravennakit/sdp/sdp_session_description.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

class SdpViewerComponent : public juce::Component
{
public:
    std::function<void (rav::sdp::SessionDescription sdp)> onApply;

    explicit SdpViewerComponent (const std::string& sdpText);
    ~SdpViewerComponent() override;
    void resized() override;

private:
    static constexpr int kMargin = 10;

    juce::TextEditor sdpTextEditor_;
    juce::TextButton closeButton_ { "Close" };
    juce::TextButton applyButton_ { "Apply" };
    juce::TextButton copyButton_ { "Copy" };
    juce::Label errorLabel_ { "error" };
};
