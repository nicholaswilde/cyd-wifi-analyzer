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
    TEST_ASSERT_TRUE(sm.getLedEnabled());
    TEST_ASSERT_EQUAL(DEFAULT_LED_BRIGHTNESS, sm.getLedBrightness());
    TEST_ASSERT_EQUAL(DEFAULT_AP_SORT_MODE, sm.getApSortMode());
    TEST_ASSERT_EQUAL(DEFAULT_AP_FILTER_MODE, sm.getApFilterMode());
    TEST_ASSERT_FALSE(sm.getScanSurveyEnabled());
    TEST_ASSERT_EQUAL(DEFAULT_REG_DOMAIN, sm.getRegDomain());
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

    sm.setLedEnabled(false);
    TEST_ASSERT_FALSE(sm.getLedEnabled());

    sm.setLedBrightness(180);
    TEST_ASSERT_EQUAL(180, sm.getLedBrightness());

    // Test LED brightness clamping
    sm.setLedBrightness(-20);
    TEST_ASSERT_EQUAL(0, sm.getLedBrightness());
    sm.setLedBrightness(300);
    TEST_ASSERT_EQUAL(255, sm.getLedBrightness());

    sm.setApSortMode(2);
    TEST_ASSERT_EQUAL(2, sm.getApSortMode());
    // AP sort clamping: 0..2
    sm.setApSortMode(-5);
    TEST_ASSERT_EQUAL(0, sm.getApSortMode());
    sm.setApSortMode(10);
    TEST_ASSERT_EQUAL(2, sm.getApSortMode());

    sm.setApFilterMode(3);
    TEST_ASSERT_EQUAL(3, sm.getApFilterMode());
    // AP filter clamping: 0..3
    sm.setApFilterMode(-2);
    TEST_ASSERT_EQUAL(0, sm.getApFilterMode());
    sm.setApFilterMode(8);
    TEST_ASSERT_EQUAL(3, sm.getApFilterMode());

    sm.setScanSurvey(true);
    TEST_ASSERT_TRUE(sm.getScanSurveyEnabled());

    sm.setRegDomain(REG_DOMAIN_EU);
    TEST_ASSERT_EQUAL(REG_DOMAIN_EU, sm.getRegDomain());
    // Regulatory domain clamping: 0..REG_DOMAIN_COUNT-1
    sm.setRegDomain(-3);
    TEST_ASSERT_EQUAL(REG_DOMAIN_US, sm.getRegDomain());
    sm.setRegDomain(10);
    TEST_ASSERT_EQUAL(REG_DOMAIN_JP, sm.getRegDomain());

    // Simulate reboot: new instance calling begin()
    SettingsManager rebootedSm;
    rebootedSm.begin();

    TEST_ASSERT_EQUAL(CATPPUCCIN_LATTE, rebootedSm.getThemeFlavor());
    TEST_ASSERT_EQUAL(100, rebootedSm.getBrightness());
    TEST_ASSERT_TRUE(rebootedSm.getAutoBrightness());
    TEST_ASSERT_EQUAL_UINT32(500, rebootedSm.getScanIntervalMs());
    TEST_ASSERT_TRUE(rebootedSm.getPassiveScan());
    TEST_ASSERT_EQUAL(3, rebootedSm.getScreenOrientation());
    TEST_ASSERT_FALSE(rebootedSm.getLedEnabled());
    TEST_ASSERT_EQUAL(255, rebootedSm.getLedBrightness());
    TEST_ASSERT_EQUAL(2, rebootedSm.getApSortMode());
    TEST_ASSERT_EQUAL(3, rebootedSm.getApFilterMode());
    TEST_ASSERT_TRUE(rebootedSm.getScanSurveyEnabled());
    TEST_ASSERT_EQUAL(REG_DOMAIN_JP, rebootedSm.getRegDomain());
}

void test_settings_reset_to_defaults(void) {
    SettingsManager sm;
    sm.setThemeFlavor(CATPPUCCIN_FRAPPE);
    sm.setBrightness(40);
    sm.setAutoBrightness(true);
    sm.setLedEnabled(false);
    sm.setLedBrightness(50);
    sm.setApSortMode(1);
    sm.setApFilterMode(2);
    sm.setScanSurvey(true);
    sm.setRegDomain(REG_DOMAIN_EU);

    sm.resetToDefaults();

    TEST_ASSERT_EQUAL(DEFAULT_THEME_FLAVOR, sm.getThemeFlavor());
    TEST_ASSERT_EQUAL(100, sm.getBrightness());
    TEST_ASSERT_FALSE(sm.getAutoBrightness());
    TEST_ASSERT_TRUE(sm.getLedEnabled());
    TEST_ASSERT_EQUAL(DEFAULT_LED_BRIGHTNESS, sm.getLedBrightness());
    TEST_ASSERT_EQUAL(DEFAULT_AP_SORT_MODE, sm.getApSortMode());
    TEST_ASSERT_EQUAL(DEFAULT_AP_FILTER_MODE, sm.getApFilterMode());
    TEST_ASSERT_FALSE(sm.getScanSurveyEnabled());
    TEST_ASSERT_EQUAL(DEFAULT_REG_DOMAIN, sm.getRegDomain());

    // Verify NVS cleared
    SettingsManager rebootedSm;
    rebootedSm.begin();
    TEST_ASSERT_EQUAL(DEFAULT_THEME_FLAVOR, rebootedSm.getThemeFlavor());
    TEST_ASSERT_EQUAL(100, rebootedSm.getBrightness());
    TEST_ASSERT_TRUE(rebootedSm.getLedEnabled());
    TEST_ASSERT_EQUAL(DEFAULT_LED_BRIGHTNESS, rebootedSm.getLedBrightness());
    TEST_ASSERT_EQUAL(DEFAULT_AP_SORT_MODE, rebootedSm.getApSortMode());
    TEST_ASSERT_EQUAL(DEFAULT_AP_FILTER_MODE, rebootedSm.getApFilterMode());
    TEST_ASSERT_FALSE(rebootedSm.getScanSurveyEnabled());
    TEST_ASSERT_EQUAL(DEFAULT_REG_DOMAIN, rebootedSm.getRegDomain());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_settings_initial_defaults);
    RUN_TEST(test_settings_setters_and_persistence);
    RUN_TEST(test_settings_reset_to_defaults);
    return UNITY_END();
}
