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

#include <CLI/CLI.hpp>

class Application final : public juce::JUCEApplication, public ApplicationContext
{
public:
    Application()
    {
        rav::system::do_system_checks();
        rav::log::set_level_from_env();
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
        CLI::App app { PROJECT_PRODUCT_NAME };
        app.add_option ("--interface-addr", interfaceAddress, "The interface address");
        app.parse (commandLine.toStdString(), false);

        rav::rtp_receiver::configuration config;
        config.interface_address = asio::ip::make_address (interfaceAddress);
        ravennaNode_ = std::make_unique<rav::ravenna_node>(std::move(config));

        addWindow();
    }

    void shutdown() override
    {
        mainWindows_.clear();
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
        RAV_ERROR ("Unhandled exception: {}, {}:{}", e->what(), sourceFilename.toRawUTF8(), lineNumber);
    }

    rav::ravenna_node& getRavennaNode() override
    {
        return *ravennaNode_;
    }

    void cloneWindow() override
    {
        addWindow();
    }

    void closeWindow (juce::Component* window) override
    {
        for (auto it = mainWindows_.begin(); it != mainWindows_.end(); ++it)
        {
            if (it->get() == window)
            {
                mainWindows_.erase (it);
                break;
            }
        }
    }

    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (const juce::String& name, ApplicationContext& context) :
            DocumentWindow (
                name,
                juce::Desktop::getInstance().getDefaultLookAndFeel().findColour (backgroundColourId),
                allButtons),
            context_ (context)
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
            context_.closeWindow (this);
        }

    private:
        ApplicationContext& context_;
        ThisLookAndFeel lookAndFeel_;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::string interfaceAddress;
    std::unique_ptr<rav::ravenna_node> ravennaNode_;
    std::vector<std::unique_ptr<MainWindow>> mainWindows_;

    void addWindow()
    {
        std::optional<juce::Rectangle<int>> bounds;
        if (!mainWindows_.empty())
            bounds = mainWindows_.back()->getBounds();
        const auto& it = mainWindows_.emplace_back (std::make_unique<MainWindow> (getApplicationName(), *this));
        it->setVisible (true);
        bounds ? it->setBounds (bounds->translated (20, 20)) : it->centreWithSize (1200, 800);
    }
};

START_JUCE_APPLICATION (Application)
