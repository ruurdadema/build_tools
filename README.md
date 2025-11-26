# RAVENNAKIT JUCE Demo

## Introduction

This is the source code for a JUCE based demo of RAVENNAKIT, a set of tools and libraries to develop applications for
RAVENNA Audio over IP technology.

## Prerequisites

To build and run this project you will need to install the following external dependencies:

- Recent version of Git
- CMake (see CMakeLists.txt for required version)
- Python 3.9 (https://www.python.org/downloads/)
- Windows: Visual Studio (Community | Professional | Enterprise) 2019 or higher
- macOS: Xcode 12 or higher
- macOS: Homebrew

## Dependencies

This project requires the following dependencies:

- [RAVENNAKIT](https://github.com/soundondigital/ravennakit) (AGPLv3 or commercial license)
- [JUCE](https://github.com/juce-framework/JUCE), managed using CMake FetchContent (AGPLv3 or commercial license)

### RAVENNAKIT dependency

If you are using the distribution package of this demo (and not the repository), you will need to download 
RAVENNAKIT separately and place the source inside the ravennakit folder.

## Prepare the build environment

### macOS

Install vcpkg dependencies:

```
brew install pkg-config
```

Install build.py dependencies:

```
pip install pygit2
```

## How to build the project for macOS using CMake

```
# From the project root (generates build folder if necessary)
cmake -B xcode-build -G Xcode \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_NUMBER=0 \
    -DCMAKE_TOOLCHAIN_FILE=ravennakit/submodules/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 \
    -DVCPKG_OVERLAY_TRIPLETS=ravennakit/triplets \
    -DVCPKG_TARGET_TRIPLET=macos-$(uname -m) \
    -DCMAKE_OSX_ARCHITECTURES=$(uname -m)
cmake --build xcode-build --config Release
```

Note: adjust the parameters to match your environment.

## How to build the project for Windows using CMake

```
# From the project root (generates build folder if necessary)
cmake -B win-build -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="ravennakit\submodules\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_OVERLAY_TRIPLETS="ravennakit\triplets" -DVCPKG_TARGET_TRIPLET=windows-x64
cmake --build win-build --config Release
```

Note: adjust the parameters to match your environment.

## How to configure CLion to use vcpkg

Add the following CMake options:

```
-DCMAKE_TOOLCHAIN_FILE=ravennakit/submodules/vcpkg/scripts/buildsystems/vcpkg.cmake
-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
-DVCPKG_OVERLAY_TRIPLETS=../ravennakit/triplets
-DVCPKG_TARGET_TRIPLET=macos-arm64 
```

Note: change the triplet to match your cpu architecture.
