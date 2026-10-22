---
name: screen-reader
description: Skill to capture, inspect, and visually analyze live CYD device screens over USB serial for UI debugging, layout verification, and design system adherence.
---

# Screen Reader & Visual Analyzer Skill

This skill guides the agent in capturing, inspecting, and visually auditing live CYD (Cheap Yellow Display) device screens via USB serial. It enables full multimodal feedback loops to debug LVGL layouts, verify Catppuccin palette tokens, and diagnose typography clipping on real hardware.

---

## 1. Prerequisites

1. The CYD device must be connected to the host via a USB data cable (typically `/dev/ttyUSB0` or `/dev/ttyACM0`).
2. The device must be running firmware compiled with `ScreenshotManager` (included in `main`).
3. No active serial monitor sessions (such as `task monitor` or `minicom`) may hold the serial port lock.

---

## 2. Capture Workflows

All captures are executed via the `task screenshot` runner using `run_command`:

### Capture Current Screen
```bash
task screenshot
```
* Captures whatever screen is currently active on the CYD.
* Outputs to `screenshots/screenshot_YYYYMMDD_HHMMSS.png`.
* Automatically updates the shortcut file `screenshots/latest.png`.

### Switch to a Specific Tab and Capture
```bash
task screenshot -- -t <index>
```
Tab index mapping:
* `-t 0`: **Spectrum Tab** (parabolic channel curves, peak apex dBm badges, clean channels banner)
* `-t 1`: **Networks Tab** (scrollable AP catalog with signal bars and security badges)
* `-t 2`: **Channels Tab** (14-channel interference and rating matrix)
* `-t 3`: **Settings Tab** (touch settings, sliders, toggle switches, and reset button)

### Batch Capture All Tabs
```bash
task screenshot -- --all
```
Automatically switches through each of the 4 tabs in sequence and saves:
* `screenshots/spectrum.png`
* `screenshots/networks.png`
* `screenshots/channels.png`
* `screenshots/settings.png`

### Optional Flags
* `-p <port>` / `--port <port>`: Specify explicit serial port (e.g. `task screenshot -- -p /dev/ttyUSB1`).
* `--raw`: Stream uncompressed 24-bit BMP instead of high-speed RLE16.

---

## 3. Reading and Analyzing the Screenshot

Once the capture completes:

1. **Read the image file** using the `read_file` tool:
   ```json
   { "Path": "/home/nicholas/git/nicholaswilde/cyd-wifi-analyzer/screenshots/latest.png" }
   ```
2. **Audit visual criteria** against [`.agents/design.md`](file:///home/nicholas/git/nicholaswilde/cyd-wifi-analyzer/.agents/design.md):
   * **Typography & Wrapping**:
     - Check for awkward mid-word or multi-line wraps in labels (e.g., table cells, SSID tags, apex dBm markers).
     - Ensure numbers and units (e.g. `dBm`, `%`, `Ch1`) remain together.
   * **Alignment & Spacing**:
     - Verify table columns have adequate breathing room and don't collide with the right-hand scrollbar.
     - Check that chart axes, ticks, and legends align cleanly to gridlines.
   * **Catppuccin Color Tokens**:
     - Verify text colors adhere to `Text` (`#cdd6f4`), `Subtext0` (`#a6adc8`), and `Mauve` / `Blue` accents.
     - Verify channel colors match their assigned palette tokens (e.g., Pink for Ch 1, Green for Ch 4, Lavender for Ch 11).
   * **Touch Targets**:
     - Verify interactive controls (switches, buttons, sliders) provide at least $32 \times 32\text{ px}$ of touchable surface.

---

## 4. Iteration and Verification Cycle

When modifying UI code in [`src/ui.cpp`](file:///home/nicholas/git/nicholaswilde/cyd-wifi-analyzer/src/ui.cpp):

1. **Apply edits** to [`src/ui.cpp`](file:///home/nicholas/git/nicholaswilde/cyd-wifi-analyzer/src/ui.cpp).
2. **Run native regression tests**:
   ```bash
   rtk test pio test -e native
   ```
3. **Flash to hardware**:
   ```bash
   task upload:cyd_28r
   ```
4. **Wait 2–3 seconds** for the ESP32 bootloader and setup to initialize.
5. **Re-capture the affected tab**:
   ```bash
   task screenshot -- -t <index>
   ```
6. **Read and verify** the updated screenshot with `read_file` to confirm the fix.

---

## 5. Troubleshooting Capture Failures

* **`TimeoutError: Device did not respond with >>>SCREENSHOT_START`**:
  - The board may have been flashed with older firmware that lacks `ScreenshotManager`. Run `task upload:cyd_28r` to flash the latest build.
  - Opening the serial port can toggle the CH340 DTR pin, briefly rebooting the ESP32. Allow the device 2–3 seconds to complete `setup()`.
* **Port Busy (`Device or resource busy`)**:
  - A running `task monitor` or background serial process is holding `/dev/ttyUSB0`. Terminate the serial monitor before taking a screenshot.
