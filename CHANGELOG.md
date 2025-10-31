# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [unreleased]

### Fixed

- Fixed a crash caused by an assertion when receiving PTP management messages (and when `RAV_ABORT_ON_ASSERT` was
  enabled). Now PTP management and signalling messages are ignored.
- Fixed TTL not being updated. TTL is now updated for both unicast and multicast datagrams. TTL is clamped to [0, 255].

## [v0.17.0] - October 27, 2025

### Added

- Logs are now written to files.
- Added a way of generating a diagnostics package.
- A page with developer tools (which can be enabled with the environment variable RAV_ENABLE_DEVELOPER_TOOLS).

### Fixed

- Removed some realtime unsafe branches from realtime paths.

## [v0.16.0] - October 16, 2025

### Added

- Added drift correction between the local audio device and the PTP clock domain. Streaming won't have glitches due to
  resyncs.
- Added sample rate conversion to senders and receivers which are configured at a different sample rate than the local
  audio device.

## [v0.15.0] - September 8, 2025

### Added

- Added and enabled ASIO (Steinberg) for much better performance. Note that this library requires a license from
  Steinberg.

### Changed

- Only check if PTP is locked when first aligning the audio device.

## [v0.14.0] - September 7, 2025

### Added

- Added a tab showing PTP status information.
- Added the ability to configure the PTP domain.
- Added the version info (under settings)

## v0.13.0 - September 18, 2025
