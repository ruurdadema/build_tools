cmake_minimum_required(VERSION 3.22)

include(submodules/ravennakit/submodules/build_tools/cmake/git_version.cmake)
include(submodules/ravennakit/cmake/helpers.cmake)

project(ravennakit_diagnostics VERSION ${GIT_VERSION_MAJOR}.${GIT_VERSION_MINOR}.${GIT_VERSION_PATCH} LANGUAGES C CXX)

#####################
# Project variables #
#####################

add_project_variable(PROJECT_COMPANY_NAME "Sound on Digital")
add_project_variable(PROJECT_PRODUCT_NAME "RAVENNAKIT Diagnostics")
add_project_variable(PROJECT_BUNDLE_ID "com.soundondigital.ravennakit-diagnostics")
add_project_variable(PROJECT_URL "https://soundondigital.com/")
add_project_variable(PROJECT_SUPPORT_URL "https://soundondigital.com/support/")
add_project_variable(PROJECT_EMAIL "support@soundondigital.com")
add_project_variable(PROJECT_VERSION_STRING ${GIT_DESCRIBE_VERSION})

##########################
# Compiler configuration #
##########################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
set(CMAKE_XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=Release] "YES")
set(CMAKE_XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=RelWithDebInfo] "YES")

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE INTERNAL "")
endif ()

##############
# RAVENNAKIT #
##############

add_subdirectory(submodules/ravennakit)

########
# JUCE #
########

set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

add_subdirectory(submodules/JUCE)
set(JUCE_COMMON_TARGET_DEFINITIONS
        DONT_SET_USING_JUCE_NAMESPACE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISABLE_JUCE_VERSION_PRINTING=1
        JUCE_DISABLE_COREGRAPHICS_FONT_SMOOTHING=1
        JUCE_COREGRAPHICS_DRAW_ASYNC=1
        JUCE_CATCH_UNHANDLED_EXCEPTIONS=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        NOMINMAX
        JUCE_ENABLE_REPAINT_DEBUGGING=0
        JUCE_IOS_AUDIO_LOGGING=0
        JucePlugin_AAXDisableDynamicProcessing=1
        JUCE_DIRECTSOUND=0
        JUCE_WASAPI=1
        JUCE_ASIO=1
)

juce_add_gui_app(ravennakit_diagnostics
        PRODUCT_NAME ${PROJECT_PRODUCT_NAME}
        COMPANY_NAME ${PROJECT_COMPANY_NAME}
        BUNDLE_ID ${PROJECT_BUNDLE_ID}
)

target_sources(ravennakit_diagnostics PRIVATE
        source/Main.cpp
        source/MainComponent.h
        source/MainComponent.cpp
)

target_compile_definitions(ravennakit_diagnostics PUBLIC
        ${JUCE_COMMON_TARGET_DEFINITIONS}
        PRODUCT_NAME="$<TARGET_PROPERTY:ravennakit_diagnostics,JUCE_PRODUCT_NAME>"
)

if (WIN32)
    target_compile_definitions(ravennakit_diagnostics PRIVATE
            _CRT_SECURE_NO_WARNINGS
            WIN32_LEAN_AND_MEAN
    )
endif ()

target_link_libraries(ravennakit_diagnostics
        PUBLIC
        juce::juce_recommended_warning_flags
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags

        PRIVATE
        # JUCE
        juce::juce_core
        juce::juce_gui_extra
        juce::juce_audio_utils
        ravennakit
)
