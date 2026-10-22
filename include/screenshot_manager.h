#ifndef SCREENSHOT_MANAGER_H
#define SCREENSHOT_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <string>

#ifndef NATIVE_TEST
#include <Arduino.h>
#include <lvgl.h>
#else
#include "Arduino.h"
#include "lvgl.h"
#endif

struct BmpHeader {
    uint8_t bytes[54];
};

enum ScreenshotFormat {
    FORMAT_RLE16,
    FORMAT_BMP24
};

enum ScreenshotDestination {
    DEST_SERIAL,
    DEST_SD
};

class ScreenshotManager {
public:
    static void init();
    static void checkSerial();
    static void handleCommand(const String& cmd);
    static bool startCapture(ScreenshotFormat format = FORMAT_RLE16);
    static void onFlushTile(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t* pixels);
    static void endCapture();
    static bool isCaptureInProgress();

    // BMP utility methods
    static BmpHeader createHeader(uint32_t width, uint32_t height);
    static void convertRGB565ToBGR24(uint16_t rgb565, uint8_t& r, uint8_t& g, uint8_t& b);
    static uint16_t convertBGR24ToRGB565(uint8_t r, uint8_t g, uint8_t b);
    static std::string generateFilename(const struct tm* timeinfo, unsigned long fallbackMillis);

    // SD Card Capture methods
    static bool beginCaptureToSD(const char* filepath = nullptr);
    static bool captureToSD(const char* filepath = nullptr);

    // Buffer RLE compression for streaming and tests
    static size_t compressRLE16(const uint16_t* pixels, size_t count, uint8_t* outBuf, size_t maxOut);
    static size_t decompressRLE16(const uint8_t* inBuf, size_t inSize, uint16_t* outPixels, size_t maxPixels);

private:
    static bool _captureInProgress;
    static ScreenshotFormat _format;
    static ScreenshotDestination _destination;
    static uint32_t _width;
    static uint32_t _height;
    static String _serialBuffer;
    static std::string _currentFilename;

    // Streaming RLE state
    static uint16_t _rlePixel;
    static uint8_t _rleCount;
    static bool _rleHasPending;

    static void rleFlushRun();
    static void rlePushPixel(uint16_t pixel);
};

#endif // SCREENSHOT_MANAGER_H
