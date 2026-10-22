# Core Architecture & Invariants

ESP32 CYD Wi-Fi Analyzer: interactive 2.4 GHz spectrum analyzer and network cataloging tool for ESP32 Cheap Yellow Display (CYD) hardware family.

## Modules & Structure
- `src/main.cpp`: Firmware setup & main loop, non-blocking scan intervals, LVGL tick dispatch.
- `src/ui.cpp` & `include/ui.h`: LVGL v8 UI with Tabview (Spectrum, Networks, Channels, Settings) and AP detail modal.
- `src/settings_manager.cpp` & `include/settings_manager.h`: NVS-backed persistence (`Preferences`) for theme, brightness, auto-dim, scan intervals, orientation.
- `src/backlight_manager.cpp` & `include/backlight_manager.h`: Hardware PWM backlight fading and LDR auto-dimming.
- `src/wifi_analyzer.cpp` & `include/wifi_analyzer.h`: Asynchronous background Wi-Fi scanning and channel interference rating.
- `config/config.h`: Default constants, pin definitions, Catppuccin flavor identifiers.
