/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "MainComponent.hpp"

#include "ReceiversComponent.hpp"
#include "SendersComponent.hpp"
#include "ravennakit/core/tracy.hpp"

MainComponent::MainComponent()
{
    menu_.addItem ("Receivers", "receivers", [this] {
        showContent (std::make_unique<ReceiversComponent>());
    });
    menu_.addItem ("Senders", "senders", [this] {
        showContent (std::make_unique<SendersComponent>());
    });
    addAndMakeVisible (menu_);
}

void MainComponent::paint (juce::Graphics& g)
{
    TRACY_ZONE_SCOPED;

    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto b = getLocalBounds();
    menu_.setBounds (b.removeFromTop (49));
    if (content_ != nullptr)
        content_->setBounds (b);
}

void MainComponent::showContent (std::unique_ptr<Component> content)
{
    content_ = std::move (content);
    if (content_ == nullptr)
        return;
    addAndMakeVisible (content_.get());
    resized();
}
