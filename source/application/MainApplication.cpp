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

    // Load previous state
    if (const auto applicationStateFile = getApplicationStateFile(); applicationStateFile.existsAsFile())
    {
        if (const auto result = loadFromFile (applicationStateFile); !result)
            RAV_ERROR ("Failed to load previous state: {}", result.error());
    }

    if (mainWindows_.empty())
        addWindow();

    audioDeviceManager_.addAudioCallback (audioReceivers_.get());
    audioDeviceManager_.addAudioCallback (audioSenders_.get());
}

void MainApplication::shutdown()
{
    if (const auto result = saveToFile (getApplicationStateFile()); !result)
        RAV_ERROR ("Failed to save application state: {}", result.error());

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
            if (mainWindows_.empty())
                quit(); // Quit the application if no windows are left
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

tl::expected<void, std::string> MainApplication::saveToFile (const juce::File& file)
{
    const auto json = toJson().dump (4);
    if (const auto result = file.getParentDirectory().createDirectory(); result.failed())
        return tl::unexpected("Failed to create parent directory");
    if (!file.replaceWithText (json))
        return tl::unexpected ("Failed to save to file: " + file.getFullPathName().toStdString());
    RAV_TRACE ("Saved to file: {}", file.getFullPathName().toRawUTF8());
    return {};
}

tl::expected<void, std::string> MainApplication::loadFromFile (const juce::File& file)
{
    if (!file.existsAsFile())
        return tl::unexpected ("File does not exist: " + file.getFullPathName().toStdString());

    const auto jsonData = file.loadFileAsString();
    if (jsonData.isEmpty())
        return tl::unexpected ("Failed to load file: " + file.getFullPathName().toStdString());

    const auto json = nlohmann::json::parse (jsonData.toRawUTF8());
    if (json.is_null())
        return tl::unexpected ("Failed to parse JSON from file: " + file.getFullPathName().toStdString());

    if (!restoreFromJson (json))
        return tl::unexpected ("Failed to restore from JSON: " + file.getFullPathName().toStdString());

    RAV_TRACE ("Loaded from file: {}", file.getFullPathName().toRawUTF8());

    return {};
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
    auto windows = nlohmann::json::array();
    for (auto& window : mainWindows_)
    {
        nlohmann::json windowJson;
        windowJson["state"] = window->getWindowStateAsString().toStdString();
        windows.push_back (windowJson);
    }

    nlohmann::json state;
    state["ravenna_node"] = ravennaNode_->to_json().get();
    state["windows"] = windows;

    const auto audioDeviceManagerState = audioDeviceManager_.createStateXml();
    if (audioDeviceManagerState != nullptr)
    {
        const auto format = juce::XmlElement::TextFormat().singleLine();
        state["audio_device_manager_state"] = audioDeviceManagerState->toString (format).toStdString();
    }

    nlohmann::json application;
    application["version"] = PROJECT_VERSION_STRING;
    application["name"] = PROJECT_PRODUCT_NAME;
    application["company"] = PROJECT_COMPANY_NAME;
    application["state"] = state;

    nlohmann::json root;
    root["ravennakit_juce_demo"] = application;

    return root;
}

bool MainApplication::restoreFromJson (const nlohmann::json& json)
{
    try
    {
        const auto application = json.at ("ravennakit_juce_demo");
        const auto state = application.at ("state");

        if (state.contains ("windows"))
        {
            auto windows = state.at ("windows").get<std::vector<nlohmann::json>>();

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
                    window->restoreWindowStateFromString (windows[i].at ("state").get<std::string>());
                }
            }
        }

        if (state.contains ("audio_device_manager_state"))
        {
            const auto xml = juce::parseXML (state.at ("audio_device_manager_state").get<std::string>());
            if (xml != nullptr)
                audioDeviceManager_.initialise (2, 0, xml.get(), true);
        }

        const auto ravennaNode = state.at ("ravenna_node");
        auto result = ravennaNode_->restore_from_json (ravennaNode).get();
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

const juce::File& MainApplication::getApplicationStateFile()
{
    const static auto applicationStateFilePath = juce::File::getSpecialLocation (
                                                     juce::File::userApplicationDataDirectory)
                                                     .getChildFile (PROJECT_COMPANY_NAME)
                                                     .getChildFile (PROJECT_PRODUCT_NAME)
                                                     .getChildFile ("application_state.json");
    return applicationStateFilePath;
}

START_JUCE_APPLICATION (MainApplication)
