/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "DiagnosticsFile.hpp"

std::string DiagnosticsFile::SystemInformation::toString() const
{
    return fmt::format (
        "System information:\n"
        "  OS:                  {}\n"
        "  Device:              {}\n"
        "  Device manufacturer: {}\n"
        "  CPU Logical cores:   {}\n"
        "  CPU Physical cores:  {}\n"
        "  Memory size:         {}MB\n"
        "  CPU Vendor:          {}\n"
        "  CPU Model:           {}\n"
        "  CPU Speed:           {}MHz\n"
        "  JUCE Version:        {}\n"
        "  64bit (OS):          {}\n",
        systemOperatingName.toRawUTF8(),
        deviceDescription.toRawUTF8(),
        deviceManufacturer.toRawUTF8(),
        numberOfLogicalCpuCores,
        numberOfPhysicalCpuCores,
        memorySizeInMegabytes,
        cpuVendor.toRawUTF8(),
        cpuModel.toRawUTF8(),
        cpuSpeedInMegahertz,
        juceVersion.toRawUTF8(),
        (isOperatingSystem64bit ? "true" : "false"));
}

std::string DiagnosticsFile::UserInformation::toString() const
{
    return fmt::format (
        "User information:\n"
        "  Region:           {}\n"
        "  Language:         {}\n"
        "  Display language: {}\n",
        region.toRawUTF8(),
        language.toRawUTF8(),
        displayLanguage.toRawUTF8());
}

void DiagnosticsFile::addFile (const juce::File& fileToAdd, const juce::String& subFolder)
{
    if (fileToAdd.existsAsFile())
    {
        const auto path = subFolder.isNotEmpty()
                        ? fmt::format (
                              PROJECT_PRODUCT_NAME " diagnostics/{}/{}",
                              subFolder.toRawUTF8(),
                              fileToAdd.getFileName().toRawUTF8())
                        : fmt::format (PROJECT_PRODUCT_NAME " diagnostics/{}", fileToAdd.getFileName().toRawUTF8());
        builder_.addFile (fileToAdd, k_zip_compression_level, path);
    }
}

void DiagnosticsFile::addTextFile (const juce::String& text, const juce::String& filename)
{
    const auto file = getDiagnosticsDirectory().getChildFile (filename);
    if (!file.replaceWithText (text))
    {
        RAV_LOG_ERROR ("Failed to text to file");
        return;
    }
    builder_.addFile (file, k_zip_compression_level, juce::String (PROJECT_PRODUCT_NAME " diagnostics/") + filename);
}

void DiagnosticsFile::addLogfiles()
{
    // Add logfiles
    auto logFiles = logging::getLogFilesPath().findChildFiles (juce::File::findFiles, false);
    for (const auto& logFile : logFiles)
    {
        builder_.addFile (
            logFile,
            k_zip_compression_level,
            juce::String (PROJECT_PRODUCT_NAME " diagnostics/logs/") + logFile.getFileName());
    }
}

void DiagnosticsFile::addProjectAndBuildInfoFile()
{
    const auto file = getDiagnosticsDirectory().getChildFile ("build_info.txt");
    file.replaceWithText (logging::getProjectAndBuildInformation());
    builder_.addFile (file, k_zip_compression_level, juce::String (PROJECT_PRODUCT_NAME " diagnostics/build_info.txt"));
}

void DiagnosticsFile::addSystemInformationFile()
{
    const auto file = getDiagnosticsDirectory().getChildFile ("system_information.txt");
    if (!file.replaceWithText (SystemInformation().toString()))
    {
        RAV_LOG_ERROR ("Failed to replace file with text");
    }
    builder_.addFile (file, k_zip_compression_level, juce::String (PROJECT_PRODUCT_NAME " diagnostics/system_information.txt"));
}

void DiagnosticsFile::addUserInformationFile()
{
    const auto file = getDiagnosticsDirectory().getChildFile ("user_information.txt");
    if (!file.replaceWithText (UserInformation().toString()))
    {
        RAV_LOG_ERROR ("Failed to replace file with text");
    }
    builder_.addFile (file, k_zip_compression_level, juce::String (PROJECT_PRODUCT_NAME " diagnostics/user_information.txt"));
}

bool DiagnosticsFile::generateDiagnosticsFile (const juce::File& fileToWriteTo) const
{
    if (fileToWriteTo.exists())
        std::ignore = fileToWriteTo.deleteFile();

    juce::FileOutputStream fileOutputStream (fileToWriteTo);

    if (fileOutputStream.failedToOpen())
    {
        RAV_LOG_ERROR (
            "Failed to open output file ({}): {}",
            fileToWriteTo.getFullPathName().toRawUTF8(),
            fileOutputStream.getStatus().getErrorMessage().toRawUTF8());
        return false;
    }

    if (!builder_.writeToStream (fileOutputStream, nullptr))
    {
        RAV_LOG_ERROR ("Failed to write diagnostics file to zip");
        return false;
    }

    fileOutputStream.flush();

    return true;
}

void DiagnosticsFile::saveDiagnosticsFile (const std::function<void (bool success, const juce::File& file)>& resultHandler)
{
    if (resultHandler == nullptr)
        return;

    file_chooser_ = std::make_unique<juce::FileChooser> (
        "Please select the folder where you want to save the diagnostics file",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile (PROJECT_PRODUCT_NAME " diagnostics.zip")),

    file_chooser_->launchAsync (
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
        [this, resultHandler] (const juce::FileChooser& chooser) {
            const auto file = chooser.getResult();

            if (file.getFullPathName().isEmpty())
                return; // User cancelled.

            const auto result = generateDiagnosticsFile (file);
            if (!result)
                RAV_LOG_ERROR ("Failed to generate diagnostics file to: {}", file.getFullPathName().toRawUTF8());
            resultHandler (result, file);
        });
}

const juce::File& DiagnosticsFile::getDiagnosticsDirectory()
{
#if JUCE_ANDROID || JUCE_IOS
    const static auto diagnosticsDirectory = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                                 .getChildFile ("diagnostics");
#else
    const static auto diagnosticsDirectory = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                                 .getChildFile (PROJECT_COMPANY_NAME)
                                                 .getChildFile (PROJECT_PRODUCT_NAME)
                                                 .getChildFile ("diagnostics");
#endif
    const auto result = diagnosticsDirectory.createDirectory();
    if (result.failed())
        RAV_LOG_ERROR ("Failed to create diagnostics directory");

    return diagnosticsDirectory;
}
