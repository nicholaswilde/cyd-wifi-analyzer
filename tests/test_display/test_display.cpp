#include <unity.h>
#include "../mocks/mocks.cpp"
#include "settings_manager.h"
#include "touch_manager.h"
#include "screenshot_manager.h"
#include "../../src/sd_card_manager.cpp"
#include "../../src/screenshot_manager.cpp"
#include "../../src/touch_manager.cpp"
#include "../../src/settings_manager.cpp"

// Mock UI stubs for screenshot_manager
static uint16_t s_mock_current_tab = 0;
void setTab(uint16_t index) { s_mock_current_tab = index; }
uint16_t getTab() { return s_mock_current_tab; }
void openDetailModal() {}
void closeDetailModal() {}
void openTrackerView(const String&, const String&, uint8_t) {}
void openTrackerDemo() {}
void closeTrackerView() {}

#include "../../src/display.cpp"

void setUp(void) {
    setMockCapacitive(false);
    setMockTouch(false, 0, 0);
}

void tearDown(void) {
    ScreenshotManager::endCapture();
}

void test_resistive_coordinate_mapping_landscape(void) {
    int16_t out_x, out_y;
    setMockCapacitive(false);

    // Orientation 1 (Landscape, 320x240)
    mapTouchCoordinates(200, 200, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(0, out_x);
    TEST_ASSERT_EQUAL(0, out_y);

    mapTouchCoordinates(3700, 3900, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(319, out_x);
    TEST_ASSERT_EQUAL(239, out_y);
}

void test_resistive_coordinate_mapping_landscape_rev(void) {
    int16_t out_x, out_y;
    setMockCapacitive(false);

    // Orientation 3 (Landscape Rev, 320x240)
    mapTouchCoordinates(200, 200, out_x, out_y, 3);
    TEST_ASSERT_EQUAL(319, out_x);
    TEST_ASSERT_EQUAL(239, out_y);

    mapTouchCoordinates(3700, 3900, out_x, out_y, 3);
    TEST_ASSERT_EQUAL(0, out_x);
    TEST_ASSERT_EQUAL(0, out_y);
}

void test_resistive_coordinate_mapping_portrait(void) {
    int16_t out_x, out_y;
    setMockCapacitive(false);

    // Orientation 0 (Portrait Rev, 240x320)
    mapTouchCoordinates(200, 200, out_x, out_y, 0);
    TEST_ASSERT_EQUAL(239, out_x);
    TEST_ASSERT_EQUAL(0, out_y);

    mapTouchCoordinates(3700, 3900, out_x, out_y, 0);
    TEST_ASSERT_EQUAL(0, out_x);
    TEST_ASSERT_EQUAL(319, out_y);
}

void test_resistive_coordinate_mapping_portrait_rev(void) {
    int16_t out_x, out_y;
    setMockCapacitive(false);

    // Orientation 2 (Portrait, 240x320)
    mapTouchCoordinates(200, 200, out_x, out_y, 2);
    TEST_ASSERT_EQUAL(0, out_x);
    TEST_ASSERT_EQUAL(319, out_y);

    mapTouchCoordinates(3700, 3900, out_x, out_y, 2);
    TEST_ASSERT_EQUAL(239, out_x);
    TEST_ASSERT_EQUAL(0, out_y);
}

void test_capacitive_coordinate_mapping_landscape(void) {
    int16_t out_x, out_y;
    setMockCapacitive(true);

    // Native portrait coords (x <= 240, y <= 320) -> Landscape (1)
    mapTouchCoordinates(50, 100, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(100, out_x);
    TEST_ASSERT_EQUAL(240 - 50, out_y);

    // Landscape orientation (1) with landscape raw coords
    mapTouchCoordinates(150, 120, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(120, out_x);
    TEST_ASSERT_EQUAL(240 - 150, out_y);
}

void test_capacitive_coordinate_mapping_landscape_rev(void) {
    int16_t out_x, out_y;
    setMockCapacitive(true);

    // Native portrait coords -> Landscape Rev (3)
    mapTouchCoordinates(50, 100, out_x, out_y, 3);
    TEST_ASSERT_EQUAL(320 - 100, out_x);
    TEST_ASSERT_EQUAL(50, out_y);
}

void test_coordinate_clamping_and_bounds(void) {
    int16_t out_x, out_y;
    setMockCapacitive(false);

    // Below min
    mapTouchCoordinates(-500, -500, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(0, out_x);
    TEST_ASSERT_EQUAL(0, out_y);

    // Above max
    mapTouchCoordinates(5000, 5000, out_x, out_y, 1);
    TEST_ASSERT_EQUAL(319, out_x);
    TEST_ASSERT_EQUAL(239, out_y);
}

void test_touch_manager_mock_and_callback(void) {
    TouchManager::begin();
    TEST_ASSERT_FALSE(TouchManager::isCapacitive());

    setMockCapacitive(true);
    TEST_ASSERT_TRUE(TouchManager::isCapacitive());

    setMockTouch(false, 0, 0);
    TEST_ASSERT_FALSE(TouchManager::isTouched());

    int tx = 0, ty = 0;
    TEST_ASSERT_FALSE(TouchManager::getTouchPoint(tx, ty));

    setMockTouch(true, 150, 200);
    TEST_ASSERT_TRUE(TouchManager::isTouched());
    TEST_ASSERT_TRUE(TouchManager::getTouchPoint(tx, ty));
    TEST_ASSERT_EQUAL(150, tx);
    TEST_ASSERT_EQUAL(200, ty);

    lv_indev_data_t data;
    my_touchpad_read(nullptr, &data);
    TEST_ASSERT_EQUAL(LV_INDEV_STATE_PR, data.state);

    setMockTouch(false, 0, 0);
    my_touchpad_read(nullptr, &data);
    TEST_ASSERT_EQUAL(LV_INDEV_STATE_REL, data.state);
}

void test_disp_flush_and_init(void) {
    initDisplayAndTouch();
    initLVGL();

    lv_disp_drv_t drv;
    lv_disp_drv_init(&drv);
    lv_disp_draw_buf_t dbuf;
    memset(&dbuf, 0, sizeof(dbuf));
    drv.draw_buf = &dbuf;

    lv_area_t area = {0, 0, 9, 9};
    lv_color_t color_buf[100];
    for (int i = 0; i < 100; ++i) {
        color_buf[i].full = 0xFFFF;
    }

    my_disp_flush(&drv, &area, color_buf);

    ScreenshotManager::startCapture();
    my_disp_flush(&drv, &area, color_buf);
    ScreenshotManager::endCapture();
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_resistive_coordinate_mapping_landscape);
    RUN_TEST(test_resistive_coordinate_mapping_landscape_rev);
    RUN_TEST(test_resistive_coordinate_mapping_portrait);
    RUN_TEST(test_resistive_coordinate_mapping_portrait_rev);
    RUN_TEST(test_capacitive_coordinate_mapping_landscape);
    RUN_TEST(test_capacitive_coordinate_mapping_landscape_rev);
    RUN_TEST(test_coordinate_clamping_and_bounds);
    RUN_TEST(test_touch_manager_mock_and_callback);
    RUN_TEST(test_disp_flush_and_init);
    return UNITY_END();
}
