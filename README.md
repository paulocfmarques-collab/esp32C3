# ESP32-C3 Gateway

An ESP32-C3 gateway project with a 1.44" LCD, Wi-Fi provisioning portal, UDP command processing, NTP time sync, weather integration, RGB LED feedback, and OTA updates.

## Overview

This repository contains the firmware for an ESP32-C3 based gateway built around a compact TFT interface. The device provides:

- Wi-Fi connection management with up to 5 saved networks
- Web portal for network configuration and device information
- UDP command console
- NTP time synchronization with timezone and DST support
- Weather sync using Open-Meteo
- RGB LED status and animation feedback
- OTA firmware updates
- A dashboard on the LCD with clock, status, network, system, and saved Wi-Fi pages

## Main Files

- `ESP32C3.ino` - application entry point and device loop
- `ESP32Gateway.cpp` - Wi-Fi portal, web pages, UDP transport, and saved network handling
- `CommandProcessor.cpp` - command parser and device actions
- `DisplayUtil.cpp` - LCD rendering, dashboard pages, console, and Wi-Fi signal visualization
- `NTPUtil.cpp` - time synchronization, timezone persistence, and manual time setting
- `ClimaManager.h` - weather fetching and condition parsing
- `OTAManager.h` - OTA setup and progress screen
- `RGBLed.cpp` - RGB LED effects and helpers

## Features

### Display

- Clock screen with date, time, temperature, and weather condition
- Status page with CPU, RAM, uptime, and NTP state
- Network page with IP, SSID, channel, RSSI, MAC, and service ports
- System diagnostics page
- Saved Wi-Fi networks page
- Console-style command output on the LCD
- Idle matrix-style screensaver

### Connectivity

- Auto-connect to saved Wi-Fi networks
- Fallback access point: `ESP32_C3_CONFIG`
- HTTP interface:
  - `/info` for device details
  - `/wifi` for Wi-Fi management
- UDP command input for remote control

### Commands

Examples include:

- `help`
- `info`
- `status`
- `uptime`
- `reason`
- `version`
- `reboot`
- `desliga`
- `net_info`
- `reset_wifi`
- `set_fuso:X`
- `set_time:YYYY-MM-DD HH:MM:SS`
- `led_on`
- `led_off`
- `led_breath`
- `led_pisca:P:I`
- `led_blink:I`
- `clima`
- `clima_age`
- `clima_sync`
- `health`

## Hardware Notes

The repository is configured for an ESP32-C3 board with a small TFT display. The code uses the following fixed pins from the board configuration:

- LCD CS: GPIO 2
- LCD DC: GPIO 0
- LCD RESET: GPIO 5
- LCD MOSI: GPIO 4
- LCD SCK: GPIO 3
- RGB LED: GPIO 11
- Buttons: GPIO 8, GPIO 9, GPIO 10

## Arduino IDE Setup

- Install **ESP32 by Espressif Systems** 3.x
- Select **ESP32C3 Dev Module**
- Enable **USB CDC On Boot** if you want serial over native USB
- Install **GFX Library for Arduino (Arduino_GFX)**
- Use a partition scheme that supports OTA

## Behavior

- The device shows a dashboard by default once connected
- Long-press actions on the user button support reboot and Wi-Fi reset behavior
- OTA is enabled automatically when Wi-Fi is connected in station mode
- Weather updates are refreshed periodically and cached to reduce network usage

## Repository Language

This project is primarily written in **C++** for the ESP32 Arduino framework.

## Premium GitHub Edition

A cleaner, more polished GitHub-ready presentation of this project:

- Clear feature overview for visitors
- Organized file map for fast navigation
- Practical setup instructions
- Explicit hardware pin references
- Command summary for remote usage
- Better project positioning for contributors and users

## Contributing

Contributions are welcome. Please open an issue or pull request with improvements, bug fixes, or documentation updates.

## License

Add a license file if you want to define reuse terms for this project.
