#include <unity.h>
#include <cstring>
#include "../mocks/mocks.cpp"
#include "../../src/oui.cpp"
#include "../../src/wifi_analyzer.cpp"
#include "oui.h"
#include "wifi_analyzer.h"

void setUp(void) {}
void tearDown(void) {}

void test_oui_table_integrity_and_sorting(void) {
    size_t count = getOuiTableSize();
    TEST_ASSERT_GREATER_THAN(50, count);

    const OuiEntry* table = getOuiTable();
    TEST_ASSERT_NOT_NULL(table);

    // Verify table is strictly sorted ascending and all entries can be retrieved
    for (size_t i = 0; i < count; i++) {
        if (i > 0) {
            TEST_ASSERT_GREATER_THAN(table[i - 1].prefix, table[i].prefix);
        }
        TEST_ASSERT_NOT_NULL(table[i].vendor);
        TEST_ASSERT_GREATER_THAN(0, strlen(table[i].vendor));

        // Binary search lookup for each prefix in table must succeed
        const char* vendor = getOuiVendor(table[i].prefix);
        TEST_ASSERT_EQUAL_STRING(table[i].vendor, vendor);
    }
}

void test_common_networking_vendors(void) {
    // 1. Apple (00:17:F2, 3C:D9:2B)
    uint8_t mac_apple[6] = {0x00, 0x17, 0xF2, 0xAA, 0xBB, 0xCC};
    TEST_ASSERT_EQUAL_STRING("Apple", getOuiVendor(mac_apple));
    TEST_ASSERT_EQUAL_STRING("Apple", getOuiVendor("00:17:F2:AA:BB:CC"));
    TEST_ASSERT_EQUAL_STRING("Apple", getOuiVendor("3C:D9:2B:11:22:33"));

    // 2. Cisco/Meraki (00:18:0A, 00:01:42, 70:69:79)
    uint8_t mac_meraki[6] = {0x00, 0x18, 0x0A, 0x12, 0x34, 0x56};
    TEST_ASSERT_EQUAL_STRING("Cisco/Meraki", getOuiVendor(mac_meraki));
    TEST_ASSERT_EQUAL_STRING("Cisco/Meraki", getOuiVendor("00:01:42:00:11:22"));
    TEST_ASSERT_EQUAL_STRING("Cisco/Meraki", getOuiVendor("70:69:79:DE:AD:BE"));

    // 3. Ubiquiti (00:15:6D, 24:A4:3C, B4:FB:E4)
    uint8_t mac_ubnt[6] = {0x00, 0x15, 0x6D, 0xAA, 0xBB, 0xCC};
    TEST_ASSERT_EQUAL_STRING("Ubiquiti", getOuiVendor(mac_ubnt));
    TEST_ASSERT_EQUAL_STRING("Ubiquiti", getOuiVendor("24:A4:3C:44:55:66"));
    TEST_ASSERT_EQUAL_STRING("Ubiquiti", getOuiVendor("B4:FB:E4:77:88:99"));

    // 4. Espressif (18:FE:34, 24:0A:C4, 30:AE:A4)
    uint8_t mac_esp[6] = {0x18, 0xFE, 0x34, 0x11, 0x22, 0x33};
    TEST_ASSERT_EQUAL_STRING("Espressif", getOuiVendor(mac_esp));
    TEST_ASSERT_EQUAL_STRING("Espressif", getOuiVendor("24:0A:C4:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("Espressif", getOuiVendor("30:AE:A4:AA:BB:CC"));

    // 5. TP-Link (00:0A:EB, 50:C7:BF, C0:06:C3)
    uint8_t mac_tplink[6] = {0x00, 0x0A, 0xEB, 0x10, 0x20, 0x30};
    TEST_ASSERT_EQUAL_STRING("TP-Link", getOuiVendor(mac_tplink));
    TEST_ASSERT_EQUAL_STRING("TP-Link", getOuiVendor("50:C7:BF:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("TP-Link", getOuiVendor("C0:06:C3:AA:BB:CC"));

    // 6. Netgear (00:09:5B, 20:E5:2A, C0:3F:0E)
    uint8_t mac_netgear[6] = {0x00, 0x09, 0x5B, 0x01, 0x02, 0x03};
    TEST_ASSERT_EQUAL_STRING("Netgear", getOuiVendor(mac_netgear));
    TEST_ASSERT_EQUAL_STRING("Netgear", getOuiVendor("20:E5:2A:44:55:66"));
    TEST_ASSERT_EQUAL_STRING("Netgear", getOuiVendor("C0:3F:0E:77:88:99"));

    // 7. Intel (00:02:B3, 80:86:F2, A4:4C:C8)
    uint8_t mac_intel[6] = {0x00, 0x02, 0xB3, 0x11, 0x22, 0x33};
    TEST_ASSERT_EQUAL_STRING("Intel", getOuiVendor(mac_intel));
    TEST_ASSERT_EQUAL_STRING("Intel", getOuiVendor("80:86:F2:12:34:56"));
    TEST_ASSERT_EQUAL_STRING("Intel", getOuiVendor("A4:4C:C8:FE:ED:01"));

    // 8. Aruba/HP (00:0B:86, 00:01:E6, 24:DE:C6)
    uint8_t mac_aruba[6] = {0x00, 0x0B, 0x86, 0xAA, 0xBB, 0xCC};
    TEST_ASSERT_EQUAL_STRING("Aruba/HP", getOuiVendor(mac_aruba));
    TEST_ASSERT_EQUAL_STRING("Aruba/HP", getOuiVendor("00:01:E6:12:34:56"));
    TEST_ASSERT_EQUAL_STRING("Aruba/HP", getOuiVendor("24:DE:C6:55:66:77"));

    // 9. Google (00:1A:11, 3C:5A:37, F4:F5:DB)
    uint8_t mac_google[6] = {0x00, 0x1A, 0x11, 0x01, 0x02, 0x03};
    TEST_ASSERT_EQUAL_STRING("Google", getOuiVendor(mac_google));
    TEST_ASSERT_EQUAL_STRING("Google", getOuiVendor("3C:5A:37:10:20:30"));
    TEST_ASSERT_EQUAL_STRING("Google", getOuiVendor("F4:F5:DB:99:88:77"));

    // 10. Amazon (00:FC:8B, 40:B4:CD, FC:A6:67)
    uint8_t mac_amazon[6] = {0x00, 0xFC, 0x8B, 0xAA, 0xBB, 0xCC};
    TEST_ASSERT_EQUAL_STRING("Amazon", getOuiVendor(mac_amazon));
    TEST_ASSERT_EQUAL_STRING("Amazon", getOuiVendor("40:B4:CD:11:22:33"));
    TEST_ASSERT_EQUAL_STRING("Amazon", getOuiVendor("FC:A6:67:EE:DD:CC"));

    // Additional prominent Wi-Fi AP vendors
    TEST_ASSERT_EQUAL_STRING("Asus", getOuiVendor("00:0C:6E:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("Linksys", getOuiVendor("00:04:5A:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("D-Link", getOuiVendor("00:05:5D:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("Huawei", getOuiVendor("00:18:82:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("AVM", getOuiVendor("00:04:0E:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("Samsung", getOuiVendor("00:00:F0:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("Raspberry Pi", getOuiVendor("B8:27:EB:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("MikroTik", getOuiVendor("00:0C:42:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("Ruckus", getOuiVendor("00:1D:2D:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("eero", getOuiVendor("40:9B:CD:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("Belkin", getOuiVendor("00:11:50:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("Synology", getOuiVendor("00:11:32:01:02:03"));
    TEST_ASSERT_EQUAL_STRING("Xiaomi", getOuiVendor("00:9E:C8:01:02:03"));
}

void test_unknown_vendor_fallback(void) {
    // Unmapped MAC addresses must return "Unknown Vendor"
    uint8_t unmapped[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01};
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor(unmapped));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor("DE:AD:BE:EF:00:01"));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor("11:22:33:44:55:66"));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor(0xDEADBE));

    // Null and empty pointers
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor((const uint8_t*)nullptr));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor((const char*)nullptr));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor(""));

    // Malformed strings
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor("not-a-mac"));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor("12:34"));
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor("ZZ:YY:XX:00:11:22"));
}

void test_mac_string_formats(void) {
    // Colon-separated uppercase
    TEST_ASSERT_EQUAL_STRING("Ubiquiti", getOuiVendor("00:15:6D:AA:BB:CC"));
    // Colon-separated lowercase
    TEST_ASSERT_EQUAL_STRING("Ubiquiti", getOuiVendor("00:15:6d:aa:bb:cc"));
    // Hyphen-separated
    TEST_ASSERT_EQUAL_STRING("Ubiquiti", getOuiVendor("00-15-6D-AA-BB-CC"));
    // Raw 6-hex without separators
    TEST_ASSERT_EQUAL_STRING("Ubiquiti", getOuiVendor("00156DAABBCC"));
    // Leading and trailing spaces
    TEST_ASSERT_EQUAL_STRING("Ubiquiti", getOuiVendor("   00:15:6D:AA:BB:CC   "));
}

void test_access_point_info_resolution(void) {
    // Case 1: ap with bssid bytes
    AccessPointInfo ap1;
    ap1.bssid[0] = 0x00; ap1.bssid[1] = 0x15; ap1.bssid[2] = 0x6D;
    ap1.bssid[3] = 0x12; ap1.bssid[4] = 0x34; ap1.bssid[5] = 0x56;
    ap1.bssidStr = "00:15:6D:12:34:56";
    TEST_ASSERT_EQUAL_STRING("Ubiquiti", getOuiVendor(ap1));

    // Case 2: ap with zero bytes but valid bssidStr
    AccessPointInfo ap2;
    memset(ap2.bssid, 0, 6);
    ap2.bssidStr = "00:17:F2:11:22:33"; // Apple
    TEST_ASSERT_EQUAL_STRING("Apple", getOuiVendor(ap2));

    // Case 3: ap with unmapped bssid
    AccessPointInfo ap3;
    ap3.bssidStr = "DE:AD:BE:01:02:03";
    TEST_ASSERT_EQUAL_STRING("Unknown Vendor", getOuiVendor(ap3));

    // Case 4: WifiAnalyzer setScanResults auto-populates vendor field
    WifiAnalyzer wa;
    std::vector<AccessPointInfo> aps;
    aps.push_back(ap1);
    aps.push_back(ap2);
    aps.push_back(ap3);
    wa.setScanResults(aps);

    const auto& results = wa.getAccessPoints();
    TEST_ASSERT_EQUAL_UINT32(3, results.size());
    // APs are sorted by RSSI in setScanResults; check that each AP has non-null vendor
    for (const auto& ap : results) {
        TEST_ASSERT_NOT_NULL(ap.vendor);
        if (ap.bssidStr == "00:15:6D:12:34:56") {
            TEST_ASSERT_EQUAL_STRING("Ubiquiti", ap.vendor);
        } else if (ap.bssidStr == "00:17:F2:11:22:33") {
            TEST_ASSERT_EQUAL_STRING("Apple", ap.vendor);
        } else if (ap.bssidStr == "DE:AD:BE:01:02:03") {
            TEST_ASSERT_EQUAL_STRING("Unknown Vendor", ap.vendor);
        }
    }
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_oui_table_integrity_and_sorting);
    RUN_TEST(test_common_networking_vendors);
    RUN_TEST(test_unknown_vendor_fallback);
    RUN_TEST(test_mac_string_formats);
    RUN_TEST(test_access_point_info_resolution);
    return UNITY_END();
}
