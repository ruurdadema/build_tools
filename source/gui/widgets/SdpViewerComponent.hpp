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
