# CYD WiFi Analyzer
[![Coveralls](https://img.shields.io/badge/dynamic/xml?url=https%3A%2F%2Fcoveralls.io%2Frepos%2Fgithub%2Fnicholaswilde%2Fcyd-wifi-analyzer%2Fbadge.svg%3Fbranch%3Dmain&query=%2F%2F*%5Blocal-name()%3D'text'%5D%5Blast()%5D&label=Coveralls&style=for-the-badge&logo=coveralls)](https://coveralls.io/github/nicholaswilde/cyd-wifi-analyzer?branch=main)
[![task](https://img.shields.io/badge/Task-Enabled-brightgreen?style=for-the-badge&logo=task&logoColor=white)](https://taskfile.dev/#/)

> [!WARNING]
> This project is currently in a `v0.X.X` development stage. Features and configurations are subject to change, and breaking changes may be introduced at any time.

## :star: Features

## :hammer_and_wrench: Hardware Requirements

- **ESP32 Cheap Yellow Device (CYD)**:
  - **CYD 2.8" (Resistive)**: ESP32-2432S028R — 2.8″ 320×240 ILI9341 LCD with XPT2046 resistive touch.
  - **CYD 2.8" (Capacitive)**: ESP32-JC2432W328C — 2.8″ 320×240 ST7789 LCD with CST816 capacitive touch.
  - **CYD 2.8" NodeMiner / Rockbase (Resistive)**: ESP32-2432S028 ("NM") — 2.8″ 320×240 ST7789 LCD with XPT2046 resistive touch.
  - **CYD 3.5" (Capacitive)**: ESP32-3248S035C — 3.5″ 480×320 ST7796 LCD with GT911/CST820 capacitive touch.

## :rocket: Getting Started

### 1. Quick Install (Pre-compiled Binaries)

### 2. Build from Source

#### Secrets Setup

#### Configuration

#### Build & Upload

```bash
task build    # Compile firmware
task upload   # Flash to the connected CYD board
task monitor  # Open serial monitor (115200 baud)
```

---

## :gear: Settings Tab Reference

## :wrench: Troubleshooting

If you encounter any issues with your screen or the software, please review the solutions below or [create an issue](https://github.com/nicholaswilde/cyd-weather-station/issues) on GitHub if you are still stuck.

### Inverted Colors
If the colors on your display appear inverted (for example, dark themes like `Mocha`, `Macchiato`, or `Frappe` display with bright white backgrounds while the light `Latte` theme appears dark), this is due to hardware display controller differences between CYD batches. You can easily resolve it by:
- Flashing the alternative inversion release for your board (e.g., `cyd_28r` vs `cyd_28r_inv`, `cyd_28c` vs `cyd_28c_inv`, `cyd_28_nm` vs `cyd_28_nm_inv`, or `cyd_35c` vs `cyd_35c_inv`).
- Or, if building from source, switching to the corresponding `_inv` environment in PlatformIO or toggling the `-D TFT_INVERSION_ON=1` build flag.

### Garbled or Scrambled Screen
If the UI renders distorted, noisy, or with stretched fonts across the display, ensure you flashed the firmware matching your exact display size and controller:
- **2.8" Resistive (ILI9341)**: `cyd_28r` / `cyd_28r_inv` (240x320)
- **2.8" Capacitive (ESP32-2432W328C)**: `cyd_28c` / `cyd_28c_inv` (240x320)
- **2.8" Resistive NodeMiner / ST7789 (`ESP32-2432S028`)**: `cyd_28_nm` / `cyd_28_nm_inv` (240x320)
- **3.5" Capacitive (ST7796)**: `cyd_35c` / `cyd_35c_inv` (320x480)

Flashing a 3.5" image onto a 2.8" display (or vice versa) will result in garbled screens due to framebuffer and resolution mismatches.

### Partial Screen Rendering / 80px Noise Band (1/4 Screen)
If the UI displays across only 240 columns while the right 80 columns show uninitialized static noise, your board likely has an **ST7789** controller or an alternative **ILI9341 clone** controller (common on `ESP32-2432S028` boards manufactured under "NM" / NodeMiner or 2-USB variants). Pre-configured environments `cyd_28_nm` and `cyd_28_nm_inv` are available for these boards directly in `platformio.ini`. See the detailed guide in [docs/hardware-variants.md](docs/hardware-variants.md) for background and build configurations (`ILI9341_2_DRIVER` or `ST7789_DRIVER`).

### Flashing, Erasing Flash & Boot Reset
- **Erasing Flash**: Running a full `erase_flash` wipes all non-volatile storage (NVS), which erases previously saved Wi-Fi credentials and configuration. The device will reboot into Access Point (`192.168.4.1`) setup mode. Reflashing without erasing preserves your Wi-Fi credentials and saved runtime settings.
- **Bootloader Mode**: If flashing manually via serial (`esptool.py`), the ESP32 might remain in bootloader mode after writing. Press the physical **RESET / EN** button (or power cycle the unit) to boot into the application.

### RGB / BGR Swap
If your screen has red and blue colors swapped, it means the display expects a BGR color order instead of RGB. A dedicated release is not currently provided for this variation, but you can fix it by building from source: simply append `-D TFT_RGB_ORDER=TFT_BGR` to your environment's `build_flags` in `platformio.ini`.

### Touch Calibration (Resistive Screens)
The 2.8" resistive version (`cyd_28r`) uses a resistive touch layer which can sometimes be misaligned or mapped to inverted coordinates depending on the batch. If touches are registering in the wrong place, you may need to run a touch calibration sketch to determine the correct offsets for your specific screen. Capacitive variants (`cyd_28c`, `cyd_35c`) do not require calibration.

### Backlight Pin and Polarity Differences
If your screen is completely black but the board seems to be running, it might be a backlight pin mapping issue. While most boards use GPIO 21 or 27 for the backlight (`TFT_BL`), some obscure batches might map it differently. Furthermore, some boards require `TFT_BACKLIGHT_ON=LOW` instead of `HIGH`. 

### Different Driver Chips
Most 2.8" screens use the `ILI9341` driver. However, occasionally manufacturers substitute it with an `ST7789V` driver without changing the board's appearance. This will result in a completely blank or noisy screen until you change the driver flag in your `platformio.ini` from `-D ILI9341_DRIVER=1` to `-D ST7789_DRIVER=1`.

### Power Delivery Glitches
Intermittent touch issues, screen flickering, or spontaneous reboots are frequently caused by inadequate power. Ensure you are using a high-quality USB power supply and a good USB cable that can supply the burst current the ESP32 and backlight need simultaneously.

## :balance_scale: License

[Apache License 2.0](LICENSE)

## :writing_hand: Author

This project was started in 2026 by [Nicholas Wilde](https://github.com/nicholaswilde/).
