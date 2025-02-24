/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "MainApplication.hpp"

namespace
{
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
        setLookAndFeel (&rav::get_global_instance_of_type<ThisLookAndFeel>());
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};

}
MainApplication::MainApplication()
{
    rav::system::do_system_checks();
    rav::log::set_level_from_env();
}

const juce::String MainApplication::getApplicationName()
{
    return PROJECT_PRODUCT_NAME;
}

const juce::String MainApplication::getApplicationVersion()
{
    return PROJECT_VERSION_STRING;
}
bool MainApplication::moreThanOneInstanceAllowed()
{
    return true;
}

void MainApplication::initialise (const juce::String& commandLine)
{
    CLI::App app { PROJECT_PRODUCT_NAME };
    app.add_option ("--interface-addr", interfaceAddress, "The interface address");
    app.parse (commandLine.toStdString(), false);

    rav::rtp_receiver::configuration config;
    config.interface_address = asio::ip::make_address (interfaceAddress);
    ravennaNode_ = std::make_unique<rav::ravenna_node> (std::move (config));

    addWindow();
}

void MainApplication::shutdown()
{
    mainWindows_.clear();
}

void MainApplication::systemRequestedQuit()
{
    quit();
}

void MainApplication::anotherInstanceStarted (const juce::String& commandLine)
{
    std::ignore = commandLine;
}

void MainApplication::unhandledException (const std::exception* e, const juce::String& sourceFilename, int lineNumber)
{
    RAV_ASSERT (e != nullptr, "Unhandled exception without exception object");
    RAV_ERROR ("Unhandled exception: {}, {}:{}", e->what(), sourceFilename.toRawUTF8(), lineNumber);
}

rav::ravenna_node& MainApplication::getRavennaNode()
{
    return *ravennaNode_;
}

void MainApplication::cloneWindow()
{
    addWindow();
}

void MainApplication::closeWindow (juce::Component* window)
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

void MainApplication::addWindow()
{
    std::optional<juce::Rectangle<int>> bounds;
    if (!mainWindows_.empty())
        bounds = mainWindows_.back()->getBounds();
    const auto& it = mainWindows_.emplace_back (std::make_unique<MainWindow> (getApplicationName(), *this));
    it->setVisible (true);
    bounds ? it->setBounds (bounds->translated (20, 20)) : it->centreWithSize (1200, 800);
}

START_JUCE_APPLICATION (MainApplication)
