## Network monitor and forecast update

- Add asynchronous gateway ICMP latency sampling and a 24-result graph.
- Track Wi-Fi disconnects, cumulative offline duration and three recent events in RAM.
- Add a separate daily forecast page with minimum/maximum temperature, maximum
  precipitation probability, forecast date and cache age for Porto Alegre.
- Expand K1/K2 navigation to seven pages; preserve buffered rendering.
- Validate daily forecast values and retain the last successful forecast on errors.
- Bound weather HTTP connection/read timeouts to three seconds each.

# Changelog

Consolidated changes for the ESP32-C3 variant developed in this project. Release numbers and historical dates are not assigned without a corresponding verified release. Last documentation update: **2026-10-07**.

## [Unreleased]

### Added

- Buffered display rendering (32 KB canvas) to reduce visible clearing between frames.
- Command console rotated 90 degrees clockwise from its previous orientation.
- Markdown command reference regenerated from the current firmware help.


- Five persistent SSID/password profiles with circular replacement and visible next-slot information.
- Startup scanning, sequential connection attempts, and configuration AP fallback.
- HTTP routes `/info` and `/wifi`.
- Clock, status, network, system, and saved-profile display pages.
- Interrupt-based Key1/BOOT page navigation and Key2 short/long-press actions.
- Manual local-time command `set_time:YYYY-MM-DD HH:MM:SS`.
- Deep-sleep command `desliga`, retaining profiles and returning through RESET.
- Adaptive Wi-Fi bars and a one-pixel trace of actual RSSI history.
- English GitHub documentation and a corrected Mermaid architecture diagram.

### Changed

- Wi-Fi bars now use green for RSSI at or above -67 dBm, yellow from -80 to below -67 dBm, and red below -80 dBm; disconnected bars remain gray.


- Ported ESP32-C6/ST7789 firmware to ESP32-C3/ST7735, with a 128 × 128 panel and board-specific pins.
- Resized clock, console, and OTA views for a 1.44-inch screen.
- Applied a global 90-degree counterclockwise display rotation.
- Adapted LED commands to the single-color onboard LED.
- Increased signal-bar sensitivity to small recent RSSI changes.
- Updated weather requests to the API endpoint and added one-minute retries before synchronization.
- Standardized the command list and documented circular profile behavior.

### Fixed

- Corrected `Preferences::putString` return-length checks and added read-back verification, including empty passwords.
- Corrected verification when migrating legacy Wi-Fi credentials.
- Restored required `false` arguments for finite blinking and DST deactivation.
- Separated the health report from command dispatch and restored helper methods.
- Aligned declarations and implementations after SD removal.
- Corrected network command dispatch previously nested under RSSI handling.
- Replaced the malformed quoted TFT label in Mermaid with a valid quoted node label.

### Removed

- SD storage, card-based logging, associated commands, and the SdFat dependency.
- SD-dependent `net_scan`, RGB color commands, and PSRAM commands.
- Redundant `init`, `heap`, `heap_min`, `flash_info`, and `selftest` commands; use `reason`, `ram`, `flash`, and `health`.

### Known limitations

- Backlight remains powered during deep sleep.
- No runtime switching between saved SSIDs or hidden-network discovery.
- No configured authentication for AP, HTTP, UDP, or OTA; weather HTTPS certificate validation is disabled.
- Weather age is based on the latest attempt timestamp.
- The latest package does not have a recorded complete build or hardware regression test.
