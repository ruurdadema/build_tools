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

} // namespace
MainApplication::MainApplication()
{
    rav::do_system_checks();
    rav::set_log_level_from_env();
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
    std::string interfaceAddressString;
    app.add_option ("--interface-addr", interfaceAddressString, "The interface address");
    app.parse (commandLine.toStdString(), false);

    asio::ip::address_v4 interfaceAddress;
    if (!interfaceAddressString.empty())
    {
        asio::error_code ec;
        interfaceAddress = asio::ip::make_address_v4 (interfaceAddressString, ec);
        if (ec)
        {
            RAV_ERROR ("Invalid interface address: {}", interfaceAddressString);
            juce::NativeMessageBox::showMessageBoxAsync (
                juce::AlertWindow::AlertIconType::WarningIcon,
                "Invalid interface address",
                "Failed to parse the interface address: " + interfaceAddressString,
                nullptr);
        }
    }
    ravennaNode_ = std::make_unique<rav::RavennaNode> (interfaceAddress);
    sessions_ = std::make_unique<RavennaSessions> (*ravennaNode_);
    audioReceivers_ = std::make_unique<AudioReceivers> (*ravennaNode_);
    audioSenders_ = std::make_unique<AudioSenders> (*ravennaNode_);

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    setup.bufferSize = 32;
    audioDeviceManager_.initialise (2, 2, nullptr, false, {}, &setup);
    audioDeviceManager_.addAudioCallback (audioReceivers_.get());
    audioDeviceManager_.addAudioCallback (audioSenders_.get());

    addWindow();
}

void MainApplication::shutdown()
{
    audioDeviceManager_.removeAudioCallback (audioReceivers_.get());
    audioDeviceManager_.removeAudioCallback (audioSenders_.get());
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
    RAV_ERROR ("Unhandled exception: {}, {}:{}", e->what(), sourceFilename.toRawUTF8(), lineNumber);
    RAV_ASSERT (e != nullptr, "Unhandled exception without exception object");

    juce::NativeMessageBox::showMessageBoxAsync (
        juce::AlertWindow::AlertIconType::WarningIcon,
        "Exception",
        e->what(),
        nullptr);
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

RavennaSessions& MainApplication::getSessions()
{
    return *sessions_;
}

juce::AudioDeviceManager& MainApplication::getAudioDeviceManager()
{
    return audioDeviceManager_;
}

AudioReceivers& MainApplication::getAudioReceivers()
{
    return *audioReceivers_;
}

AudioSenders& MainApplication::getAudioSenders()
{
    return *audioSenders_;
}

void MainApplication::addWindow()
{
    std::optional<juce::Rectangle<int>> bounds;
    if (!mainWindows_.empty())
        bounds = mainWindows_.back()->getBounds();
    const auto& it = mainWindows_.emplace_back (std::make_unique<MainWindow> (getApplicationName(), *this));
    it->setVisible (true);
    it->setResizeLimits (1100, 400, 99999, 99999);
    bounds ? it->setBounds (bounds->translated (20, 20)) : it->centreWithSize (1200, 800);
}

START_JUCE_APPLICATION (MainApplication)
