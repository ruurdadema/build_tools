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

#include "HorizontalMenu.hpp"

#include <utility>

HorizontalMenu::HorizontalMenu() = default;

void HorizontalMenu::addItem (const juce::String& name, const juce::String& path, std::function<void()> onClick)
{
    auto* newButton = buttons_.add (std::make_unique<MenuItem> (name, path, [this, handler = std::move (onClick), path] {
        if (handler)
            handler();
        for (auto* item : buttons_)
            item->setActive (item->getPath() == path);
    }));

    if (newButton == nullptr)
        return;

    addAndMakeVisible (newButton);

    if (buttons_.size() == 1)
        newButton->triggerClick();

    resized();
}

bool HorizontalMenu::navigateTo (const juce::String& path)
{
    for (auto* item : buttons_)
    {
        if (item->getPath() == path)
        {
            item->triggerClick();
            return true;
        }
    }
    return false;
}

void HorizontalMenu::paint (juce::Graphics& g)
{
    g.drawRect (getLocalBounds().removeFromBottom (1));
}

void HorizontalMenu::resized()
{
    auto b = getLocalBounds();
    for (auto* item : buttons_)
        item->setBounds (b.removeFromLeft (static_cast<int> (item->getIdealWidth())));
}

HorizontalMenu::MenuItem::MenuItem (const juce::String& name, const juce::String& path, std::function<void()> onClick)
{
    path_ = path;
    button_.setButtonText (name);
    button_.setFont (font_, false);
    button_.onClick = std::move (onClick);

    addAndMakeVisible (button_);
}

float HorizontalMenu::MenuItem::getIdealWidth() const
{
    return juce::TextLayout::getStringWidth (font_, button_.getButtonText()) + 50;
}

void HorizontalMenu::MenuItem::triggerClick()
{
    button_.triggerClick();
}

void HorizontalMenu::MenuItem::setActive (const bool active)
{
    if (active_ == active)
        return;
    active_ = active;
    repaint();
}

const juce::String& HorizontalMenu::MenuItem::getPath() const
{
    return path_;
}

void HorizontalMenu::MenuItem::resized()
{
    const auto b = getLocalBounds();
    button_.setBounds (b);
}

void HorizontalMenu::MenuItem::paint (juce::Graphics& g)
{
    if (active_)
    {
        g.setColour (findColour (juce::HyperlinkButton::ColourIds::textColourId));
        g.drawRect (getLocalBounds().removeFromBottom (1));
    }
}
