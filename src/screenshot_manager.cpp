#include "screenshot_manager.h"
#include "sd_card_manager.h"
#include "ui.h"
#include <cstring>
#include <cstdio>

#ifndef NATIVE_TEST
#include <FS.h>
#include <SD.h>
using fs::File;
#else
#include "SD.h"
#include "FS.h"
#endif

static File _sdFile;

bool ScreenshotManager::_captureInProgress = false;
ScreenshotFormat ScreenshotManager::_format = FORMAT_RLE16;
ScreenshotDestination ScreenshotManager::_destination = DEST_SERIAL;
uint32_t ScreenshotManager::_width = 320;
uint32_t ScreenshotManager::_height = 240;
String ScreenshotManager::_serialBuffer = "";
std::string ScreenshotManager::_currentFilename = "";

uint16_t ScreenshotManager::_rlePixel = 0;
uint8_t ScreenshotManager::_rleCount = 0;
bool ScreenshotManager::_rleHasPending = false;

void ScreenshotManager::init() {
    _captureInProgress = false;
    _format = FORMAT_RLE16;
    _destination = DEST_SERIAL;
    _width = 320;
    _height = 240;
    _serialBuffer = "";
    _currentFilename = "";
    _rlePixel = 0;
    _rleCount = 0;
    _rleHasPending = false;
}

void ScreenshotManager::checkSerial() {
#ifndef NATIVE_TEST
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            if (_serialBuffer.length() > 0) {
                handleCommand(_serialBuffer);
                _serialBuffer = "";
            }
        } else if (c >= 32 && c <= 126 && _serialBuffer.length() < 64) {
            _serialBuffer += c;
        }
    }
#endif
}

void ScreenshotManager::handleCommand(const String& cmd) {
    String trimmed = cmd;
    trimmed.trim();
    if (trimmed.length() == 0) return;

    if (trimmed.equalsIgnoreCase("screenshot") || trimmed.equalsIgnoreCase("screenshot rle")) {
        startCapture(FORMAT_RLE16);
    } else if (trimmed.equalsIgnoreCase("screenshot raw") || trimmed.equalsIgnoreCase("screenshot bmp")) {
        startCapture(FORMAT_BMP24);
    } else if (trimmed.equalsIgnoreCase("screenshot sd")) {
        captureToSD();
    } else if (trimmed.startsWith("tab ") || trimmed.startsWith("TAB ")) {
        int tabIdx = trimmed.substring(4).toInt();
        setTab((uint16_t)tabIdx);
#ifndef NATIVE_TEST
        Serial.printf("[Screenshot] Switched to tab %d\n", tabIdx);
#endif
    } else if (trimmed.equalsIgnoreCase("modal") || trimmed.equalsIgnoreCase("modal open")) {
        openDetailModal();
#ifndef NATIVE_TEST
        Serial.println("[Screenshot] Opened detail modal");
#endif
    } else if (trimmed.equalsIgnoreCase("modal close")) {
        closeDetailModal();
#ifndef NATIVE_TEST
        Serial.println("[Screenshot] Closed detail modal");
#endif
    } else if (trimmed.equalsIgnoreCase("tracker") || trimmed.equalsIgnoreCase("tracker open")) {
        openTrackerDemo();
#ifndef NATIVE_TEST
        Serial.println("[Screenshot] Opened tracker view");
#endif
    } else if (trimmed.equalsIgnoreCase("tracker close")) {
        closeTrackerView();
#ifndef NATIVE_TEST
        Serial.println("[Screenshot] Closed tracker view");
#endif
    }
}

std::string ScreenshotManager::generateFilename(const struct tm* timeinfo, unsigned long fallbackMillis) {
    char filename[64];
    if (timeinfo) {
        strftime(filename, sizeof(filename), "/screenshot_%Y%m%d_%H%M%S.bmp", timeinfo);
    } else {
        snprintf(filename, sizeof(filename), "/screenshot_%lu.bmp", fallbackMillis);
    }
    return std::string(filename);
}

bool ScreenshotManager::startCapture(ScreenshotFormat format) {
    if (_captureInProgress) return false;

    _captureInProgress = true;
    _destination = DEST_SERIAL;
    _format = format;
    _rleHasPending = false;
    _rleCount = 0;

#ifndef NATIVE_TEST
    if (lv_disp_get_default()) {
        _width = lv_disp_get_hor_res(lv_disp_get_default());
        _height = lv_disp_get_ver_res(lv_disp_get_default());
    } else {
        _width = 320;
        _height = 240;
    }

    if (_format == FORMAT_RLE16) {
        Serial.printf("\n>>>SCREENSHOT_START:WIDTH=%u:HEIGHT=%u:FORMAT=RLE16<<<\n", _width, _height);
    } else {
        Serial.printf("\n>>>SCREENSHOT_START:WIDTH=%u:HEIGHT=%u:FORMAT=BMP24<<<\n", _width, _height);
        BmpHeader hdr = createHeader(_width, _height);
        Serial.write(hdr.bytes, sizeof(hdr.bytes));
    }

    // Force synchronous full-screen redraw through my_disp_flush
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(NULL);

    endCapture();
#endif
    return true;
}

bool ScreenshotManager::beginCaptureToSD(const char* filepath) {
    if (_captureInProgress) {
        Serial.println("[Screenshot] Error: Capture already in progress.");
        return false;
    }

    if (!SdCardManager::begin()) {
        Serial.println("[Screenshot] Error: SD card not mounted.");
        return false;
    }

    if (filepath && strlen(filepath) > 0) {
        _currentFilename = filepath;
    } else {
        _currentFilename = generateFilename(nullptr, millis());
    }

#ifndef NATIVE_TEST
    _sdFile = SD.open(_currentFilename.c_str(), FILE_WRITE);
#else
    _sdFile = SD.open(_currentFilename.c_str(), "w");
#endif

    if (!_sdFile) {
        Serial.printf("[Screenshot] Error: Failed to open %s for writing.\n", _currentFilename.c_str());
        return false;
    }

#ifndef NATIVE_TEST
    if (lv_disp_get_default()) {
        _width = lv_disp_get_hor_res(lv_disp_get_default());
        _height = lv_disp_get_ver_res(lv_disp_get_default());
    } else {
        _width = 320;
        _height = 240;
    }
#else
    _width = 320;
    _height = 240;
#endif

    // Write BMP header (54 bytes)
    BmpHeader hdr = createHeader(_width, _height);
    _sdFile.write(hdr.bytes, sizeof(hdr.bytes));

    // Pre-fill pixel area with zeros so random-access seek/write per tile functions properly
    uint8_t zeroBuf[480 * 3];
    memset(zeroBuf, 0, sizeof(zeroBuf));
    for (uint32_t r = 0; r < _height; r++) {
        _sdFile.write(zeroBuf, _width * 3);
    }

    _captureInProgress = true;
    _destination = DEST_SD;
    Serial.printf("[Screenshot] SD capture started: %s\n", _currentFilename.c_str());
    return true;
}

bool ScreenshotManager::captureToSD(const char* filepath) {
    if (!beginCaptureToSD(filepath)) {
        return false;
    }

#ifndef NATIVE_TEST
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(NULL);
#else
    // Simulate tiles covering frame for tests
    uint16_t tile[320 * 120] = {0};
    onFlushTile(0, 0, 319, 119, tile);
    onFlushTile(0, 120, 319, 239, tile);
#endif

    endCapture();
    return true;
}

void ScreenshotManager::rleFlushRun() {
    if (_rleHasPending && _rleCount > 0) {
        uint8_t packet[3];
        packet[0] = _rleCount;
        packet[1] = (uint8_t)(_rlePixel & 0xFF);
        packet[2] = (uint8_t)((_rlePixel >> 8) & 0xFF);
#ifndef NATIVE_TEST
        Serial.write(packet, 3);
#endif
        _rleCount = 0;
        _rleHasPending = false;
    }
}

void ScreenshotManager::rlePushPixel(uint16_t pixel) {
    if (!_rleHasPending) {
        _rlePixel = pixel;
        _rleCount = 1;
        _rleHasPending = true;
    } else if (pixel == _rlePixel && _rleCount < 255) {
        _rleCount++;
    } else {
        rleFlushRun();
        _rlePixel = pixel;
        _rleCount = 1;
        _rleHasPending = true;
    }
}

void ScreenshotManager::onFlushTile(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t* pixels) {
    if (!_captureInProgress || !pixels) return;

    uint32_t tileW = (uint32_t)(x2 - x1 + 1);
    uint32_t tileH = (uint32_t)(y2 - y1 + 1);

    if (_destination == DEST_SD) {
        if (!_sdFile) return;
        uint8_t rowBuf[480 * 3];
        for (uint32_t r = 0; r < tileH; r++) {
            const uint16_t* srcRow = pixels + r * tileW;
            for (uint32_t c = 0; c < tileW; c++) {
                uint8_t red, green, blue;
                convertRGB565ToBGR24(srcRow[c], red, green, blue);
                rowBuf[c * 3]     = blue;
                rowBuf[c * 3 + 1] = green;
                rowBuf[c * 3 + 2] = red;
            }
            uint32_t absRow = (uint32_t)y1 + r;
            uint32_t fileOff = 54 + absRow * _width * 3 + (uint32_t)x1 * 3;
            _sdFile.seek(fileOff);
            _sdFile.write(rowBuf, tileW * 3);
        }
        return;
    }

    if (_format == FORMAT_RLE16) {
        uint32_t totalPixels = tileW * tileH;
        for (uint32_t i = 0; i < totalPixels; i++) {
            rlePushPixel(pixels[i]);
        }
    } else {
        // Raw 24-bit BGR streaming row by row
        uint8_t rowBuf[480 * 3];
        for (uint32_t r = 0; r < tileH; r++) {
            const uint16_t* src = pixels + r * tileW;
            for (uint32_t c = 0; c < tileW; c++) {
                uint8_t red, green, blue;
                convertRGB565ToBGR24(src[c], red, green, blue);
                rowBuf[c * 3]     = blue;
                rowBuf[c * 3 + 1] = green;
                rowBuf[c * 3 + 2] = red;
            }
#ifndef NATIVE_TEST
            Serial.write(rowBuf, tileW * 3);
#endif
        }
    }
}

void ScreenshotManager::endCapture() {
    if (!_captureInProgress) return;

    if (_destination == DEST_SD) {
        if (_sdFile) {
            _sdFile.close();
        }
        Serial.printf("[Screenshot] SD capture saved: %s\n", _currentFilename.c_str());
#ifdef NATIVE_TEST
        extern bool mock_sd_card_mounted;
        mock_sd_card_mounted = false;
#endif
    } else {
        if (_format == FORMAT_RLE16) {
            rleFlushRun();
        }
#ifndef NATIVE_TEST
        Serial.print("\n>>>SCREENSHOT_END<<<\n");
        Serial.flush();
#endif
    }

    _captureInProgress = false;
}

bool ScreenshotManager::isCaptureInProgress() {
    return _captureInProgress;
}

BmpHeader ScreenshotManager::createHeader(uint32_t width, uint32_t height) {
    BmpHeader header;
    memset(header.bytes, 0, sizeof(header.bytes));

    uint32_t imageSize = width * height * 3;
    uint32_t fileSize  = 54 + imageSize;

    // BMP file header (14 bytes)
    header.bytes[0]  = 'B';
    header.bytes[1]  = 'M';
    header.bytes[2]  = (uint8_t)(fileSize & 0xFF);
    header.bytes[3]  = (uint8_t)((fileSize >> 8) & 0xFF);
    header.bytes[4]  = (uint8_t)((fileSize >> 16) & 0xFF);
    header.bytes[5]  = (uint8_t)((fileSize >> 24) & 0xFF);
    header.bytes[10] = 54; // Pixel data offset

    // DIB header (BITMAPINFOHEADER, 40 bytes)
    header.bytes[14] = 40;

    // Width (positive)
    header.bytes[18] = (uint8_t)(width & 0xFF);
    header.bytes[19] = (uint8_t)((width >> 8) & 0xFF);
    header.bytes[20] = (uint8_t)((width >> 16) & 0xFF);
    header.bytes[21] = (uint8_t)((width >> 24) & 0xFF);

    // Height: NEGATIVE = top-down row order
    int32_t negHeight = -(int32_t)height;
    header.bytes[22] = (uint8_t)(negHeight & 0xFF);
    header.bytes[23] = (uint8_t)((negHeight >> 8) & 0xFF);
    header.bytes[24] = (uint8_t)((negHeight >> 16) & 0xFF);
    header.bytes[25] = (uint8_t)((negHeight >> 24) & 0xFF);

    // Color planes (1)
    header.bytes[26] = 1;
    // Bits per pixel (24)
    header.bytes[28] = 24;

    // Image size
    header.bytes[34] = (uint8_t)(imageSize & 0xFF);
    header.bytes[35] = (uint8_t)((imageSize >> 8) & 0xFF);
    header.bytes[36] = (uint8_t)((imageSize >> 16) & 0xFF);
    header.bytes[37] = (uint8_t)((imageSize >> 24) & 0xFF);

    return header;
}

void ScreenshotManager::convertRGB565ToBGR24(uint16_t rgb565, uint8_t& r, uint8_t& g, uint8_t& b) {
    uint8_t rawR = (rgb565 >> 11) & 0x1F;
    uint8_t rawG = (rgb565 >> 5)  & 0x3F;
    uint8_t rawB =  rgb565        & 0x1F;

    r = (uint8_t)((rawR * 255) / 31);
    g = (uint8_t)((rawG * 255) / 63);
    b = (uint8_t)((rawB * 255) / 31);
}

uint16_t ScreenshotManager::convertBGR24ToRGB565(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t rawR = (uint16_t)((r * 31 + 127) / 255);
    uint16_t rawG = (uint16_t)((g * 63 + 127) / 255);
    uint16_t rawB = (uint16_t)((b * 31 + 127) / 255);
    return (uint16_t)((rawR << 11) | (rawG << 5) | rawB);
}

size_t ScreenshotManager::compressRLE16(const uint16_t* pixels, size_t count, uint8_t* outBuf, size_t maxOut) {
    if (!pixels || count == 0 || !outBuf || maxOut < 3) return 0;
    size_t outIdx = 0;
    size_t inIdx = 0;

    while (inIdx < count) {
        uint16_t curPix = pixels[inIdx];
        uint8_t runLen = 1;
        while (inIdx + runLen < count && pixels[inIdx + runLen] == curPix && runLen < 255) {
            runLen++;
        }
        if (outIdx + 3 > maxOut) break;
        outBuf[outIdx++] = runLen;
        outBuf[outIdx++] = (uint8_t)(curPix & 0xFF);
        outBuf[outIdx++] = (uint8_t)((curPix >> 8) & 0xFF);
        inIdx += runLen;
    }
    return outIdx;
}

size_t ScreenshotManager::decompressRLE16(const uint8_t* inBuf, size_t inSize, uint16_t* outPixels, size_t maxPixels) {
    if (!inBuf || inSize < 3 || !outPixels || maxPixels == 0) return 0;
    size_t inIdx = 0;
    size_t outIdx = 0;

    while (inIdx + 2 < inSize && outIdx < maxPixels) {
        uint8_t count = inBuf[inIdx++];
        uint16_t pixel = (uint16_t)(inBuf[inIdx] | (inBuf[inIdx + 1] << 8));
        inIdx += 2;

        for (uint8_t i = 0; i < count && outIdx < maxPixels; i++) {
            outPixels[outIdx++] = pixel;
        }
    }
    return outIdx;
}
