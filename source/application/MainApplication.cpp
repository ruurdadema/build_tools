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

#include "MainApplication.hpp"

#include "Logging.hpp"
#include "ravennakit/core/util/common_paths.hpp"

namespace
{
class MainWindow final : public juce::DocumentWindow
{
public:
    explicit MainWindow (const juce::String& name, ApplicationContext& context) :
        DocumentWindow (name, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour (backgroundColourId), allButtons),
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
MainApplication::MainApplication() {}

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

    const auto eulaHash = getEulaAcceptedFile().loadFileAsString();

    if (eulaHash != EulaAcceptWindow::getEulaHash())
    {
        eulaAcceptWindow_ = std::make_unique<EulaAcceptWindow> ("End User License Agreement");
        eulaAcceptWindow_->setVisible (true);
        eulaAcceptWindow_->centreWithSize (1000, 700);
        eulaAcceptWindow_->onEulaAccepted = [commandLine] (const juce::String& hash) {
            const auto& file = getEulaAcceptedFile();
            if (const auto result = file.getParentDirectory().createDirectory(); result.failed())
            {
                RAV_LOG_ERROR ("Failed to create directory");
                quit();
            }
            if (!file.replaceWithText (hash))
            {
                RAV_LOG_ERROR ("Failed to accept EULA");
                quit();
            }

            juce::MessageManager::callAsync ([commandLine] {
                if (auto* app = getInstance())
                    app->initialise (commandLine);
            });
        };
        eulaAcceptWindow_->onEulaDeclined = [] {
            quit();
        };
        return;
    }

    eulaAcceptWindow_.reset();

    ravennaNode_ = std::make_unique<rav::RavennaNode>();
    sessions_ = std::make_unique<DiscoveredSessionsModel> (*ravennaNode_);
    audioReceivers_ = std::make_unique<AudioReceiversModel> (*ravennaNode_);
    audioSenders_ = std::make_unique<AudioSendersModel> (*ravennaNode_);

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    setup.bufferSize = 32;
    audioDeviceManager_.initialise (2, 2, nullptr, false, {}, &setup);

    // Load previous state
    if (const auto& applicationStateFile = getApplicationStateFile(); applicationStateFile.existsAsFile())
    {
        if (const auto result = loadFromFile (applicationStateFile); !result)
            RAV_LOG_ERROR ("Failed to load previous state: {}", result.error());
    }

    if (mainWindows_.empty())
        addWindow();

    audioDeviceManager_.addAudioCallback (audioReceivers_.get());
    audioDeviceManager_.addAudioCallback (audioSenders_.get());
}

void MainApplication::shutdown()
{
    if (ravennaNode_ != nullptr)
    {
        if (const auto result = saveToFile (getApplicationStateFile()); !result)
            RAV_LOG_ERROR ("Failed to save application state: {}", result.error());
    }

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
    RAV_LOG_ERROR ("Unhandled exception: {}, {}:{}", e->what(), sourceFilename.toRawUTF8(), lineNumber);
    RAV_ASSERT (e != nullptr, "Unhandled exception without exception object");

    juce::NativeMessageBox::showMessageBoxAsync (juce::AlertWindow::AlertIconType::WarningIcon, "Exception", e->what(), nullptr);
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

DiscoveredSessionsModel& MainApplication::getSessions()
{
    return *sessions_;
}

juce::AudioDeviceManager& MainApplication::getAudioDeviceManager()
{
    return audioDeviceManager_;
}

AudioReceiversModel& MainApplication::getAudioReceivers()
{
    return *audioReceivers_;
}

AudioSendersModel& MainApplication::getAudioSenders()
{
    return *audioSenders_;
}

tl::expected<void, std::string> MainApplication::saveToFile (const juce::File& file)
{
    const auto json_str = boost::json::serialize (toBoostJson());
    if (const auto result = file.getParentDirectory().createDirectory(); result.failed())
        return tl::unexpected ("Failed to create parent directory");
    if (!file.replaceWithText (json_str))
        return tl::unexpected ("Failed to save to file: " + file.getFullPathName().toStdString());
    RAV_LOG_TRACE ("Saved to file: {}", file.getFullPathName().toRawUTF8());
    return {};
}

tl::expected<void, std::string> MainApplication::loadFromFile (const juce::File& file)
{
    if (!file.existsAsFile())
        return tl::unexpected ("File does not exist: " + file.getFullPathName().toStdString());

    const auto jsonData = file.loadFileAsString();
    if (jsonData.isEmpty())
        return tl::unexpected ("Failed to load file: " + file.getFullPathName().toStdString());

    const auto json = boost::json::parse (jsonData.toRawUTF8());
    if (json.is_null())
        return tl::unexpected ("Failed to parse JSON from file: " + file.getFullPathName().toStdString());

    if (!restoreFromBoostJson (json))
        return tl::unexpected ("Failed to restore from JSON: " + file.getFullPathName().toStdString());

    RAV_LOG_TRACE ("Loaded from file: {}", file.getFullPathName().toRawUTF8());

    return {};
}

std::string MainApplication::getApplicationStateJson()
{
    return boost::json::serialize (toBoostJson());
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

boost::json::object MainApplication::toBoostJson() const
{
    auto windows = boost::json::array();
    for (auto& window : mainWindows_)
    {
        windows.push_back (boost::json::object { { "state", window->getWindowStateAsString().toStdString() } });
    }

    RAV_ASSERT (ravennaNode_ != nullptr, "ravennaNode is null");

    boost::json::object state {
        { "ravenna_node", ravennaNode_->to_boost_json().get() },
        { "windows", windows },
    };

    const auto audioDeviceManagerState = audioDeviceManager_.createStateXml();
    if (audioDeviceManagerState != nullptr)
    {
        const auto format = juce::XmlElement::TextFormat().singleLine();
        state["audio_device_manager_state"] = audioDeviceManagerState->toString (format).toStdString();
    }

    return {
        {
            "ravennakit_juce_demo",
            {
                { "version", PROJECT_VERSION_STRING },
                { "name", PROJECT_PRODUCT_NAME },
                { "company", PROJECT_COMPANY_NAME },
                { "state", state },
            },
        },
    };
}

bool MainApplication::restoreFromBoostJson (const boost::json::value& json)
{
    try
    {
        const auto& application = json.at ("ravennakit_juce_demo");
        const auto& state = application.at ("state");

        auto result = state.try_at ("windows");
        if (result.has_value())
        {
            const auto& windows = result->as_array();

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
                    window->restoreWindowStateFromString (windows[i].at ("state").as_string().data());
                }
            }
        }

        result = state.try_at ("audio_device_manager_state");
        if (result.has_value())
        {
            const auto xml = juce::parseXML (state.at ("audio_device_manager_state").as_string().c_str());
            if (xml != nullptr)
                audioDeviceManager_.initialise (2, 0, xml.get(), true);
        }

        const auto& ravennaNode = state.at ("ravenna_node");
        auto restored = ravennaNode_->restore_from_boost_json (ravennaNode).get();
        if (!restored)
        {
            RAV_LOG_ERROR ("Failed to restore from JSON: {}", restored.error());
            return false;
        }
    }
    catch (const std::exception& e)
    {
        RAV_LOG_ERROR ("Failed to restore from JSON: {}", e.what());
        return false;
    }

    return true;
}

const juce::File& MainApplication::getApplicationStateFile()
{
    const static auto applicationStateFilePath = juce::File (
        (rav::common_paths::application_data() / PROJECT_COMPANY_NAME / PROJECT_PRODUCT_NAME / "application_state.json").c_str());
    return applicationStateFilePath;
}

const juce::File& MainApplication::getEulaAcceptedFile()
{
    const static auto eulaAcceptedFilePath = juce::File (
        (rav::common_paths::application_data() / PROJECT_COMPANY_NAME / PROJECT_PRODUCT_NAME / "accepted_eula").c_str());
    return eulaAcceptedFilePath;
}

JUCE_CREATE_APPLICATION_DEFINE (MainApplication);
JUCE_MAIN_FUNCTION
{
    rav::do_system_checks();
    rav::set_log_level ("trace");
    logging::setupLogging();
    juce::JUCEApplicationBase::createInstance = &juce_CreateApplication;
    return juce::JUCEApplicationBase::main (JUCE_MAIN_FUNCTION_ARGS);
}
