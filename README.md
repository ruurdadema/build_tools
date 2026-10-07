# Build Tools

Tools for helping with building, packing &amp; signing software products because as devs we already suffer enough.

## CMake

Allows to invoke CMake from a python script.

## macOS

### Universal binaries

Facilities for working with universal binaries on macOS.

### Signing

Facilities for code signing on both macOS and Windows.

### ProductBuilder (creating installers) 

A simple tool for creating installers on macOS using product builder.

## Windows

### Innosetup

A simple tool for generating and building Innosetup scripts.

### Signtool

Helpers for finding (the newest) signtool, verifying signatures and building signtool options for Azure Artifact
Signing or a GlobalSign EV token.

### Azure Artifact Signing

`artifact_signing_setup()` downloads the Artifact Signing dlib from NuGet and writes the `metadata.json` signtool needs.
Requires the .NET 8 (or newer) x64 runtime and the `AZURE_TENANT_ID`, `AZURE_CLIENT_ID` and `AZURE_CLIENT_SECRET`
environment variables.
