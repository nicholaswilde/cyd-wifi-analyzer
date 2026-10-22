#include <unity.h>
#include "Arduino.h"
#include "../mocks/mocks.cpp"
#include "screenshot_manager.h"
#include "../../src/screenshot_manager.cpp"
#include "ui.h"

// Mock setTab / getTab for native testing
static uint16_t s_mock_current_tab = 0;
void setTab(uint16_t index) { s_mock_current_tab = index; }
uint16_t getTab() { return s_mock_current_tab; }

void setUp(void) {
    ScreenshotManager::init();
    s_mock_current_tab = 0;
}

void tearDown(void) {
    ScreenshotManager::endCapture();
}

void test_bmp_header_creation(void) {
    BmpHeader hdr = ScreenshotManager::createHeader(320, 240);

    // Signature 'BM'
    TEST_ASSERT_EQUAL_UINT8('B', hdr.bytes[0]);
    TEST_ASSERT_EQUAL_UINT8('M', hdr.bytes[1]);

    // Total file size: 54 + 320 * 240 * 3 = 230454
    uint32_t fileSize = (uint32_t)hdr.bytes[2] |
                        ((uint32_t)hdr.bytes[3] << 8) |
                        ((uint32_t)hdr.bytes[4] << 16) |
                        ((uint32_t)hdr.bytes[5] << 24);
    TEST_ASSERT_EQUAL_UINT32(230454, fileSize);

    // Pixel data offset = 54
    uint32_t offset = (uint32_t)hdr.bytes[10] | ((uint32_t)hdr.bytes[11] << 8);
    TEST_ASSERT_EQUAL_UINT32(54, offset);

    // DIB header size = 40
    uint32_t dibSize = (uint32_t)hdr.bytes[14] | ((uint32_t)hdr.bytes[15] << 8);
    TEST_ASSERT_EQUAL_UINT32(40, dibSize);

    // Width = 320
    uint32_t width = (uint32_t)hdr.bytes[18] | ((uint32_t)hdr.bytes[19] << 8);
    TEST_ASSERT_EQUAL_UINT32(320, width);

    // Negative Height = -240 (top-down row order)
    int32_t height = (int32_t)((uint32_t)hdr.bytes[22] |
                               ((uint32_t)hdr.bytes[23] << 8) |
                               ((uint32_t)hdr.bytes[24] << 16) |
                               ((uint32_t)hdr.bytes[25] << 24));
    TEST_ASSERT_EQUAL_INT32(-240, height);

    // Color planes = 1, bpp = 24
    uint16_t planes = (uint16_t)hdr.bytes[26] | ((uint16_t)hdr.bytes[27] << 8);
    uint16_t bpp = (uint16_t)hdr.bytes[28] | ((uint16_t)hdr.bytes[29] << 8);
    TEST_ASSERT_EQUAL_UINT16(1, planes);
    TEST_ASSERT_EQUAL_UINT16(24, bpp);

    // Image size = 320 * 240 * 3 = 230400
    uint32_t imgSize = (uint32_t)hdr.bytes[34] |
                       ((uint32_t)hdr.bytes[35] << 8) |
                       ((uint32_t)hdr.bytes[36] << 16) |
                       ((uint32_t)hdr.bytes[37] << 24);
    TEST_ASSERT_EQUAL_UINT32(230400, imgSize);
}

void test_color_conversion_rgb565_to_bgr24(void) {
    uint8_t r, g, b;

    // Red: 0xF800
    ScreenshotManager::convertRGB565ToBGR24(0xF800, r, g, b);
    TEST_ASSERT_EQUAL_UINT8(255, r);
    TEST_ASSERT_EQUAL_UINT8(0, g);
    TEST_ASSERT_EQUAL_UINT8(0, b);

    // Green: 0x07E0
    ScreenshotManager::convertRGB565ToBGR24(0x07E0, r, g, b);
    TEST_ASSERT_EQUAL_UINT8(0, r);
    TEST_ASSERT_EQUAL_UINT8(255, g);
    TEST_ASSERT_EQUAL_UINT8(0, b);

    // Blue: 0x001F
    ScreenshotManager::convertRGB565ToBGR24(0x001F, r, g, b);
    TEST_ASSERT_EQUAL_UINT8(0, r);
    TEST_ASSERT_EQUAL_UINT8(0, g);
    TEST_ASSERT_EQUAL_UINT8(255, b);

    // Black: 0x0000
    ScreenshotManager::convertRGB565ToBGR24(0x0000, r, g, b);
    TEST_ASSERT_EQUAL_UINT8(0, r);
    TEST_ASSERT_EQUAL_UINT8(0, g);
    TEST_ASSERT_EQUAL_UINT8(0, b);

    // White: 0xFFFF
    ScreenshotManager::convertRGB565ToBGR24(0xFFFF, r, g, b);
    TEST_ASSERT_EQUAL_UINT8(255, r);
    TEST_ASSERT_EQUAL_UINT8(255, g);
    TEST_ASSERT_EQUAL_UINT8(255, b);

    // Roundtrip conversion
    uint16_t reconstructed = ScreenshotManager::convertBGR24ToRGB565(255, 128, 64);
    uint8_t r2, g2, b2;
    ScreenshotManager::convertRGB565ToBGR24(reconstructed, r2, g2, b2);
    TEST_ASSERT_INT_WITHIN(8, 255, r2);
    TEST_ASSERT_INT_WITHIN(8, 128, g2);
    TEST_ASSERT_INT_WITHIN(8, 64, b2);
}

void test_rle16_compression_and_decompression_uniform(void) {
    const size_t COUNT = 320;
    uint16_t src[COUNT];
    for (size_t i = 0; i < COUNT; i++) {
        src[i] = 0x1E1E; // Catppuccin Base fill
    }

    uint8_t compressed[1024];
    size_t compSize = ScreenshotManager::compressRLE16(src, COUNT, compressed, sizeof(compressed));

    // 320 pixels: run 1 = 255 (3 bytes), run 2 = 65 (3 bytes) -> 6 bytes total!
    TEST_ASSERT_EQUAL_UINT32(6, compSize);
    TEST_ASSERT_EQUAL_UINT8(255, compressed[0]);
    TEST_ASSERT_EQUAL_UINT8(65, compressed[3]);

    uint16_t decompressed[COUNT];
    size_t decompCount = ScreenshotManager::decompressRLE16(compressed, compSize, decompressed, COUNT);
    TEST_ASSERT_EQUAL_UINT32(COUNT, decompCount);

    for (size_t i = 0; i < COUNT; i++) {
        TEST_ASSERT_EQUAL_HEX16(src[i], decompressed[i]);
    }
}

void test_rle16_compression_and_decompression_mixed(void) {
    const size_t COUNT = 600;
    uint16_t src[COUNT];
    for (size_t i = 0; i < 200; i++) src[i] = 0xF800;       // 200 Red
    for (size_t i = 200; i < 300; i++) src[i] = 0x07E0;     // 100 Green
    for (size_t i = 300; i < 600; i++) src[i] = 0x001F;     // 300 Blue (split into 255 + 45)

    uint8_t compressed[2048];
    size_t compSize = ScreenshotManager::compressRLE16(src, COUNT, compressed, sizeof(compressed));

    // 4 runs total: 200 Red (3B) + 100 Green (3B) + 255 Blue (3B) + 45 Blue (3B) = 12 bytes!
    TEST_ASSERT_EQUAL_UINT32(12, compSize);

    uint16_t decompressed[COUNT];
    size_t decompCount = ScreenshotManager::decompressRLE16(compressed, compSize, decompressed, COUNT);
    TEST_ASSERT_EQUAL_UINT32(COUNT, decompCount);

    for (size_t i = 0; i < COUNT; i++) {
        TEST_ASSERT_EQUAL_HEX16(src[i], decompressed[i]);
    }
}

void test_screenshot_command_parsing(void) {
    TEST_ASSERT_FALSE(ScreenshotManager::isCaptureInProgress());

    // Test tab navigation command
    ScreenshotManager::handleCommand("tab 2");
    TEST_ASSERT_EQUAL_UINT16(2, s_mock_current_tab);

    ScreenshotManager::handleCommand("tab 0");
    TEST_ASSERT_EQUAL_UINT16(0, s_mock_current_tab);

    // Test startCapture
    bool started = ScreenshotManager::startCapture(FORMAT_RLE16);
    TEST_ASSERT_TRUE(started);

    // End capture
    ScreenshotManager::endCapture();
    TEST_ASSERT_FALSE(ScreenshotManager::isCaptureInProgress());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_bmp_header_creation);
    RUN_TEST(test_color_conversion_rgb565_to_bgr24);
    RUN_TEST(test_rle16_compression_and_decompression_uniform);
    RUN_TEST(test_rle16_compression_and_decompression_mixed);
    RUN_TEST(test_screenshot_command_parsing);
    return UNITY_END();
}
