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

#pragma once

#include "Logging.hpp"
#include "ravennakit/core/log.hpp"

#include <fmt/format.h>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Convenience class for creating a zip file containing all kinds of diagnostics information.
 */
class DiagnosticsFile
{
public:
    struct UserInformation
    {
        juce::String region = juce::SystemStats::getUserRegion();
        juce::String language = juce::SystemStats::getUserLanguage();
        juce::String displayLanguage = juce::SystemStats::getDisplayLanguage();

        [[nodiscard]] std::string toString() const;
    };

    struct SystemInformation
    {
        juce::String systemOperatingName = juce::SystemStats::getOperatingSystemName();
        juce::String deviceDescription = juce::SystemStats::getDeviceDescription();
        juce::String deviceManufacturer = juce::SystemStats::getDeviceManufacturer();
        int numberOfLogicalCpuCores = juce::SystemStats::getNumCpus();
        int numberOfPhysicalCpuCores = juce::SystemStats::getNumPhysicalCpus();
        int memorySizeInMegabytes = juce::SystemStats::getMemorySizeInMegabytes();
        juce::String cpuVendor = juce::SystemStats::getCpuVendor();
        juce::String cpuModel = juce::SystemStats::getCpuModel();
        int cpuSpeedInMegahertz = juce::SystemStats::getCpuSpeedInMegahertz();
        juce::String juceVersion = juce::SystemStats::getJUCEVersion();
        bool isOperatingSystem64bit = juce::SystemStats::isOperatingSystem64Bit();

        [[nodiscard]] std::string toString() const;
    };

    DiagnosticsFile() = default;

    /**
     * Adds a file to the zip file.
     * @param fileToAdd The file to add.
     * @param subFolder The subfolder in which the file should be added. If empty, the file will be added to the root.
     */
    void addFile (const juce::File& fileToAdd, const juce::String& subFolder = juce::String());

    /**
     * Adds text to a file in the zip file.
     * @param text The text to add.
     * @param filename The name (and path) of the filename inside the zip file.
     */
    void addTextFile (const juce::String& text, const juce::String& filename);

    /**
     * Adds logfiles to the zip file.
     */
    void addLogfiles();

    /**
     * Adds project and build info as files to the zip file.
     */
    void addProjectAndBuildInfoFile();

    /**
     * Adds system information as file to the zip file.
     */
    void addSystemInformationFile();

    /**
     * Adds user information as file to the zip file.
     */
    void addUserInformationFile();

    /**
     * Generates the diagnostics file.
     * @param fileToWriteTo File to write the diagnostics file to. If the file already exists, it will be overwritten.
     */
    [[nodiscard]] bool generateDiagnosticsFile (const juce::File& fileToWriteTo) const;

    /**
     * Opens a file chooser to allow the user to select where to save the diagnostics file and saves the diagnostics
     * into the selected file.
     * @param resultHandler Called when the operation finished. The first parameter indicates whether the operation was
     * successful. The second parameter is the file that was selected.
     */
    void saveDiagnosticsFile (const std::function<void (bool success, const juce::File& file)>& resultHandler);

    static const juce::File& getDiagnosticsDirectory();

private:
    static constexpr int k_json_indentation = 2;
    static constexpr int k_zip_compression_level = 9;

    juce::ZipFile::Builder builder_;
    std::vector<std::unique_ptr<juce::TemporaryFile>> temporary_files_;
    std::unique_ptr<juce::FileChooser> file_chooser_;
    juce::ScopedMessageBox scoped_message_box_;
};
