#!/usr/bin/env python3
"""
capture-serial-screenshot.py
----------------------------
Captures a screenshot from an ESP32 Cheap Yellow Display (CYD) over USB Serial.
Supports fast RLE16 stream compression (~1.5s transfer at 115200 baud) or raw BMP24.

Requirements:
    pyserial, pillow (runnable via: uv run --with pyserial --with pillow python ...)
"""

import sys
import os
import time
import argparse
import glob
from datetime import datetime

try:
    import serial
    import serial.tools.list_ports
    from PIL import Image
except ImportError:
    sys.stderr.write("Error: Missing required packages. Run with:\n")
    sys.stderr.write("uv run --with pyserial --with pillow python scripts/capture-serial-screenshot.py\n")
    sys.exit(1)


def find_serial_port() -> str:
    """Auto-detects the connected ESP32 CYD serial port."""
    ports = serial.tools.list_ports.comports()
    for p in ports:
        desc = (p.description or "").lower()
        hwid = (p.hwid or "").lower()
        if any(chip in desc or chip in hwid for chip in ["cp210", "ch340", "ch341", "ch9102", "ftdi", "usb serial"]):
            return p.device
    # Fallback to common Linux devices
    candidates = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    if candidates:
        return candidates[0]
    return ""


def rgb565_to_rgb888(pixel: int):
    r = ((pixel >> 11) & 0x1F) * 255 // 31
    g = ((pixel >> 5) & 0x3F) * 255 // 63
    b = (pixel & 0x1F) * 255 // 31
    return (r, g, b)


def capture_frame(ser: serial.Serial, cmd: str = "screenshot rle", timeout: float = 10.0) -> Image.Image:
    # Flush existing buffer
    ser.reset_input_buffer()
    
    # Send screenshot command
    ser.write(f"{cmd}\n".encode("ascii"))
    ser.flush()

    # Search for start delimiter
    start_time = time.time()
    header_line = ""
    while time.time() - start_time < timeout:
        line = ser.readline().decode("latin1", errors="ignore").strip()
        if ">>>SCREENSHOT_START:" in line:
            header_line = line
            break

    if not header_line:
        raise TimeoutError("Device did not respond with >>>SCREENSHOT_START within timeout.")

    # Parse header: >>>SCREENSHOT_START:WIDTH=320:HEIGHT=240:FORMAT=RLE16<<<
    parts = header_line.replace(">>>", "").replace("<<<", "").split(":")
    params = {}
    for part in parts[1:]:
        if "=" in part:
            k, v = part.split("=", 1)
            params[k.strip()] = v.strip()

    width = int(params.get("WIDTH", "320"))
    height = int(params.get("HEIGHT", "240"))
    fmt = params.get("FORMAT", "RLE16")

    total_pixels = width * height
    pixels = []

    if fmt == "RLE16":
        raw_bytes = bytearray()
        end_marker = b">>>SCREENSHOT_END<<<"
        
        while time.time() - start_time < (timeout + 15):
            chunk = ser.read(ser.in_waiting or 1)
            if chunk:
                raw_bytes.extend(chunk)
                if end_marker in raw_bytes:
                    break

        idx = raw_bytes.find(end_marker)
        if idx != -1:
            raw_data = raw_bytes[:idx]
        else:
            raw_data = raw_bytes

        # Strip any leading newline
        if raw_data.startswith(b"\n") or raw_data.startswith(b"\r"):
            raw_data = raw_data.lstrip(b"\r\n")

        # Decompress RLE: [count (1B), pixel_lo (1B), pixel_hi (1B)]
        ptr = 0
        while ptr + 2 < len(raw_data) and len(pixels) < total_pixels:
            count = raw_data[ptr]
            pixel = raw_data[ptr + 1] | (raw_data[ptr + 2] << 8)
            ptr += 3
            rgb = rgb565_to_rgb888(pixel)
            pixels.extend([rgb] * count)

    elif fmt == "BMP24":
        # Raw 24-bit BMP stream
        raw_bytes = bytearray()
        end_marker = b">>>SCREENSHOT_END<<<"
        while time.time() - start_time < (timeout + 25):
            chunk = ser.read(ser.in_waiting or 1)
            if chunk:
                raw_bytes.extend(chunk)
                if end_marker in raw_bytes:
                    break

        idx = raw_bytes.find(end_marker)
        bmp_data = raw_bytes[:idx] if idx != -1 else raw_bytes
        # 54 byte header, then BGR pixels
        if len(bmp_data) >= 54:
            bgr_bytes = bmp_data[54:]
            ptr = 0
            while ptr + 2 < len(bgr_bytes) and len(pixels) < total_pixels:
                b = bgr_bytes[ptr]
                g = bgr_bytes[ptr + 1]
                r = bgr_bytes[ptr + 2]
                ptr += 3
                pixels.append((r, g, b))

    if len(pixels) < total_pixels:
        # Pad remaining pixels with black if truncated
        pixels.extend([(0, 0, 0)] * (total_pixels - len(pixels)))
    elif len(pixels) > total_pixels:
        pixels = pixels[:total_pixels]

    img = Image.new("RGB", (width, height))
    img.putdata(pixels)
    return img


def main():
    parser = argparse.ArgumentParser(description="Capture CYD screen via USB Serial.")
    parser.add_argument("output", nargs="?", default="", help="Output image file path (PNG or BMP).")
    parser.add_argument("-p", "--port", default="", help="Serial port (auto-detected if omitted).")
    parser.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default: 115200).")
    parser.add_argument("-t", "--tab", type=int, default=-1, help="Switch to tab index (0=Spectrum, 1=Networks, 2=Channels, 3=Settings) before capture.")
    parser.add_argument("--modal", action="store_true", help="Open Network Detail Modal before capture.")
    parser.add_argument("--tracker", action="store_true", help="Open Tracker View before capture.")
    parser.add_argument("--all", action="store_true", help="Capture all 4 tabs, Detail Modal, and Tracker View in sequence.")
    parser.add_argument("--raw", action="store_true", help="Use raw 24-bit BMP instead of RLE compression.")
    args = parser.parse_args()

    port = args.port or find_serial_port()
    if not port:
        sys.stderr.write("Error: No CYD serial port found. Connect board or specify with -p /dev/ttyUSB0\n")
        sys.exit(1)

    os.makedirs("screenshots", exist_ok=True)
    fmt_cmd = "screenshot raw" if args.raw else "screenshot rle"

    print(f"Connecting to CYD on {port} @ {args.baud} baud...")
    with serial.Serial(port, args.baud, timeout=1.0) as ser:
        time.sleep(2.0)  # Allow DTR settling and ESP32 boot
        ser.reset_input_buffer()

        tab_names = ["spectrum", "networks", "channels", "settings"]

        if args.all:
            print("Capturing all screens in sequence...")
            # 1. Capture all 4 tabs
            for idx, name in enumerate(tab_names):
                print(f"Switching to Tab {idx} ({name})...")
                ser.write(f"tab {idx}\n".encode("ascii"))
                ser.flush()
                time.sleep(1.2)  # Allow LVGL redraw
                img = capture_frame(ser, fmt_cmd)
                out_path = f"screenshots/{name}.png"
                img.save(out_path)
                print(f"  Saved {out_path} ({img.width}x{img.height})")

            # 2. Capture Network Detail Modal
            print("Opening Network Detail Modal...")
            ser.write(b"modal\n")
            ser.flush()
            time.sleep(1.2)
            img = capture_frame(ser, fmt_cmd)
            out_path = "screenshots/modal.png"
            img.save(out_path)
            print(f"  Saved {out_path} ({img.width}x{img.height})")
            ser.write(b"modal close\n")
            ser.flush()
            time.sleep(0.6)

            # 3. Capture Tracker View
            print("Opening Tracker View...")
            ser.write(b"tracker\n")
            ser.flush()
            time.sleep(1.2)
            img = capture_frame(ser, fmt_cmd)
            out_path = "screenshots/tracker.png"
            img.save(out_path)
            print(f"  Saved {out_path} ({img.width}x{img.height})")
            ser.write(b"tracker close\n")
            ser.flush()
            time.sleep(0.6)

            # Return to Tab 0 (Spectrum)
            ser.write(b"tab 0\n")
            ser.flush()
            time.sleep(0.5)

            # Update latest shortcut to spectrum
            img_spec = Image.open("screenshots/spectrum.png")
            img_spec.save("screenshots/latest.png")

            print("All screens captured successfully.")
            return

        if args.modal:
            print("Opening Network Detail Modal...")
            ser.write(b"modal\n")
            ser.flush()
            time.sleep(1.2)
            img = capture_frame(ser, fmt_cmd)
            out_path = args.output or "screenshots/modal.png"
            img.save(out_path)
            img.save("screenshots/latest.png")
            print(f"Detail Modal saved to: {out_path}")
            ser.write(b"modal close\n")
            ser.flush()
            return

        if args.tracker:
            print("Opening Tracker View...")
            ser.write(b"tracker\n")
            ser.flush()
            time.sleep(1.2)
            img = capture_frame(ser, fmt_cmd)
            out_path = args.output or "screenshots/tracker.png"
            img.save(out_path)
            img.save("screenshots/latest.png")
            print(f"Tracker View saved to: {out_path}")
            ser.write(b"tracker close\n")
            ser.flush()
            return

        if args.tab >= 0:
            print(f"Switching to Tab {args.tab}...")
            ser.write(f"tab {args.tab}\n".encode("ascii"))
            ser.flush()
            time.sleep(1.2)

        print("Requesting screen capture...")
        img = capture_frame(ser, fmt_cmd)

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        out_path = args.output or f"screenshots/screenshot_{timestamp}.png"
        img.save(out_path)
        print(f"Screenshot successfully saved to: {out_path} ({img.width}x{img.height})")

        # Also update screenshots/latest.png for easy reference
        latest_path = "screenshots/latest.png"
        img.save(latest_path)
        print(f"Updated shortcut: {latest_path}")


if __name__ == "__main__":
    main()
