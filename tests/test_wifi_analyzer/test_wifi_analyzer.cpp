#include <unity.h>
#include <cstring>
#include "../mocks/mocks.cpp"
#include "../../src/settings_manager.cpp"
#include "../../src/catppuccin.cpp"
#include "../../src/wifi_analyzer.cpp"
#include "wifi_analyzer.h"
#include "catppuccin.h"

void setUp(void) {}
void tearDown(void) {}

void test_match_bssid_prefix(void) {
    uint8_t mac1[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x01};
    uint8_t mac2[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x02};
    uint8_t mac3[6] = {0x00, 0x11, 0x22, 0x33, 0x55, 0x01};

    // First 5 bytes match -> true
    TEST_ASSERT_TRUE(matchBssidPrefix(mac1, mac2));
    // 5th byte differs -> false
    TEST_ASSERT_FALSE(matchBssidPrefix(mac1, mac3));
    // Null pointers
    TEST_ASSERT_FALSE(matchBssidPrefix(nullptr, mac1));
    TEST_ASSERT_FALSE(matchBssidPrefix(mac1, nullptr));
}

void test_rssi_to_quality(void) {
    // Floor is -100, ceiling is -50
    TEST_ASSERT_EQUAL_UINT8(0, rssiToQuality(-105));
    TEST_ASSERT_EQUAL_UINT8(0, rssiToQuality(-100));
    TEST_ASSERT_EQUAL_UINT8(100, rssiToQuality(-50));
    TEST_ASSERT_EQUAL_UINT8(100, rssiToQuality(-30));
    TEST_ASSERT_EQUAL_UINT8(50, rssiToQuality(-75));
    TEST_ASSERT_EQUAL_UINT8(80, rssiToQuality(-60));
}

void test_auth_mode_strings(void) {
    TEST_ASSERT_EQUAL_STRING("Open", authModeToString(WIFI_AUTH_OPEN));
    TEST_ASSERT_EQUAL_STRING("WEP", authModeToString(WIFI_AUTH_WEP));
    TEST_ASSERT_EQUAL_STRING("WPA", authModeToString(WIFI_AUTH_WPA_PSK));
    TEST_ASSERT_EQUAL_STRING("WPA2", authModeToString(WIFI_AUTH_WPA2_PSK));
    TEST_ASSERT_EQUAL_STRING("WPA/WPA2", authModeToString(WIFI_AUTH_WPA_WPA2_PSK));
    TEST_ASSERT_EQUAL_STRING("Enterprise", authModeToString(WIFI_AUTH_WPA2_ENTERPRISE));
    TEST_ASSERT_EQUAL_STRING("WPA3", authModeToString(WIFI_AUTH_WPA3_PSK));
    TEST_ASSERT_EQUAL_STRING("Unknown", authModeToString(99));
}

void test_channel_analysis_empty(void) {
    std::vector<AccessPointInfo> aps;
    ChannelInfo channels[CHANNEL_COUNT];
    std::vector<uint8_t> bestChannels;

    analyzeChannels(aps, channels, bestChannels);

    for (int i = 0; i < CHANNEL_COUNT; i++) {
        TEST_ASSERT_EQUAL_UINT8(i + 1, channels[i].channel);
        TEST_ASSERT_EQUAL_UINT16(0, channels[i].apCount);
        TEST_ASSERT_EQUAL_INT32(RSSI_FLOOR, channels[i].peakRssi);
        TEST_ASSERT_EQUAL_INT64(0, channels[i].noiseScore);
    }
    // When all channels are empty (0 noise), all channels 1-11 should be tied for best
    TEST_ASSERT_EQUAL_UINT32(11, bestChannels.size());
}

void test_channel_analysis_single_network(void) {
    std::vector<AccessPointInfo> aps;
    AccessPointInfo ap1;
    ap1.ssid = "HomeNetwork";
    uint8_t bssid[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01};
    memcpy(ap1.bssid, bssid, 6);
    ap1.rssi = -60; // -60 - (-100) = 40. noise = 40 * 40 = 1600
    ap1.channel = 6;
    ap1.authMode = WIFI_AUTH_WPA2_PSK;
    ap1.isHidden = false;
    aps.push_back(ap1);

    ChannelInfo channels[CHANNEL_COUNT];
    std::vector<uint8_t> bestChannels;

    analyzeChannels(aps, channels, bestChannels);

    // Channel 6 should have AP count 1, peak RSSI -60, and noiseScore 1600
    TEST_ASSERT_EQUAL_UINT16(1, channels[5].apCount);
    TEST_ASSERT_EQUAL_INT32(-60, channels[5].peakRssi);
    TEST_ASSERT_EQUAL_INT64(1600, channels[5].noiseScore);

    // Neighboring channels within +-4 should receive spillover noise
    // Ch 5, 7: 1600
    // Ch 4, 8: 1600
    // Ch 3, 9: 1600
    // Ch 2, 10: 1600
    TEST_ASSERT_EQUAL_INT64(1600, channels[4].noiseScore); // Ch 5
    TEST_ASSERT_EQUAL_INT64(1600, channels[6].noiseScore); // Ch 7
    TEST_ASSERT_EQUAL_INT64(1600, channels[1].noiseScore); // Ch 2
    TEST_ASSERT_EQUAL_INT64(1600, channels[9].noiseScore); // Ch 10

    // Channel 1 is out of range of Channel 6 (diff is 5), so noiseScore should be 0!
    TEST_ASSERT_EQUAL_INT64(0, channels[0].noiseScore);

    // Channel 1 should therefore be among the best channels
    bool foundCh1 = false;
    for (uint8_t ch : bestChannels) {
        if (ch == 1) foundCh1 = true;
    }
    TEST_ASSERT_TRUE(foundCh1);
}

void test_duplicate_bssid_virtual_ssid_handling(void) {
    std::vector<AccessPointInfo> aps;
    // Same physical AP emitting 2 SSIDs (e.g. Main and Guest) on channel 1
    AccessPointInfo ap1, ap2;
    uint8_t macBase[6] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x01};
    uint8_t macVirtual[6] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x02};

    ap1.ssid = "MyHome";
    memcpy(ap1.bssid, macBase, 6);
    ap1.rssi = -50;
    ap1.channel = 1;
    aps.push_back(ap1);

    ap2.ssid = "MyHome_Guest";
    memcpy(ap2.bssid, macVirtual, 6);
    ap2.rssi = -52;
    ap2.channel = 1;
    aps.push_back(ap2);

    ChannelInfo channels[CHANNEL_COUNT];
    std::vector<uint8_t> bestChannels;
    analyzeChannels(aps, channels, bestChannels);

    // AP count should only be 1 because it's the same physical device (BSSID prefix match)
    TEST_ASSERT_EQUAL_UINT16(1, channels[0].apCount);
    // Peak RSSI should be -50
    TEST_ASSERT_EQUAL_INT32(-50, channels[0].peakRssi);
}

void test_catppuccin_channel_colors(void) {
    // Ensure all 14 channels return distinct non-zero colors
    for (uint8_t c = 1; c <= 14; c++) {
        uint16_t color = getChannelColor(c, CATPPUCCIN_MOCHA);
        TEST_ASSERT_NOT_EQUAL(0, color);
    }
}

void test_catppuccin_latte_channel_colors(void) {
    // Ensure all 14 channels return distinct non-zero colors in Latte
    for (uint8_t c = 1; c <= 14; c++) {
        uint16_t color = getChannelColor(c, CATPPUCCIN_LATTE);
        TEST_ASSERT_NOT_EQUAL(0, color);
        lv_color_t lvCol = getChannelLvColor(c, CATPPUCCIN_LATTE);
        TEST_ASSERT_EQUAL_HEX16(color, lvCol.full);
    }
}

void test_catppuccin_latte_contrast_tokens(void) {
    const auto& p = getCatppuccinFlavor(CATPPUCCIN_LATTE);
    // Base and Mantle are light tones
    TEST_ASSERT_NOT_EQUAL(0, p.base);
    TEST_ASSERT_NOT_EQUAL(0, p.mantle);
    // Text and subtext0 are contrasting dark tones
    TEST_ASSERT_NOT_EQUAL(p.base, p.text);
    TEST_ASSERT_NOT_EQUAL(p.mantle, p.text);
    TEST_ASSERT_NOT_EQUAL(p.mantle, p.subtext0);
    // Surface1 provides distinct contrast from mantle/base for borders and tracks
    TEST_ASSERT_NOT_EQUAL(p.mantle, p.surface1);
    TEST_ASSERT_NOT_EQUAL(p.base, p.surface1);
}

void test_calculate_parabola_y(void) {
    float cx = 100.0f;
    float spread = 40.0f;
    float peak_h = 80.0f;
    float baseline = 150.0f;

    // 1. Apex at x = cx: baseline - peak_h = 70.0f
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 70.0f, calculateParabolaY(cx, cx, spread, peak_h, baseline));

    // 2. Edges at cx +- spread: baseline = 150.0f
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 150.0f, calculateParabolaY(cx + spread, cx, spread, peak_h, baseline));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 150.0f, calculateParabolaY(cx - spread, cx, spread, peak_h, baseline));

    // 3. Halfway (dx = 20): norm = 0.5, 1 - 0.25 = 0.75. baseline - 0.75 * 80 = 150 - 60 = 90.0f
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, calculateParabolaY(cx + 20.0f, cx, spread, peak_h, baseline));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 90.0f, calculateParabolaY(cx - 20.0f, cx, spread, peak_h, baseline));

    // 4. Outside spread should clamp to baseline
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 150.0f, calculateParabolaY(cx + 45.0f, cx, spread, peak_h, baseline));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 150.0f, calculateParabolaY(cx - 50.0f, cx, spread, peak_h, baseline));

    // 5. Zero or negative spread fallback
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 150.0f, calculateParabolaY(cx, cx, 0.0f, peak_h, baseline));
}

void test_channel_to_frequency(void) {
    TEST_ASSERT_EQUAL_UINT16(2412, channelToFrequency(1));
    TEST_ASSERT_EQUAL_UINT16(2437, channelToFrequency(6));
    TEST_ASSERT_EQUAL_UINT16(2462, channelToFrequency(11));
    TEST_ASSERT_EQUAL_UINT16(2472, channelToFrequency(13));
    TEST_ASSERT_EQUAL_UINT16(2484, channelToFrequency(14));
    TEST_ASSERT_EQUAL_UINT16(0, channelToFrequency(0));
    TEST_ASSERT_EQUAL_UINT16(0, channelToFrequency(15));
}

void test_rssi_history_tracking(void) {
    WifiAnalyzer wa;
    String bssid = "AA:BB:CC:DD:EE:FF";

    TEST_ASSERT_EQUAL_UINT32(0, wa.getRssiHistory(bssid).size());

    // Record 5 samples
    for (int8_t s = -80; s >= -84; s--) {
        wa.recordRssiSample(bssid, s);
    }
    const auto& hist = wa.getRssiHistory(bssid);
    TEST_ASSERT_EQUAL_UINT32(5, hist.size());
    TEST_ASSERT_EQUAL_INT8(-80, hist[0]);
    TEST_ASSERT_EQUAL_INT8(-84, hist[4]);

    // Push 35 more samples to exceed MAX_HISTORY_SAMPLES (30)
    for (int8_t s = 1; s <= 35; s++) {
        wa.recordRssiSample(bssid, s);
    }
    const auto& hist_capped = wa.getRssiHistory(bssid);
    TEST_ASSERT_EQUAL_UINT32(WifiAnalyzer::MAX_HISTORY_SAMPLES, hist_capped.size());
    // Most recent sample should be 35
    TEST_ASSERT_EQUAL_INT8(35, hist_capped.back());

    wa.clearHistory();
    TEST_ASSERT_EQUAL_UINT32(0, wa.getRssiHistory(bssid).size());
}

void test_tracker_start_stop(void) {
    WifiAnalyzer wa;
    TEST_ASSERT_FALSE(wa.isTracking());

    wa.startTracking("11:22:33:44:55:66", "TargetNet", 6);
    TEST_ASSERT_TRUE(wa.isTracking());
    TEST_ASSERT_EQUAL_STRING("11:22:33:44:55:66", wa.getTargetBssid().c_str());
    TEST_ASSERT_EQUAL_STRING("TargetNet", wa.getTargetSsid().c_str());
    TEST_ASSERT_EQUAL_UINT8(6, wa.getTargetChannel());
    TEST_ASSERT_EQUAL_INT32(RSSI_FLOOR, wa.getTargetRssi());
    TEST_ASSERT_EQUAL_UINT32(0, wa.getTargetRssiHistory().size());

    wa.stopTracking();
    TEST_ASSERT_FALSE(wa.isTracking());
}

void test_tracker_sampling_and_history(void) {
    WifiAnalyzer wa;
    wa.startTracking("AA:BB:CC:DD:EE:FF", "MyAP", 11);

    wa.recordTargetSample(-72);
    TEST_ASSERT_EQUAL_INT32(-72, wa.getTargetRssi());
    TEST_ASSERT_EQUAL_UINT32(1, wa.getTargetRssiHistory().size());

    // Fill beyond MAX_TRACKER_HISTORY (20)
    for (int i = 0; i < 25; i++) {
        wa.recordTargetSample(-60 + (i % 5));
    }
    TEST_ASSERT_EQUAL_UINT32(WifiAnalyzer::MAX_TRACKER_HISTORY, wa.getTargetRssiHistory().size());
}

void test_tracker_trend_calculation(void) {
    WifiAnalyzer wa;
    wa.startTracking("AA:BB:CC:DD:EE:FF", "MyAP", 1);

    // Fewer than 4 samples -> 0 (stable)
    wa.recordTargetSample(-80);
    wa.recordTargetSample(-80);
    TEST_ASSERT_EQUAL(0, wa.getTargetTrend());

    // Add 2 more: stable (-80, -80, -80, -80)
    wa.recordTargetSample(-80);
    wa.recordTargetSample(-80);
    TEST_ASSERT_EQUAL(0, wa.getTargetTrend());

    // Rising / Warmer: recent average is higher than older average
    wa.recordTargetSample(-65);
    wa.recordTargetSample(-60);
    TEST_ASSERT_EQUAL(1, wa.getTargetTrend());

    // Falling / Colder: recent average drops
    wa.recordTargetSample(-90);
    wa.recordTargetSample(-95);
    TEST_ASSERT_EQUAL(-1, wa.getTargetTrend());
}

void test_tracker_proximity_strings(void) {
    WifiAnalyzer wa;
    wa.startTracking("AA:BB:CC:DD:EE:FF", "MyAP", 1);

    wa.recordTargetSample(-45);
    TEST_ASSERT_EQUAL_STRING("Very Close / Immediate", wa.getTargetProximityStr());

    wa.recordTargetSample(-60);
    TEST_ASSERT_EQUAL_STRING("Close Range (Strong)", wa.getTargetProximityStr());

    wa.recordTargetSample(-72);
    TEST_ASSERT_EQUAL_STRING("Moderate Distance", wa.getTargetProximityStr());

    wa.recordTargetSample(-82);
    TEST_ASSERT_EQUAL_STRING("Weak / Distant", wa.getTargetProximityStr());

    wa.recordTargetSample(-95);
    TEST_ASSERT_EQUAL_STRING("Faint / Out of Range", wa.getTargetProximityStr());
}

void test_ap_filtering(void) {
    std::vector<AccessPointInfo> aps;
    // AP 0: Protected, visible
    AccessPointInfo ap0;
    ap0.ssid = "HomeNet";
    ap0.rssi = -60;
    ap0.channel = 1;
    ap0.authMode = WIFI_AUTH_WPA2_PSK;
    ap0.isHidden = false;
    aps.push_back(ap0);

    // AP 1: Open, visible
    AccessPointInfo ap1;
    ap1.ssid = "CoffeeGuest";
    ap1.rssi = -70;
    ap1.channel = 6;
    ap1.authMode = WIFI_AUTH_OPEN;
    ap1.isHidden = false;
    aps.push_back(ap1);

    // AP 2: Protected, hidden (flag true)
    AccessPointInfo ap2;
    ap2.ssid = "";
    ap2.rssi = -40;
    ap2.channel = 6;
    ap2.authMode = WIFI_AUTH_WPA2_PSK;
    ap2.isHidden = true;
    aps.push_back(ap2);

    // AP 3: Open, hidden (SSID prefix "[Hidden")
    AccessPointInfo ap3;
    ap3.ssid = "[Hidden Network]";
    ap3.rssi = -85;
    ap3.channel = 11;
    ap3.authMode = WIFI_AUTH_OPEN;
    ap3.isHidden = false;
    aps.push_back(ap3);

    // AP 4: Open, visible
    AccessPointInfo ap4;
    ap4.ssid = "AirportFree";
    ap4.rssi = -50;
    ap4.channel = 11;
    ap4.authMode = WIFI_AUTH_OPEN;
    ap4.isHidden = false;
    aps.push_back(ap4);

    // AP_FILTER_ALL: all 5
    auto allIdx = filterAndSortAccessPoints(aps, AP_SORT_RSSI, AP_FILTER_ALL);
    TEST_ASSERT_EQUAL_UINT32(5, allIdx.size());

    // AP_FILTER_OPEN_ONLY: AP 1, AP 3, AP 4
    auto openIdx = filterAndSortAccessPoints(aps, AP_SORT_RSSI, AP_FILTER_OPEN_ONLY);
    TEST_ASSERT_EQUAL_UINT32(3, openIdx.size());
    for (size_t idx : openIdx) {
        TEST_ASSERT_EQUAL(WIFI_AUTH_OPEN, aps[idx].authMode);
    }

    // AP_FILTER_HIDE_HIDDEN: AP 0, AP 1, AP 4
    auto noHiddenIdx = filterAndSortAccessPoints(aps, AP_SORT_RSSI, AP_FILTER_HIDE_HIDDEN);
    TEST_ASSERT_EQUAL_UINT32(3, noHiddenIdx.size());
    for (size_t idx : noHiddenIdx) {
        TEST_ASSERT_FALSE(aps[idx].isHidden);
        TEST_ASSERT_FALSE(aps[idx].ssid.startsWith("[Hidden"));
        TEST_ASSERT_TRUE(aps[idx].ssid.length() > 0);
    }

    // AP_FILTER_OPEN_AND_VISIBLE: AP 1, AP 4
    auto openVisibleIdx = filterAndSortAccessPoints(aps, AP_SORT_RSSI, AP_FILTER_OPEN_AND_VISIBLE);
    TEST_ASSERT_EQUAL_UINT32(2, openVisibleIdx.size());
    for (size_t idx : openVisibleIdx) {
        TEST_ASSERT_EQUAL(WIFI_AUTH_OPEN, aps[idx].authMode);
        TEST_ASSERT_FALSE(aps[idx].isHidden);
        TEST_ASSERT_FALSE(aps[idx].ssid.startsWith("[Hidden"));
    }
}

void test_ap_sorting(void) {
    std::vector<AccessPointInfo> aps;

    AccessPointInfo ap0;
    ap0.ssid = "zebra";
    ap0.rssi = -60;
    ap0.channel = 6;
    ap0.authMode = WIFI_AUTH_WPA2_PSK;
    ap0.isHidden = false;
    aps.push_back(ap0);

    AccessPointInfo ap1;
    ap1.ssid = "Apple";
    ap1.rssi = -80;
    ap1.channel = 1;
    ap1.authMode = WIFI_AUTH_WPA2_PSK;
    ap1.isHidden = false;
    aps.push_back(ap1);

    AccessPointInfo ap2;
    ap2.ssid = "banana";
    ap2.rssi = -45;
    ap2.channel = 6;
    ap2.authMode = WIFI_AUTH_WPA2_PSK;
    ap2.isHidden = false;
    aps.push_back(ap2);

    AccessPointInfo ap3;
    ap3.ssid = "[Hidden]";
    ap3.rssi = -30;
    ap3.channel = 11;
    ap3.authMode = WIFI_AUTH_WPA2_PSK;
    ap3.isHidden = true;
    aps.push_back(ap3);

    // 1. Sort by RSSI: -30 (ap3) -> -45 (ap2) -> -60 (ap0) -> -80 (ap1)
    auto rssiIdx = filterAndSortAccessPoints(aps, AP_SORT_RSSI, AP_FILTER_ALL);
    TEST_ASSERT_EQUAL_UINT32(4, rssiIdx.size());
    TEST_ASSERT_EQUAL(3, rssiIdx[0]);
    TEST_ASSERT_EQUAL(2, rssiIdx[1]);
    TEST_ASSERT_EQUAL(0, rssiIdx[2]);
    TEST_ASSERT_EQUAL(1, rssiIdx[3]);

    // 2. Sort by SSID (case-insensitive, visible first): "Apple" -> "banana" -> "zebra" -> "[Hidden]"
    auto ssidIdx = filterAndSortAccessPoints(aps, AP_SORT_SSID, AP_FILTER_ALL);
    TEST_ASSERT_EQUAL_UINT32(4, ssidIdx.size());
    TEST_ASSERT_EQUAL_STRING("Apple", aps[ssidIdx[0]].ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("banana", aps[ssidIdx[1]].ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("zebra", aps[ssidIdx[2]].ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("[Hidden]", aps[ssidIdx[3]].ssid.c_str());

    // 3. Sort by Channel: Ch 1 (ap1) -> Ch 6 (-45 ap2, then -60 ap0) -> Ch 11 (ap3)
    auto chanIdx = filterAndSortAccessPoints(aps, AP_SORT_CHANNEL, AP_FILTER_ALL);
    TEST_ASSERT_EQUAL_UINT32(4, chanIdx.size());
    TEST_ASSERT_EQUAL(1, chanIdx[0]); // Ch 1
    TEST_ASSERT_EQUAL(2, chanIdx[1]); // Ch 6, -45 dBm
    TEST_ASSERT_EQUAL(0, chanIdx[2]); // Ch 6, -60 dBm
    TEST_ASSERT_EQUAL(3, chanIdx[3]); // Ch 11
}

void test_ap_filter_and_sort_empty(void) {
    std::vector<AccessPointInfo> emptyAps;
    auto res1 = filterAndSortAccessPoints(emptyAps, AP_SORT_RSSI, AP_FILTER_ALL);
    TEST_ASSERT_TRUE(res1.empty());

    auto res2 = filterAndSortAccessPoints(emptyAps, AP_SORT_SSID, AP_FILTER_OPEN_ONLY);
    TEST_ASSERT_TRUE(res2.empty());

    auto res3 = filterAndSortAccessPoints(emptyAps, AP_SORT_CHANNEL, AP_FILTER_HIDE_HIDDEN);
    TEST_ASSERT_TRUE(res3.empty());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_match_bssid_prefix);
    RUN_TEST(test_rssi_to_quality);
    RUN_TEST(test_auth_mode_strings);
    RUN_TEST(test_channel_analysis_empty);
    RUN_TEST(test_channel_analysis_single_network);
    RUN_TEST(test_duplicate_bssid_virtual_ssid_handling);
    RUN_TEST(test_catppuccin_channel_colors);
    RUN_TEST(test_catppuccin_latte_channel_colors);
    RUN_TEST(test_catppuccin_latte_contrast_tokens);
    RUN_TEST(test_calculate_parabola_y);
    RUN_TEST(test_channel_to_frequency);
    RUN_TEST(test_rssi_history_tracking);
    RUN_TEST(test_tracker_start_stop);
    RUN_TEST(test_tracker_sampling_and_history);
    RUN_TEST(test_tracker_trend_calculation);
    RUN_TEST(test_tracker_proximity_strings);
    RUN_TEST(test_ap_filtering);
    RUN_TEST(test_ap_sorting);
    RUN_TEST(test_ap_filter_and_sort_empty);
    return UNITY_END();
}
