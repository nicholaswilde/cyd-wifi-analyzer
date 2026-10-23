#ifndef SURVEY_MANAGER_H
#define SURVEY_MANAGER_H

#include <stdint.h>
#include <vector>
#include <string>
#include <ctime>
#include "wifi_analyzer.h"

#ifdef NATIVE_TEST
#include "Arduino.h"
#else
#include <Arduino.h>
#endif

class SurveyManager {
public:
    static String sanitizeSsidForCsv(const String& ssid, bool isHidden);
    static String getTimestampString(const struct tm* timeinfo = nullptr, unsigned long fallbackMillis = 0);
    static String formatCsvRow(const char* timestamp, const AccessPointInfo& ap);
    static std::string generateFilename(const struct tm* timeinfo = nullptr, unsigned long fallbackMillis = 0);

    static bool exportScanSnapshot(const std::vector<AccessPointInfo>& aps, const char* filepath = nullptr);
    static bool appendScanToSurvey(const std::vector<AccessPointInfo>& aps, const char* filepath = "/scans/wifi_survey.csv");
    static bool clearSurveyLog(const char* filepath = "/scans/wifi_survey.csv");
};

#endif // SURVEY_MANAGER_H
