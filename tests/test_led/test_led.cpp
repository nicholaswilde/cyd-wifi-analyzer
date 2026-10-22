#include <unity.h>
#include "Arduino.h"
#include "../mocks/mocks.cpp"
#include "led_manager.h"
#include "../../src/led_manager.cpp"
#include "wifi_analyzer.h"
#include "../../src/wifi_analyzer.cpp"

void setUp(void) {
    mock_millis_val = 0;
    for (int i = 0; i < 100; i++) {
        mock_pin_modes[i] = 0;
        mock_pin_states[i] = HIGH; // active-low default off
    }
}

void tearDown(void) {
    // cleanup
}

void test_led_manager_initialization(void) {
    LedManager lm(4, 16, 17);
    lm.begin();

    // Verify pin modes configured as OUTPUT
    TEST_ASSERT_EQUAL(OUTPUT, mock_pin_modes[4]);
    TEST_ASSERT_EQUAL(OUTPUT, mock_pin_modes[16]);
    TEST_ASSERT_EQUAL(OUTPUT, mock_pin_modes[17]);

    // Active-LOW: unlit pins must be HIGH
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[4]);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[16]);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[17]);

    TEST_ASSERT_EQUAL(LedManager::STATE_OFF, lm.getState());
    TEST_ASSERT_TRUE(lm.isEnabled());
    TEST_ASSERT_EQUAL(255, lm.getBrightness());
}

void test_led_manager_scanning_blink(void) {
    LedManager lm(4, 16, 17);
    lm.begin();
    lm.setState(LedManager::STATE_SCANNING);

    // At t=0: Blink ON -> Blue active (active-low: LOW)
    lm.update(0);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[4]);  // Red off
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[16]); // Green off
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[17]);  // Blue on

    // At t=399: Still ON
    mock_millis_val = 399;
    lm.update(399);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[17]);

    // At t=400: Toggles OFF
    mock_millis_val = 400;
    lm.update(400);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[17]);

    // At t=800: Toggles ON again
    mock_millis_val = 800;
    lm.update(800);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[17]);
}

void test_led_manager_clean_solid_green(void) {
    LedManager lm(4, 16, 17);
    lm.begin();
    lm.setState(LedManager::STATE_CLEAN);

    mock_millis_val = 100;
    lm.update(100);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[4]);  // Red off
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[16]);  // Green on
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[17]); // Blue off

    // Steady state maintains green
    mock_millis_val = 5000;
    lm.update(5000);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[16]);
    TEST_ASSERT_EQUAL(LedManager::STATE_CLEAN, lm.getState());
}

void test_led_manager_congested_yellow_and_red(void) {
    LedManager lm(4, 16, 17);
    lm.begin();

    // Yellow: Red + Green on, Blue off
    lm.setState(LedManager::STATE_CONGESTED_YELLOW);
    lm.update(0);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[4]);   // Red on
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[16]);  // Green on
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[17]); // Blue off

    // Red: Red on, Green & Blue off
    lm.setState(LedManager::STATE_CONGESTED_RED);
    lm.update(50);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[4]);   // Red on
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[16]); // Green off
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[17]); // Blue off
}

void test_led_manager_pulses(void) {
    LedManager lm(4, 16, 17);
    lm.begin();

    // Pulse Blue (1000ms)
    lm.setState(LedManager::STATE_PULSE_BLUE);
    lm.update(0);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[4]);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[16]);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[17]);

    lm.update(1000);
    TEST_ASSERT_EQUAL(LedManager::STATE_OFF, lm.getState());
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[17]);

    // Pulse Green (1000ms)
    lm.setState(LedManager::STATE_PULSE_GREEN);
    lm.update(2000);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[4]);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[16]);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[17]);

    lm.update(3000);
    TEST_ASSERT_EQUAL(LedManager::STATE_OFF, lm.getState());
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[16]);

    // Pulse Yellow (1000ms)
    lm.setState(LedManager::STATE_PULSE_YELLOW);
    lm.update(4000);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[4]);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[16]);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[17]);

    lm.update(5000);
    TEST_ASSERT_EQUAL(LedManager::STATE_OFF, lm.getState());

    // Pulse White (1000ms)
    lm.setState(LedManager::STATE_PULSE_WHITE);
    lm.update(6000);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[4]);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[16]);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[17]);

    lm.update(7000);
    TEST_ASSERT_EQUAL(LedManager::STATE_OFF, lm.getState());
}

void test_led_manager_alert_red(void) {
    LedManager lm(4, 16, 17);
    lm.begin();
    lm.setState(LedManager::STATE_ALERT_RED);

    // Fast blink Red (200ms)
    lm.update(0);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[4]);   // Red on
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[16]);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[17]);

    lm.update(200);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[4]);  // Red off

    lm.update(400);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[4]);   // Red on
}

void test_led_manager_disabled_mode(void) {
    LedManager lm(4, 16, 17);
    lm.begin();
    lm.setState(LedManager::STATE_CLEAN);
    lm.update(0);
    TEST_ASSERT_EQUAL(LOW, mock_pin_states[16]);  // Green on

    // Disable LED
    lm.setEnabled(false);
    TEST_ASSERT_FALSE(lm.isEnabled());

    // Verify all pins forced off (HIGH)
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[4]);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[16]);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[17]);

    // Subsequent updates must keep pins off
    mock_millis_val = 500;
    lm.update(500);
    TEST_ASSERT_EQUAL(HIGH, mock_pin_states[16]);
}

void test_led_manager_brightness_clamping(void) {
    LedManager lm(4, 16, 17);
    lm.begin();

    lm.setBrightness(-10);
    TEST_ASSERT_EQUAL(0, lm.getBrightness());

    lm.setBrightness(400);
    TEST_ASSERT_EQUAL(255, lm.getBrightness());

    lm.setBrightness(128);
    TEST_ASSERT_EQUAL(128, lm.getBrightness());
}

void test_wifi_analyzer_environment_led_status(void) {
    WifiAnalyzer wa;

    // 0 APs: Clean -> Green
    std::vector<AccessPointInfo> emptyList;
    wa.setScanResults(emptyList);
    TEST_ASSERT_EQUAL(LedManager::STATE_CLEAN, wa.getEnvironmentStatus());

    // 2 APs on Ch 6: maxAps <= 2 -> Clean
    std::vector<AccessPointInfo> aps;
    AccessPointInfo ap1;
    ap1.channel = 6; ap1.rssi = -70; ap1.ssid = "AP1"; ap1.bssidStr = "00:11:22:33:01:01";
    ap1.bssid[0]=0x00; ap1.bssid[1]=0x11; ap1.bssid[2]=0x22; ap1.bssid[3]=0x33; ap1.bssid[4]=0x01; ap1.bssid[5]=0x01;
    aps.push_back(ap1);

    AccessPointInfo ap2;
    ap2.channel = 6; ap2.rssi = -75; ap2.ssid = "AP2"; ap2.bssidStr = "00:11:22:33:02:02";
    ap2.bssid[0]=0x00; ap2.bssid[1]=0x11; ap2.bssid[2]=0x22; ap2.bssid[3]=0x33; ap2.bssid[4]=0x02; ap2.bssid[5]=0x02;
    aps.push_back(ap2);

    wa.setScanResults(aps);
    TEST_ASSERT_EQUAL(LedManager::STATE_CLEAN, wa.getEnvironmentStatus());

    // 4 APs on Ch 6: maxAps between 3 and 5 -> Congested Yellow
    for (int i = 3; i <= 4; i++) {
        AccessPointInfo ap;
        ap.channel = 6; ap.rssi = -80; ap.ssid = "AP" + std::to_string(i);
        ap.bssidStr = "00:11:22:33:0" + std::to_string(i) + ":00";
        ap.bssid[0]=0x00; ap.bssid[1]=0x11; ap.bssid[2]=0x22; ap.bssid[3]=0x33; ap.bssid[4]=(uint8_t)i; ap.bssid[5]=0x00;
        aps.push_back(ap);
    }
    wa.setScanResults(aps);
    TEST_ASSERT_EQUAL(LedManager::STATE_CONGESTED_YELLOW, wa.getEnvironmentStatus());

    // 7 APs on Ch 6: maxAps > 5 -> Congested Red
    for (int i = 5; i <= 7; i++) {
        AccessPointInfo ap;
        ap.channel = 6; ap.rssi = -82; ap.ssid = "AP" + std::to_string(i);
        ap.bssidStr = "00:11:22:33:0" + std::to_string(i) + ":00";
        ap.bssid[0]=0x00; ap.bssid[1]=0x11; ap.bssid[2]=0x22; ap.bssid[3]=0x33; ap.bssid[4]=(uint8_t)i; ap.bssid[5]=0x00;
        aps.push_back(ap);
    }
    wa.setScanResults(aps);
    TEST_ASSERT_EQUAL(LedManager::STATE_CONGESTED_RED, wa.getEnvironmentStatus());
}

void test_wifi_analyzer_tracker_led_status(void) {
    WifiAnalyzer wa;
    wa.startTracking("00:11:22:33:44:55", "TestAP", 6);

    // Strong signal (-55 dBm >= -65) -> Clean Green
    wa.recordTargetSample(-55);
    TEST_ASSERT_EQUAL(LedManager::STATE_CLEAN, wa.getTrackerLedStatus());

    // Moderate signal (-72 dBm between -80 and -65) -> Yellow
    wa.recordTargetSample(-72);
    TEST_ASSERT_EQUAL(LedManager::STATE_CONGESTED_YELLOW, wa.getTrackerLedStatus());

    // Distant/weak signal (-88 dBm < -80) -> Red
    wa.recordTargetSample(-88);
    TEST_ASSERT_EQUAL(LedManager::STATE_CONGESTED_RED, wa.getTrackerLedStatus());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_led_manager_initialization);
    RUN_TEST(test_led_manager_scanning_blink);
    RUN_TEST(test_led_manager_clean_solid_green);
    RUN_TEST(test_led_manager_congested_yellow_and_red);
    RUN_TEST(test_led_manager_pulses);
    RUN_TEST(test_led_manager_alert_red);
    RUN_TEST(test_led_manager_disabled_mode);
    RUN_TEST(test_led_manager_brightness_clamping);
    RUN_TEST(test_wifi_analyzer_environment_led_status);
    RUN_TEST(test_wifi_analyzer_tracker_led_status);
    return UNITY_END();
}
