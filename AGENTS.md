# Project Rules & Guidelines

## Codebase Lookup Preference
- When developing features or debugging, always reference the starting point implementation at:
  `/home/nicholas/git/AndroidCrypto/ESP32_CYD_WiFi_Analyzer/`
- For hardware configurations, CYD pinouts, display drivers, touch setup, and overall project shape, always reference:
  `/home/nicholas/git/nicholaswilde/cyd-weather-station/`
- For theme definitions and color tokens, reference:
  `/home/nicholas/git/nicholaswilde/catppuccin-rgb565/` (upstream: https://github.com/nicholaswilde/catppuccin-rgb565)
- Additional hardware pinout reference:
  `/home/nicholas/git/witnessmenow/ESP32-Cheap-Yellow-Display` and `/home/nicholas/git/BruceDevices/firmware`

## Build and Test Commands
- Build firmware: `pio run -e cyd_28r` (or `cyd_28c`, `cyd_35c`)
- Run host-native tests: `pio test -e native`

## RTK Command Guidelines
- **Git Operations**: Prefix `git` commands with `rtk` (e.g., `rtk git status`, `rtk git diff`, `rtk git log`, `rtk git commit`, `rtk git push`).
- **GitHub CLI**: Prefix `gh` commands with `rtk` (e.g., `rtk gh issue list | cat`, `rtk gh pr status | cat`). Always pipe `gh` commands to `cat` to bypass interactive pagers.
- **File & Directory Inspection**: Use `rtk ls`, `rtk tree`, `rtk find`, or `rtk read` when listing or reading files to get token-optimized output.
- **Searching**: Use `rtk grep` or `rtk rg` for line search pattern matching.
- **Build & Test Outputs**: Use `rtk err` or `rtk test` when running build/test commands to filter output to errors/failures only (e.g. `rtk test pio test -e native`).

## What To Do Next
- When asked "what to do next" (or similar), **always check the remote repository issues first** using `gh`:
  ```bash
  rtk gh issue list | cat
  ```

## Issue Creation
- When asked to create an issue, use your best guess to determine if it is a new feature or a bug fix.
- Prefix the issue title with `[feat]: <description>` or `[bug]: <description>`.
- Add the `enhancement` or `bug` label to the issue accordingly using the `--label` flag with the `gh` command.

## Issue Resolution & Testing
- When addressing or fixing GitHub issues, **always** create corresponding regression tests along with the issue implementations or fixes to prevent future regressions.

## Adding Settings Checklist
- Whenever adding a new setting or configuration option to the application, ensure it is consistently added across all integration points:
  1. **Default Config / Constants**: Add default definitions to `include/config/config.h`.
  2. **Settings Manager**: Add getters, setters, NVS keys, and default fallback values in `include/settings_manager.h` and `src/settings_manager.cpp`.
  3. **On-Device Settings UI**: Add the UI control (toggle switch, slider, or dropdown) in `src/ui.cpp` (Tab 4: Settings) and synchronize values in `initUI()` and `refreshThemeUI()`.
  4. **Subsystem Wiring**: Wire the setting value into the respective consumer subsystem (e.g., `backlight_manager`, `wifi_analyzer`, `display`, or `catppuccin`).
  5. **Unit Tests**: Add test assertions in `tests/test_settings/test_settings.cpp` verifying defaults, bounds clamping, simulated reboot persistence, and factory reset behavior.

## Testing cyd_28c and cyd_28c_inv Variants
- The user does **not** have physical access to the JC2432W328C board (https://www.amazon.com/dp/B0D5H84RDB).
- When developing or testing for the `cyd_28c` and `cyd_28c_inv` environments, do not attempt to flash the device directly.
- Instead, always use the `package-binaries` skill to generate firmware ZIP files (partitions.bin, firmware.bin, bootloader.bin) so the user can send them to a tester:
  ```bash
  bash .agents/skills/package-binaries/package_binaries.sh cyd_28c
  ```

## Context-Mode MCP Integration
This project uses `context-mode` for token-optimized command execution, file inspection, and searching.
- **Rules Reference**: See [`.agents/rules/context-mode-rules.md`](.agents/rules/context-mode-rules.md).

## Serena Semantic Code Navigation & LSP
This project uses Serena for semantic C/C++ code navigation, AST symbol inspection, and LSP diagnostics.
- **Rules Reference**: See [`.agents/rules/serena-rules.md`](.agents/rules/serena-rules.md) for tool mappings, refactoring protocols, and CodeGraph boundaries.
- **Compilation Database**: Clangd uses `compile_commands.json` (generated via `pio run -t compiledb`).

## Design System & Visual Guidelines
This project follows the DESIGN.md format specification (https://github.com/google-labs-code/design.md) for its visual identity, Catppuccin color tokens, typography scales, touch targets, and LVGL UI components.
- **Design System Reference**: See [`.agents/design.md`](.agents/design.md).

## Product Guide & Features

### Overview
The **ESP32 CYD Wi-Fi Analyzer** is an interactive, touch-screen-enabled 2.4 GHz spectrum analyzer and network cataloging tool designed for the **ESP32 Cheap Yellow Display (CYD)** hardware family. It scans, visualizes, and diagnoses nearby Wi-Fi networks in real time using the **LVGL (Light and Versatile Graphics Library)** for a modern, responsive, and visually appealing interface styled with the **Catppuccin** color palette.

### Target Hardware
- **Display**: ESP32-2432S028R Cheap Yellow Display (CYD) 2.8" TFT touchscreen (typically ILI9341 driver + XPT2046 resistive touch controller), 2.8" capacitive (JC2432W328C), and 3.5" capacitive (ESP32-3248S035C).
- **Core**: ESP32 dual-core Xtensa 32-bit LX6 processor with integrated 2.4 GHz 802.11 b/g/n Wi-Fi.
- **Sensors & Peripherals**: Onboard LDR light sensor (GPIO 34), PWM backlight control, and onboard active-low RGB LED.

### Key Features
1. **2.4 GHz Spectrum View**: Real-time parabolic channel curves illustrating RF spectral footprint across channels 1–14 with Catppuccin color coding and apex SSID/dBm peak tags.
2. **Access Point Catalog**: Scrollable network list displaying SSID, BSSID, RSSI, channel, and authentication mode (Open, WEP, WPA, WPA2, WPA3).
3. **Network Detail Modal**: Pop-up inspector for selected APs showing live signal bar, signal quality percentage, frequency, and an 8-sample rolling RSSI history line chart.
4. **Channel Rating & Co-channel Interference Table**: 14-channel matrix calculating AP density, peak dBm, and a cumulative co-channel/adjacent noise score to rate channels (Clean, Good, Fair, Congested).
5. **Catppuccin RGB565 Theming**: Full runtime switching across 4 Catppuccin flavors (`Mocha`, `Macchiato`, `Frappe`, `Latte`) with 14 distinct palette assignments per channel.
6. **Persistent Configuration via NVS**: `SettingsManager` backed by ESP32 `Preferences` storing theme flavor, brightness, auto-dimming toggle, scan interval, passive/active scan mode, and screen orientation across reboots.
7. **LDR Ambient Light Auto-Dimming**: Automatic backlight adjustment responding to room lighting with smooth PWM fading via `BacklightManager`.
8. **Multi-Board CYD Support**: Out-of-the-box pinout and driver support for resistive (`cyd_28r`), 2.8" capacitive (`cyd_28c`), and 3.5" capacitive (`cyd_35c`) CYD variants.
9. **Interactive On-Device Settings Tab**: Live controls for theme flavor, backlight slider, auto-bright switch, scan interval selector, passive scan switch, orientation toggle, and factory defaults reset.

### Product Guidelines (UI/UX, Performance, Reliability)

#### 1. User Interface (UI) & User Experience (UX) Design
- **Resolution Constraint**: All screens must fit a fixed 320x240 pixel landscape screen (CYD 2.8" default; also supports 320x480 / 480x320 for 3.5" boards).
- **Typography**: Clear, sans-serif fonts compatible with LVGL (e.g., standard Montserrat fonts built into LVGL: `lv_font_montserrat_10`, `lv_font_montserrat_14`, etc.).
- **Touch Targets**: Interactive elements (buttons, sliders, dropdowns, tabs) must have a minimum touch area of 32x32 pixels (preferably ≥40x40 pixels) for reliable resistive and capacitive touchscreen control.
- **Layout and Navigation**: Clean tabbed layout using LVGL Tabview (Spectrum, Networks, Channels, Settings). Redraw only changed elements to maintain smooth touch responsiveness.

#### 2. Technical and Performance Guidelines
- **Non-blocking Wi-Fi Scanning**: Asynchronous background Wi-Fi channel scanning without blocking LVGL render loops or freezing touch handling.
- **Memory Optimization**: Static LVGL draw buffers and pre-allocated vector reserves to prevent heap fragmentation.
- **NVS Safety**: Clamping and fallback guards on all persisted user configurations to ensure clean recovery from uninitialized or corrupted storage.

#### 3. Reliability & Error Recovery
- **Graceful Handling of Empty Scans**: Clear status messaging when no networks are detected or when scanning is in progress.
- **Safe Reset**: One-touch factory defaults restoration restoring safe display, brightness, and scan parameters immediately.

## Technology Stack & Hardware Architecture

### 1. Hardware Target
- **Microcontroller**: ESP32-2432S028R / JC2432W328C / ESP32-3248S035C
- **Display**: 2.8" / 3.5" TFT (ILI9341, ST7789, ST7796)
- **Touch**: XPT2046 (resistive), CST816 / GT911 (capacitive)
- **Ambient Light Sensor**: Onboard LDR photoresistor (GPIO 34)
- **Backlight Control**: PWM via LEDC (GPIO 21 or GPIO 27)

### 2. Core Software Stack
- **Language**: C/C++
- **Framework**: Arduino Framework (PlatformIO `framework = arduino`)
- **GUI Engine**: LVGL v8 (`lvgl`)
- **Color Library**: Catppuccin RGB565 (`catppuccin-rgb565`)
- **Graphics & Touch Drivers**: `TFT_eSPI`, `XPT2046_Touchscreen`, `Wire`
- **Preferences Storage**: ESP32 `Preferences` library (NVS)

### 3. Build, Execution, and Tooling
- **Build System**: PlatformIO Core CLI (`platformio.ini`)
- **Task Runner**: `go-task` (`Taskfile.yml`)
- **Testing**: PlatformIO native test runner (`pio test -e native`) with Unity test framework

## Code Style Principles
- **Readability**: Code should be easy to read and understand. Avoid overly clever or obscure constructs.
- **Consistency**: Follow existing patterns in the codebase. Maintain consistent formatting, naming, and structure.
- **Simplicity**: Prefer simple solutions over complex ones. Break down complex problems into smaller, manageable parts.
- **Maintainability**: Write code that is easy to modify and extend. Minimize dependencies and coupling.
- **Documentation**: Document *why* something is done, not just *what*. Keep documentation up-to-date with code changes.
