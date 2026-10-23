#ifndef WIFI_ANALYZER_H
#define WIFI_ANALYZER_H

#include <stdint.h>
#include <vector>
#include <map>
#include "config/config.h"
#include "led_manager.h"
#include "oui.h"

#ifdef NATIVE_TEST
#include "Arduino.h"
#define WIFI_AUTH_OPEN 0
#define WIFI_AUTH_WEP 1
#define WIFI_AUTH_WPA_PSK 2
#define WIFI_AUTH_WPA2_PSK 3
#define WIFI_AUTH_WPA_WPA2_PSK 4
#define WIFI_AUTH_WPA2_ENTERPRISE 5
#define WIFI_AUTH_WPA3_PSK 6
#else
#include <Arduino.h>
#include <WiFi.h>
#endif

struct AccessPointInfo {
    String ssid;
    uint8_t bssid[6] = {0};
    String bssidStr;
    int32_t rssi = -100;
    uint8_t channel = 1;
    uint8_t authMode = 0;
    bool isHidden = false;
    const char* vendor = nullptr;
};

struct ChannelInfo {
    uint8_t channel;
    uint16_t apCount;
    int32_t peakRssi;
    int64_t noiseScore;
    int16_t peakApIndex;
};

enum ApSortMode {
    AP_SORT_RSSI = 0,
    AP_SORT_SSID = 1,
    AP_SORT_CHANNEL = 2
};

enum ApFilterMode {
    AP_FILTER_ALL = 0,
    AP_FILTER_OPEN_ONLY = 1,
    AP_FILTER_HIDE_HIDDEN = 2,
    AP_FILTER_OPEN_AND_VISIBLE = 3
};

// Analysis utility functions
bool matchBssidPrefix(const uint8_t* a, const uint8_t* b);
uint8_t rssiToQuality(int32_t rssi);
const char* authModeToString(uint8_t authMode);
uint16_t channelToFrequency(uint8_t channel);
void analyzeChannels(const std::vector<AccessPointInfo>& aps, ChannelInfo channels[CHANNEL_COUNT], std::vector<uint8_t>& bestChannels);
float calculateParabolaY(float x, float cx, float spread, float peakHeight, float baseline);
bool isApFiltered(const AccessPointInfo& ap, uint8_t filterMode);
std::vector<size_t> filterAndSortAccessPoints(const std::vector<AccessPointInfo>& aps, uint8_t sortMode, uint8_t filterMode);
const char* getOuiVendor(const AccessPointInfo& ap);

class WifiAnalyzer {
public:
    static const size_t MAX_HISTORY_SAMPLES = 30;

    WifiAnalyzer();
    void begin();
    bool triggerScan();
    bool update();
    bool isScanning() const { return _isScanning; }
    
    const std::vector<AccessPointInfo>& getAccessPoints() const { return _accessPoints; }
    std::vector<size_t> getFilteredAndSortedIndices(uint8_t sortMode, uint8_t filterMode) const {
        return filterAndSortAccessPoints(_accessPoints, sortMode, filterMode);
    }
    const ChannelInfo* getChannels() const { return _channels; }
    const std::vector<uint8_t>& getBestChannels() const { return _bestChannels; }
    uint32_t getLastScanDurationMs() const { return _lastScanDurationMs; }
    uint32_t getScanCount() const { return _scanCount; }

    LedManager::State getEnvironmentStatus() const;
    LedManager::State getTrackerLedStatus() const;

    const std::vector<int8_t>& getRssiHistory(const String& bssidStr) const;
    void recordRssiSample(const String& bssidStr, int8_t rssi);
    void clearHistory();

    // Target AP Signal Tracking Mode
    static const size_t MAX_TRACKER_HISTORY = 20;

    void startTracking(const String& bssidStr, const String& ssid, uint8_t channel);
    void stopTracking();
    bool isTracking() const { return _isTracking; }

    const String& getTargetBssid() const { return _targetBssid; }
    const String& getTargetSsid() const { return _targetSsid; }
    uint8_t getTargetChannel() const { return _targetChannel; }
    int32_t getTargetRssi() const { return _targetRssi; }
    const std::vector<int8_t>& getTargetRssiHistory() const { return _targetRssiHistory; }
    int getTargetTrend() const;
    const char* getTargetProximityStr() const;
    void recordTargetSample(int8_t rssi);
    bool isChirpEnabled() const { return _chirpEnabled; }
    void setChirpEnabled(bool en) { _chirpEnabled = en; }

    // For testing and manual population
    void setScanResults(const std::vector<AccessPointInfo>& aps);

private:
    std::vector<AccessPointInfo> _accessPoints;
    ChannelInfo _channels[CHANNEL_COUNT];
    std::vector<uint8_t> _bestChannels;
    std::map<String, std::vector<int8_t>> _bssidHistory;

    bool _isTracking;
    String _targetBssid;
    String _targetSsid;
    uint8_t _targetChannel;
    int32_t _targetRssi;
    std::vector<int8_t> _targetRssiHistory;
    bool _chirpEnabled;
    
    bool _isScanning;
    uint32_t _scanStartTime;
    uint32_t _lastScanDurationMs;
    uint32_t _lastScanEndTime;
    uint32_t _scanCount;

    void processScanResults();
};

extern WifiAnalyzer wifiAnalyzer;

#endif // WIFI_ANALYZER_H
