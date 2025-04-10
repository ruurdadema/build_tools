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
    std::ignore = commandLine;

    ravennaNode_ = std::make_unique<rav::RavennaNode>();
    sessions_ = std::make_unique<DiscoveredSessions> (*ravennaNode_);
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

rav::RavennaNode& MainApplication::getRavennaNode()
{
    return *ravennaNode_;
}

DiscoveredSessions& MainApplication::getSessions()
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

void MainApplication::saveToFile (const juce::File& file)
{
    const auto json = toJson().dump (4);
    if (!file.replaceWithText (json))
    {
        RAV_ERROR ("Failed to save to file: {}", file.getFullPathName().toRawUTF8());
        return;
    }
    RAV_TRACE ("Saved to file: {}", file.getFullPathName().toRawUTF8());
}

void MainApplication::loadFromFile (const juce::File& file)
{
    if (!file.existsAsFile())
    {
        RAV_ERROR ("File does not exist: {}", file.getFullPathName().toRawUTF8());
        return;
    }

    const auto jsonData = file.loadFileAsString();
    if (jsonData.isEmpty())
    {
        RAV_ERROR ("Failed to load file: {}", file.getFullPathName().toRawUTF8());
        return;
    }

    const auto json = nlohmann::json::parse (jsonData.toRawUTF8());
    if (json.is_null())
    {
        RAV_ERROR ("Failed to parse JSON from file: {}", file.getFullPathName().toRawUTF8());
        return;
    }

    if (!restoreFromJson (json))
    {
        RAV_ERROR ("Failed to restore from JSON: {}", file.getFullPathName().toRawUTF8());
        return;
    }

    RAV_TRACE ("Loaded from file: {}", file.getFullPathName().toRawUTF8());
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

nlohmann::json MainApplication::toJson() const
{
    nlohmann::json json;
    json["ravenna_node"] = ravennaNode_->to_json().get();
    auto windows = nlohmann::json::array();
    for (auto& window : mainWindows_)
    {
        nlohmann::json windowJson;
        windowJson["state"] = window->getWindowStateAsString().toStdString();
        windows.push_back (windowJson);
    }
    json["windows"] = windows;
    return json;
}

bool MainApplication::restoreFromJson (const nlohmann::json& json)
{
    try
    {
        if (json.contains ("windows"))
        {
            auto windows = json.at("windows").get<std::vector<nlohmann::json>>();

            // Add windows if needed
            while (mainWindows_.size() < windows.size())
                addWindow();

            // Remove windows if needed
            if (mainWindows_.size() > windows.size())
                mainWindows_.resize (windows.size());

            for (size_t i = 0; i < windows.size(); ++i)
            {
                if (const auto& window = mainWindows_[i])
                {
                    window->restoreWindowStateFromString (windows[i].at("state").get<std::string>());
                }
            }
        }

        auto result = ravennaNode_->restore_from_json (json.at ("ravenna_node")).get();
        if (!result)
        {
            RAV_ERROR ("Failed to restore from JSON: {}", result.error());
            return false;
        }
    }
    catch (const std::exception& e)
    {
        RAV_ERROR ("Failed to restore from JSON: {}", e.what());
        return false;
    }

    return true;
}

START_JUCE_APPLICATION (MainApplication)
