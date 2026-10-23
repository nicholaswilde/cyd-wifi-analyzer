#include <unity.h>
#include <cstring>
#include <ctime>
#include "../mocks/mocks.cpp"
#include "../../src/sd_card_manager.cpp"
#include "../../src/survey_manager.cpp"
#include "../../src/wifi_analyzer.cpp"
#include "survey_manager.h"
#include "sd_card_manager.h"

void setUp(void) {
    mock_files.clear();
    mock_sd_card_present = true;
    mock_sd_card_mounted = false;
}

void tearDown(void) {
    mock_files.clear();
}

void test_sanitize_ssid_for_csv(void) {
    // Normal SSID
    TEST_ASSERT_EQUAL_STRING("HomeWiFi", SurveyManager::sanitizeSsidForCsv("HomeWiFi", false).c_str());

    // SSID with comma
    TEST_ASSERT_EQUAL_STRING("\"Guest,Cafe\"", SurveyManager::sanitizeSsidForCsv("Guest,Cafe", false).c_str());

    // SSID with double quote
    TEST_ASSERT_EQUAL_STRING("\"Alice\"\"s AP\"", SurveyManager::sanitizeSsidForCsv("Alice\"s AP", false).c_str());

    // SSID with comma and quote
    TEST_ASSERT_EQUAL_STRING("\"AP, \"\"Fast\"\"\"", SurveyManager::sanitizeSsidForCsv("AP, \"Fast\"", false).c_str());

    // Hidden network flag
    TEST_ASSERT_EQUAL_STRING("[Hidden]", SurveyManager::sanitizeSsidForCsv("Secret", true).c_str());

    // Empty SSID
    TEST_ASSERT_EQUAL_STRING("[Hidden]", SurveyManager::sanitizeSsidForCsv("", false).c_str());

    // Prefix "[Hidden Network]"
    TEST_ASSERT_EQUAL_STRING("[Hidden]", SurveyManager::sanitizeSsidForCsv("[Hidden Network]", false).c_str());
}

void test_timestamp_string(void) {
    // Fallback millis
    String fallback = SurveyManager::getTimestampString(nullptr, 42000);
    TEST_ASSERT_EQUAL_STRING("42000", fallback.c_str());

    // Struct tm
    struct tm t;
    std::memset(&t, 0, sizeof(t));
    t.tm_year = 126; // 2026
    t.tm_mon = 9;   // October (0-indexed)
    t.tm_mday = 22;
    t.tm_hour = 22;
    t.tm_min = 30;
    t.tm_sec = 15;

    String ts = SurveyManager::getTimestampString(&t, 0);
    TEST_ASSERT_EQUAL_STRING("2026-10-22 22:30:15", ts.c_str());
}

void test_format_csv_row(void) {
    AccessPointInfo ap;
    ap.ssid = "Starbucks, Free";
    ap.bssidStr = "aa:bb:cc:dd:ee:ff";
    ap.rssi = -68;
    ap.channel = 6;
    ap.authMode = WIFI_AUTH_WPA2_PSK;
    ap.isHidden = false;

    String row = SurveyManager::formatCsvRow("2026-10-22 22:30:00", ap);
    TEST_ASSERT_EQUAL_STRING("2026-10-22 22:30:00,\"Starbucks, Free\",aa:bb:cc:dd:ee:ff,-68,6,WPA2,0\n", row.c_str());

    // Hidden AP
    AccessPointInfo hiddenAp;
    hiddenAp.ssid = "";
    hiddenAp.bssidStr = "11:22:33:44:55:66";
    hiddenAp.rssi = -45;
    hiddenAp.channel = 11;
    hiddenAp.authMode = WIFI_AUTH_OPEN;
    hiddenAp.isHidden = true;

    String hiddenRow = SurveyManager::formatCsvRow("1000", hiddenAp);
    TEST_ASSERT_EQUAL_STRING("1000,[Hidden],11:22:33:44:55:66,-45,11,Open,1\n", hiddenRow.c_str());
}

void test_generate_filename(void) {
    std::string fallback = SurveyManager::generateFilename(nullptr, 12345);
    TEST_ASSERT_EQUAL_STRING("/scans/wifi_scan_12345.csv", fallback.c_str());

    struct tm t;
    std::memset(&t, 0, sizeof(t));
    t.tm_year = 126; // 2026
    t.tm_mon = 9;   // October
    t.tm_mday = 22;
    t.tm_hour = 14;
    t.tm_min = 5;
    t.tm_sec = 9;

    std::string named = SurveyManager::generateFilename(&t, 0);
    TEST_ASSERT_EQUAL_STRING("/scans/wifi_scan_20261022_140509.csv", named.c_str());
}

void test_export_scan_snapshot_success(void) {
    std::vector<AccessPointInfo> aps;
    AccessPointInfo ap;
    ap.ssid = "TestNet";
    ap.bssidStr = "00:11:22:33:44:55";
    ap.rssi = -55;
    ap.channel = 1;
    ap.authMode = WIFI_AUTH_WPA2_PSK;
    ap.isHidden = false;
    aps.push_back(ap);

    bool ok = SurveyManager::exportScanSnapshot(aps, "/scans/test_snapshot.csv");
    TEST_ASSERT_TRUE(ok);

    auto it = mock_files.find("/scans/test_snapshot.csv");
    TEST_ASSERT_TRUE(it != mock_files.end());

    // Verify header and content
    std::string content = it->second;
    TEST_ASSERT_TRUE(content.find("Timestamp,SSID,BSSID,RSSI,Channel,AuthMode,Hidden\n") == 0);
    TEST_ASSERT_TRUE(content.find("TestNet,00:11:22:33:44:55,-55,1,WPA2,0") != std::string::npos);
}

void test_export_scan_snapshot_no_sd(void) {
    mock_sd_card_present = false;
    std::vector<AccessPointInfo> aps;
    AccessPointInfo ap;
    ap.ssid = "TestNet";
    aps.push_back(ap);

    bool ok = SurveyManager::exportScanSnapshot(aps, "/scans/fail.csv");
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_TRUE(mock_files.find("/scans/fail.csv") == mock_files.end());
}

void test_append_scan_to_survey(void) {
    std::vector<AccessPointInfo> batch1;
    AccessPointInfo ap1;
    ap1.ssid = "Net1";
    ap1.bssidStr = "00:00:00:00:00:01";
    ap1.rssi = -60;
    ap1.channel = 1;
    ap1.authMode = WIFI_AUTH_OPEN;
    ap1.isHidden = false;
    batch1.push_back(ap1);

    bool ok1 = SurveyManager::appendScanToSurvey(batch1, "/scans/survey_test.csv");
    TEST_ASSERT_TRUE(ok1);

    std::vector<AccessPointInfo> batch2;
    AccessPointInfo ap2;
    ap2.ssid = "Net2";
    ap2.bssidStr = "00:00:00:00:00:02";
    ap2.rssi = -75;
    ap2.channel = 6;
    ap2.authMode = WIFI_AUTH_WPA3_PSK;
    ap2.isHidden = false;
    batch2.push_back(ap2);

    bool ok2 = SurveyManager::appendScanToSurvey(batch2, "/scans/survey_test.csv");
    TEST_ASSERT_TRUE(ok2);

    auto it = mock_files.find("/scans/survey_test.csv");
    TEST_ASSERT_TRUE(it != mock_files.end());

    const std::string& content = it->second;
    // Header should only appear once
    size_t firstHeader = content.find("Timestamp,SSID,BSSID,RSSI,Channel,AuthMode,Hidden\n");
    TEST_ASSERT_TRUE(firstHeader != std::string::npos);
    size_t secondHeader = content.find("Timestamp,SSID,BSSID,RSSI,Channel,AuthMode,Hidden\n", firstHeader + 10);
    TEST_ASSERT_TRUE(secondHeader == std::string::npos);

    // Both entries should be present
    TEST_ASSERT_TRUE(content.find("Net1,00:00:00:00:00:01,-60,1,Open,0") != std::string::npos);
    TEST_ASSERT_TRUE(content.find("Net2,00:00:00:00:00:02,-75,6,WPA3,0") != std::string::npos);
}

void test_clear_survey_log(void) {
    mock_files["/scans/test_del.csv"] = "some data";
    TEST_ASSERT_TRUE(SurveyManager::clearSurveyLog("/scans/test_del.csv"));
    TEST_ASSERT_TRUE(mock_files.find("/scans/test_del.csv") == mock_files.end());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sanitize_ssid_for_csv);
    RUN_TEST(test_timestamp_string);
    RUN_TEST(test_format_csv_row);
    RUN_TEST(test_generate_filename);
    RUN_TEST(test_export_scan_snapshot_success);
    RUN_TEST(test_export_scan_snapshot_no_sd);
    RUN_TEST(test_append_scan_to_survey);
    RUN_TEST(test_clear_survey_log);
    return UNITY_END();
}
