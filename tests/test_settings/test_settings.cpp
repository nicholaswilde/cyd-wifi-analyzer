#include <unity.h>
#include "Arduino.h"
#include "../mocks/mocks.cpp"
#include "../../src/settings_manager.cpp"
#include "settings_manager.h"

void setUp(void) {}
void tearDown(void) {}

void test_settings_initial_defaults(void) {
    Preferences prefs;
    prefs.begin("wifi_analyzer", false);
    prefs.clear();
    prefs.end();

    SettingsManager sm;
    sm.begin();

    TEST_ASSERT_EQUAL(DEFAULT_THEME_FLAVOR, sm.getThemeFlavor());
    TEST_ASSERT_EQUAL(100, sm.getBrightness());
    TEST_ASSERT_FALSE(sm.getAutoBrightness());
    TEST_ASSERT_EQUAL_UINT32(SCAN_INTERVAL_MS, sm.getScanIntervalMs());
    TEST_ASSERT_FALSE(sm.getPassiveScan());
    TEST_ASSERT_EQUAL(DEFAULT_SCREEN_ORIENTATION, sm.getScreenOrientation());
}

void test_settings_setters_and_persistence(void) {
    Preferences prefs;
    prefs.begin("wifi_analyzer", false);
    prefs.clear();
    prefs.end();

    SettingsManager sm;
    sm.begin();

    sm.setThemeFlavor(CATPPUCCIN_LATTE);
    TEST_ASSERT_EQUAL(CATPPUCCIN_LATTE, sm.getThemeFlavor());

    sm.setBrightness(75);
    TEST_ASSERT_EQUAL(75, sm.getBrightness());

    // Test brightness clamping
    sm.setBrightness(5); // Below min (10)
    TEST_ASSERT_EQUAL(10, sm.getBrightness());
    sm.setBrightness(120); // Above max (100)
    TEST_ASSERT_EQUAL(100, sm.getBrightness());

    sm.setAutoBrightness(true);
    TEST_ASSERT_TRUE(sm.getAutoBrightness());

    sm.setScanIntervalMs(5000);
    TEST_ASSERT_EQUAL_UINT32(5000, sm.getScanIntervalMs());

    // Clamping scan interval >= 500ms
    sm.setScanIntervalMs(200);
    TEST_ASSERT_EQUAL_UINT32(500, sm.getScanIntervalMs());

    sm.setPassiveScan(true);
    TEST_ASSERT_TRUE(sm.getPassiveScan());

    sm.setScreenOrientation(3);
    TEST_ASSERT_EQUAL(3, sm.getScreenOrientation());

    // Simulate reboot: new instance calling begin()
    SettingsManager rebootedSm;
    rebootedSm.begin();

    TEST_ASSERT_EQUAL(CATPPUCCIN_LATTE, rebootedSm.getThemeFlavor());
    TEST_ASSERT_EQUAL(100, rebootedSm.getBrightness());
    TEST_ASSERT_TRUE(rebootedSm.getAutoBrightness());
    TEST_ASSERT_EQUAL_UINT32(500, rebootedSm.getScanIntervalMs());
    TEST_ASSERT_TRUE(rebootedSm.getPassiveScan());
    TEST_ASSERT_EQUAL(3, rebootedSm.getScreenOrientation());
}

void test_settings_reset_to_defaults(void) {
    SettingsManager sm;
    sm.setThemeFlavor(CATPPUCCIN_FRAPPE);
    sm.setBrightness(40);
    sm.setAutoBrightness(true);

    sm.resetToDefaults();

    TEST_ASSERT_EQUAL(DEFAULT_THEME_FLAVOR, sm.getThemeFlavor());
    TEST_ASSERT_EQUAL(100, sm.getBrightness());
    TEST_ASSERT_FALSE(sm.getAutoBrightness());

    // Verify NVS cleared
    SettingsManager rebootedSm;
    rebootedSm.begin();
    TEST_ASSERT_EQUAL(DEFAULT_THEME_FLAVOR, rebootedSm.getThemeFlavor());
    TEST_ASSERT_EQUAL(100, rebootedSm.getBrightness());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_settings_initial_defaults);
    RUN_TEST(test_settings_setters_and_persistence);
    RUN_TEST(test_settings_reset_to_defaults);
    return UNITY_END();
}
