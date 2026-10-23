#include "survey_manager.h"
#include "sd_card_manager.h"
#include <cstdio>
#include <cstring>

#ifndef NATIVE_TEST
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#else
#include "SD.h"
#include "FS.h"
#include "SPI.h"
#endif

String SurveyManager::sanitizeSsidForCsv(const String& ssid, bool isHidden) {
    if (isHidden || ssid.length() == 0 || ssid.startsWith("[Hidden")) {
        return "[Hidden]";
    }
    bool needsQuotes = false;
    for (size_t i = 0; i < ssid.length(); i++) {
        if (ssid[i] == ',' || ssid[i] == '"' || ssid[i] == '\n' || ssid[i] == '\r') {
            needsQuotes = true;
            break;
        }
    }
    if (!needsQuotes) {
        return ssid;
    }
    String result = "\"";
    for (size_t i = 0; i < ssid.length(); i++) {
        if (ssid[i] == '"') {
            result += "\"\"";
        } else {
            result += ssid[i];
        }
    }
    result += "\"";
    return result;
}

String SurveyManager::getTimestampString(const struct tm* timeinfo, unsigned long fallbackMillis) {
    char buf[32];
    if (timeinfo && timeinfo->tm_year > 70) {
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);
    } else {
        snprintf(buf, sizeof(buf), "%lu", fallbackMillis);
    }
    return String(buf);
}

String SurveyManager::formatCsvRow(const char* timestamp, const AccessPointInfo& ap) {
    String cleanSsid = sanitizeSsidForCsv(ap.ssid, ap.isHidden);
    const char* authStr = authModeToString(ap.authMode);
    char rowBuf[256];
    snprintf(rowBuf, sizeof(rowBuf), "%s,%s,%s,%d,%u,%s,%s\n",
             timestamp,
             cleanSsid.c_str(),
             ap.bssidStr.c_str(),
             (int)ap.rssi,
             ap.channel,
             authStr,
             ap.isHidden ? "1" : "0");
    return String(rowBuf);
}

std::string SurveyManager::generateFilename(const struct tm* timeinfo, unsigned long fallbackMillis) {
    char filename[64];
    if (timeinfo && timeinfo->tm_year > 70) {
        strftime(filename, sizeof(filename), "/scans/wifi_scan_%Y%m%d_%H%M%S.csv", timeinfo);
    } else {
        snprintf(filename, sizeof(filename), "/scans/wifi_scan_%lu.csv", fallbackMillis);
    }
    return std::string(filename);
}

bool SurveyManager::exportScanSnapshot(const std::vector<AccessPointInfo>& aps, const char* filepath) {
    if (aps.empty()) {
        return false;
    }
    if (!SdCardManager::begin()) {
        Serial.println("[Survey] Error: SD card not mounted.");
        return false;
    }

    std::string path;
    if (filepath) {
        path = filepath;
    } else {
        path = generateFilename(nullptr, millis());
    }

    if (!SD.exists("/scans")) {
        SD.mkdir("/scans");
    }

    File file = SD.open(path.c_str(), "w");
    if (!file) {
        Serial.printf("[Survey] Error: Failed to open %s for writing.\n", path.c_str());
        return false;
    }

    file.print("Timestamp,SSID,BSSID,RSSI,Channel,AuthMode,Hidden\n");
    String ts = getTimestampString(nullptr, millis());
    for (const auto& ap : aps) {
        file.print(formatCsvRow(ts.c_str(), ap));
    }
    file.close();
    Serial.printf("[Survey] Exported %u APs to %s\n", (unsigned)aps.size(), path.c_str());
    return true;
}

bool SurveyManager::appendScanToSurvey(const std::vector<AccessPointInfo>& aps, const char* filepath) {
    if (aps.empty()) {
        return false;
    }
    if (!SdCardManager::begin()) {
        return false;
    }

    if (!SD.exists("/scans")) {
        SD.mkdir("/scans");
    }

    const char* targetPath = filepath ? filepath : "/scans/wifi_survey.csv";
    bool fileExists = SD.exists(targetPath);

    File file = SD.open(targetPath, "a");
    if (!file) {
        Serial.printf("[Survey] Error: Failed to open %s for appending.\n", targetPath);
        return false;
    }

    if (!fileExists) {
        file.print("Timestamp,SSID,BSSID,RSSI,Channel,AuthMode,Hidden\n");
    }

    String ts = getTimestampString(nullptr, millis());
    for (const auto& ap : aps) {
        file.print(formatCsvRow(ts.c_str(), ap));
    }
    file.close();
    return true;
}

bool SurveyManager::clearSurveyLog(const char* filepath) {
    const char* targetPath = filepath ? filepath : "/scans/wifi_survey.csv";
    if (!SdCardManager::begin()) {
        return false;
    }
    if (SD.exists(targetPath)) {
        return SD.remove(targetPath);
    }
    return true;
}
