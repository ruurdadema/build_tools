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

class PtpMainComponent : public juce::Component, public rav::ptp::Instance::Subscriber, juce::Timer
{
public:
    explicit PtpMainComponent (ApplicationContext& context);
    ~PtpMainComponent() override;

    void resized() override;

    void ptp_parent_changed (const rav::ptp::ParentDs& parent) override;
    void ptp_port_changed_state (const rav::ptp::Port& port) override;
    void ptp_port_removed (uint16_t portNumber) override;
    void ptp_configuration_updated (const rav::ptp::Instance::Configuration& config) override;
    void ptp_stats_updated (const rav::ptp::Stats& ptp_stats) override;

private:
    rav::RavennaNode& ravenna_node_;
    rav::ptp::Instance::Configuration ptp_config_;

    juce::Label domainTitle_;
    juce::TextEditor domainTextEditor_;

    juce::Label grandmasterIdTitle_;
    juce::Label grandmasterIdValue_;

    juce::Label parentPortIdTitle_;
    juce::Label parentPortIdValue_;

    juce::Label priority1Title_;
    juce::Label priority1Value_;

    juce::Label priority2Title_;
    juce::Label priority2Value_;

    juce::Label port1StatusTitle_;
    juce::Label port1StatusValue_;

    juce::Label port2StatusTitle_;
    juce::Label port2StatusValue_;

    juce::Label currentTimeTitle_;
    juce::Label currentTimeValue_;

    juce::Label offsetFromMasterMinTitle_;
    juce::Label offsetFromMasterMinValue_;

    juce::Label offsetFromMasterMaxTitle_;
    juce::Label offsetFromMasterMaxValue_;

    juce::Label ignoredOutliersTitle_;
    juce::Label ignoredOutliersValue_;
    uint32_t totalIgnoredOutliers{};

    MessageThreadExecutor executor_;

    void timerCallback() override;
    void reset();
};
