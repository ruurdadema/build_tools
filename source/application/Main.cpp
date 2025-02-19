/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include <memory>

#include "gui/MainComponent.hpp"
#include "gui/lookandfeel/ThisLookAndFeel.hpp"
#include "ravennakit/core/system.hpp"

class Application final : public juce::JUCEApplication
{
public:
    Application()
    {
        rav::system::do_system_checks();
    }

    const juce::String getApplicationName() override
    {
        return PROJECT_PRODUCT_NAME;
    }

    const juce::String getApplicationVersion() override
    {
        return PROJECT_VERSION_STRING;
    }

    bool moreThanOneInstanceAllowed() override
    {
        return true;
    }

    void initialise (const juce::String& commandLine) override
    {
        rav::log::set_level_from_env();

        std::ignore = commandLine;
        mainWindow_ = std::make_unique<MainWindow> (getApplicationName(), context_);
        mainWindow_->setVisible (true);
        mainWindow_->centreWithSize (1200, 800);
    }

    void shutdown() override
    {
        mainWindow_.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
        std::ignore = commandLine;
    }

    void unhandledException (const std::exception* e, const juce::String& sourceFilename, int lineNumber) override
    {
        RAV_ASSERT (e != nullptr, "Unhandled exception without exception object");
        RAV_ERROR("Unhandled exception: {}, {}:{}", e->what(), sourceFilename.toRawUTF8(), lineNumber);
    }

    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (const juce::String& name, ApplicationContext& context) :
            DocumentWindow (
                name,
                juce::Desktop::getInstance().getDefaultLookAndFeel().findColour (backgroundColourId),
                allButtons)
        {
            setLookAndFeel (&lookAndFeel_);
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent (context), true);

            setResizable (true, true);
            centreWithSize (getWidth(), getHeight());
        }

        ~MainWindow() override
        {
            setLookAndFeel (nullptr);
        }

        void closeButtonPressed() override
        {
            getInstance()->systemRequestedQuit();
        }

    private:
        ThisLookAndFeel lookAndFeel_;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    ApplicationContext context_;
    std::unique_ptr<MainWindow> mainWindow_;
};

START_JUCE_APPLICATION (Application)
