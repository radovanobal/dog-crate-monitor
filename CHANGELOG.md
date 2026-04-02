# Changelog

All notable changes to this project will be documented in this file.

The format is based on Keep a Changelog, and this project follows a simple `Unreleased` first workflow for now.

## [Unreleased]

### Added
- Initial ESP-IDF project bring-up for the Waveshare ESP32-S3 3.97-inch e-paper board.
- Local component integration for the e-paper display, SHTC3 temperature and humidity sensor, and PCF85063 RTC.
- Basic on-device UI showing clock, temperature, and humidity.
- Grid- and region-based display layout helpers for positioning screen elements.
- Display module split into dedicated display source and header files.
- Environment module split into dedicated environment source and header files.

### Changed
- Refactored the application away from a single flat `main.c` toward separate orchestration, display, and environment modules.
- Improved runtime handling for failed environment reads by preserving cached sensor values.
- Added environment read status handling to distinguish between success, warning, and failure states.
- Moved full-versus-partial refresh selection into the display controller so the screen manager now focuses on lifecycle, routing, and generation-aware render forwarding.
- Locked the screen-to-display contract around dirty-only render plans, with `DisplayRenderPlan.count == 0` representing no visual update work.
- Refactored the home screen render path into separate derivation, dirty detection, and render-item emission phases using committed and next screen-local state.
- Updated the home screen to emit only changed render items and clear screen-local state on deactivation so reactivation starts from a fresh local screen lifetime.

### Fixed
- Corrected build integration issues while splitting the application into multiple source files.
- Resolved sensor driver usage to call the implemented SHTC3 API.
- Corrected clock string handling and safer RTC time formatting.