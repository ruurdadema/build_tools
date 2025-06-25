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

#include "discovered/DiscoveredMainComponent.hpp"
#include "nmos/NmosMainComponent.hpp"
#include "ravennakit/core/util/tracy.hpp"
#include "receivers/ReceiversMainComponent.hpp"
#include "senders/SendersMainComponent.hpp"
#include "settings/SettingsMainComponent.hpp"

MainComponent::MainComponent (ApplicationContext& context) : context_ (context), topRightSection_ (context)
{
    menu_.addItem ("Discovered", "discovered", [this] {
        showContent (std::make_unique<DiscoveredMainComponent> (context_, *this));
    });
    menu_.addItem ("Receivers", "receivers", [this] {
        showContent (std::make_unique<ReceiversMainComponent> (context_));
    });
    menu_.addItem ("Senders", "senders", [this] {
        showContent (std::make_unique<SendersMainComponent> (context_));
    });
    menu_.addItem ("NMOS", "nmos", [this] {
        showContent (std::make_unique<NmosMainComponent> (context_));
    });
    menu_.addItem ("Settings", "settings", [this] {
        showContent (std::make_unique<SettingsMainComponent> (context_));
    });
    addAndMakeVisible (menu_);

    addAndMakeVisible (topRightSection_);
}

void MainComponent::paint (juce::Graphics& g)
{
    TRACY_ZONE_SCOPED;

    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto b = getLocalBounds();
    auto top = b.removeFromTop (49);
    topRightSection_.setBounds (top.removeFromRight (274));
    menu_.setBounds (top);
    if (content_ != nullptr)
        content_->setBounds (b);
}

bool MainComponent::navigateTo (const juce::String& path)
{
    return menu_.navigateTo (path);
}

MainComponent::TopRightSection::TopRightSection (ApplicationContext& context)
{
    saveButton_.setButtonText ("Save");
    saveButton_.setColour (juce::TextButton::ColourIds::buttonColourId, juce::Colour (0xFF8E8F9A));
    saveButton_.setColour (juce::TextButton::ColourIds::buttonOnColourId, juce::Colour (0xFF8E8F9A));
    saveButton_.onClick = [this, &context] {
        chooser_ = std::make_unique<juce::FileChooser> (
            "Please select the file to save to...",
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
            "*.json");
        constexpr int flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting |
                              juce::FileBrowserComponent::doNotClearFileNameOnRootChange;
        chooser_->launchAsync (flags, [&context] (const juce::FileChooser& chooser) {
            const auto file = chooser.getResult();
            if (file.getFullPathName().isEmpty())
                return; // Canceled
            if (auto result = context.saveToFile (file); !result)
            {
                juce::NativeMessageBox::showAsync (
                    juce::MessageBoxOptions()
                        .withTitle ("Error")
                        .withMessage ("Failed to save to file: " + result.error())
                        .withIconType (juce::MessageBoxIconType::WarningIcon)
                        .withParentComponent (nullptr),
                    [] (int) {});
            }
        });
    };
    addAndMakeVisible (saveButton_);

    loadButton_.setButtonText ("Load");
    loadButton_.setColour (juce::TextButton::ColourIds::buttonColourId, juce::Colour (0xFF8E8F9A));
    loadButton_.setColour (juce::TextButton::ColourIds::buttonOnColourId, juce::Colour (0xFF8E8F9A));
    loadButton_.onClick = [this, &context] {
        chooser_ = std::make_unique<juce::FileChooser> (
            "Please select the file to load from...",
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
            "*.json");
        constexpr int flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        chooser_->launchAsync (flags, [&context] (const juce::FileChooser& chooser) {
            const auto file = chooser.getResult();
            if (file.getFullPathName().isEmpty())
                return; // Canceled
            if (auto result = context.loadFromFile (file); !result)
            {
                juce::NativeMessageBox::showAsync (
                    juce::MessageBoxOptions()
                        .withTitle ("Error")
                        .withMessage ("Failed to load from file: " + result.error())
                        .withIconType (juce::MessageBoxIconType::WarningIcon)
                        .withParentComponent (nullptr),
                    [] (int) {});
            }
        });
    };
    addAndMakeVisible (loadButton_);

    cloneWindowButton_.setButtonText ("Clone window");
    cloneWindowButton_.setColour (juce::TextButton::ColourIds::buttonColourId, juce::Colour (0xFF8E8F9A));
    cloneWindowButton_.setColour (juce::TextButton::ColourIds::buttonOnColourId, juce::Colour (0xFF8E8F9A));
    cloneWindowButton_.onClick = [&context] {
        context.cloneWindow();
    };
    addAndMakeVisible (cloneWindowButton_);
}

void MainComponent::TopRightSection::resized()
{
    auto b = getLocalBounds().reduced (8);

    cloneWindowButton_.setBounds (b.removeFromRight (82));
    b.removeFromRight (6);
    loadButton_.setBounds (b.removeFromRight (82));
    b.removeFromRight (6);
    saveButton_.setBounds (b.removeFromRight (82));
}

void MainComponent::TopRightSection::paint (juce::Graphics& g)
{
    g.drawRect (getLocalBounds().removeFromBottom (1));
}

void MainComponent::showContent (std::unique_ptr<Component> content)
{
    content_ = std::move (content);
    if (content_ == nullptr)
        return;
    addAndMakeVisible (content_.get());
    resized();
}
