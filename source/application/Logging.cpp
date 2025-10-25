/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "Logging.hpp"

#include "ravennakit/core/log.hpp"

#include <spdlog/sinks/android_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/msvc_sink.h>

namespace
{
void logBasicBuildInformation()
{
    RAV_LOG_INFO ("Project and build info:\n{}", logging::getProjectAndBuildInformation());
}

} // namespace

void logging::setupLogging()
{
    static std::atomic inited { false };
    bool expected = false;
    if (!inited.compare_exchange_strong (expected, true))
        return; // Nothing to do here.

    const auto logfilesPath = getLogFilesPath();

    // Create the directory (if it does not exist)
    const auto result = logfilesPath.createDirectory();
    if (!result)
    {
        RAV_LOG_ERROR ("Failed to creat logfiles directory: {}", result.getErrorMessage().toRawUTF8());
        return;
    }

    auto logFileName = fmt::format (
        "{}/{}.log",
        logfilesPath.getFullPathName().toRawUTF8(),
        juce::String (PROJECT_PRODUCT_NAME).replaceCharacter (' ', '_').toRawUTF8());

    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt> (logFileName, 1048576 * 5,
                                                                            10); // 5 * 10 MB

    std::shared_ptr<spdlog::logger> logger;

#if defined _WIN32 && !defined _CONSOLE
    auto msvcSink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    // A note about logging on Windows: when creating a GUI app, Windows doesn't set up stdin/stdout which means that
    // logs to this sink don't end up anywhere. To look at logs in realtime you have to run in debug mode (attaching a
    // debugger).
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    logger = std::make_shared<spdlog::logger> (spdlog::logger ("default", { msvcSink, fileSink, console_sink }));
#elif JUCE_ANDROID
    auto android_sink = std::make_shared<spdlog::sinks::android_sink_mt>();
    logger = std::make_shared<spdlog::logger> (spdlog::logger ("default", { fileSink, android_sink }));
#else
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    logger = std::make_shared<spdlog::logger> (spdlog::logger ("default", { fileSink, console_sink }));
#endif

    // For some reason the global functions (like spdlog::set_default_logger()) don't work on Android when compiled in
    // release mode. I haven't been able to figure out what exactly is going on, but going the manual route and avoiding
    // the global functions seems to work.
    auto& registry = spdlog::details::registry::instance();
    registry.set_default_logger (std::move (logger));
    registry.flush_on (spdlog::level::warn);
    registry.set_level (spdlog::level::trace);
    registry.flush_every (std::chrono::seconds (1));

    RAV_LOG_INFO (
        "==================== Starting new log on {} ====================",
        juce::SystemStats::getOperatingSystemName().toRawUTF8());
    RAV_LOG_INFO ("Logging to: ({})", fileSink->filename());

    logBasicBuildInformation();

#if TRACY_ENABLE
    RAV_LOG_INFO ("Tracy enabled");
#endif
}

const juce::File& logging::getLogFilesPath()
{
#if JUCE_ANDROID || JUCE_IOS
    const static auto logfilesPath = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("logs");
#elif JUCE_MAC
    const static auto logfilesPath = juce::File ("~/Library/Logs").getChildFile (PROJECT_COMPANY_NAME).getChildFile (PROJECT_PRODUCT_NAME);
#else
    const static auto logfilesPath = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                         .getChildFile (PROJECT_COMPANY_NAME)
                                         .getChildFile (PROJECT_PRODUCT_NAME)
                                         .getChildFile ("logs");
#endif
    return logfilesPath;
}

std::string logging::getProjectAndBuildInformation()
{
#ifdef RAV_ENABLE_DEBUG
    static constexpr bool debugEnabled { true };
#else
    static constexpr bool debugEnabled { false };
#endif

    return fmt::format (
        "Project manufacturer:     {}\n"
        "Project application name: {}\n"
        "Project version string:   {}\n"
        "Project bundle id:        {}\n"
        "Debug enabled:            {}\n",
        PROJECT_COMPANY_NAME,
        PROJECT_PRODUCT_NAME,
        PROJECT_VERSION_STRING,
        PROJECT_BUNDLE_ID,
        debugEnabled ? "true" : "false");
}
