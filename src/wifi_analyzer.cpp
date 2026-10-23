#include "wifi_analyzer.h"
#include "settings_manager.h"
#include <algorithm>
#include <cstring>
#include <cstdio>

WifiAnalyzer wifiAnalyzer;

bool matchBssidPrefix(const uint8_t* a, const uint8_t* b) {
    if (!a || !b) return false;
    for (uint8_t i = 0; i < 5; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

uint8_t rssiToQuality(int32_t rssi) {
    if (rssi <= RSSI_FLOOR) return 0;
    if (rssi >= -50) return 100;
    return (uint8_t)(2 * (rssi + 100));
}

const char* authModeToString(uint8_t authMode) {
    switch (authMode) {
        case WIFI_AUTH_OPEN:            return "Open";
        case WIFI_AUTH_WEP:             return "WEP";
        case WIFI_AUTH_WPA_PSK:         return "WPA";
        case WIFI_AUTH_WPA2_PSK:        return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "Enterprise";
        case WIFI_AUTH_WPA3_PSK:        return "WPA3";
        default:                        return "Unknown";
    }
}

uint16_t channelToFrequency(uint8_t channel) {
    if (channel >= 1 && channel <= 13) {
        return 2407 + channel * 5;
    } else if (channel == 14) {
        return 2484;
    }
    return 0;
}

float calculateParabolaY(float x, float cx, float spread, float peakHeight, float baseline) {
    if (spread <= 0.0f) return baseline;
    float dx = x - cx;
    if (dx < -spread || dx > spread) {
        return baseline;
    }
    float norm = dx / spread;
    return baseline - peakHeight * (1.0f - norm * norm);
}

void analyzeChannels(const std::vector<AccessPointInfo>& aps, ChannelInfo channels[CHANNEL_COUNT], std::vector<uint8_t>& bestChannels) {
    for (uint8_t c = 0; c < CHANNEL_COUNT; c++) {
        channels[c].channel = c + 1;
        channels[c].apCount = 0;
        channels[c].peakRssi = RSSI_FLOOR;
        channels[c].noiseScore = 0;
        channels[c].peakApIndex = -1;
    }
    bestChannels.clear();

    for (size_t i = 0; i < aps.size(); i++) {
        const auto& ap = aps[i];
        if (ap.channel < 1 || ap.channel > CHANNEL_COUNT) continue;
        uint8_t idx = ap.channel - 1;

        if (ap.rssi > channels[idx].peakRssi) {
            channels[idx].peakRssi = ap.rssi;
            channels[idx].peakApIndex = (int16_t)i;
        }

        bool duplicateBssid = false;
        for (size_t j = 0; j < i; j++) {
            if (aps[j].channel == ap.channel && matchBssidPrefix(aps[j].bssid, ap.bssid)) {
                duplicateBssid = true;
                break;
            }
        }

        if (!duplicateBssid) {
            channels[idx].apCount++;

            int32_t clampedRssi = ap.rssi < RSSI_FLOOR ? RSSI_FLOOR : ap.rssi;
            int64_t noise = clampedRssi - RSSI_FLOOR;
            noise = noise * noise;

            channels[idx].noiseScore += noise;
            if (ap.channel > 1) channels[idx - 1].noiseScore += noise;
            if (ap.channel > 2) channels[idx - 2].noiseScore += noise;
            if (ap.channel > 3) channels[idx - 3].noiseScore += noise;
            if (ap.channel > 4) channels[idx - 4].noiseScore += noise;

            if (ap.channel < 14) channels[idx + 1].noiseScore += noise;
            if (ap.channel < 13) channels[idx + 2].noiseScore += noise;
            if (ap.channel < 12) channels[idx + 3].noiseScore += noise;
            if (ap.channel < 11) channels[idx + 4].noiseScore += noise;
        }
    }

    // Determine channels with minimum noise in range 1-11
    int64_t minNoise = channels[0].noiseScore;
    for (uint8_t c = 1; c < 11; c++) {
        if (channels[c].noiseScore < minNoise) {
            minNoise = channels[c].noiseScore;
        }
    }

    for (uint8_t c = 0; c < 11; c++) {
        if (channels[c].noiseScore == minNoise) {
            bestChannels.push_back(channels[c].channel);
        }
    }
}

WifiAnalyzer::WifiAnalyzer()
    : _isScanning(false), _scanStartTime(0), _lastScanDurationMs(0), _lastScanEndTime(0), _scanCount(0),
      _isTracking(false), _targetBssid(""), _targetSsid(""), _targetChannel(1), _targetRssi(RSSI_FLOOR), _chirpEnabled(false) {
    for (uint8_t c = 0; c < CHANNEL_COUNT; c++) {
        _channels[c].channel = c + 1;
        _channels[c].apCount = 0;
        _channels[c].peakRssi = RSSI_FLOOR;
        _channels[c].noiseScore = 0;
        _channels[c].peakApIndex = -1;
    }
}

void WifiAnalyzer::begin() {
#ifndef NATIVE_TEST
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
#endif
    processScanResults();
}

bool WifiAnalyzer::triggerScan() {
    if (_isScanning) return false;

#ifndef NATIVE_TEST
    _isScanning = true;
    _scanStartTime = millis();
    int16_t status;
    if (_isTracking) {
        status = WiFi.scanNetworks(true, SHOW_HIDDEN, settings.getPassiveScan(), 120, _targetChannel);
    } else {
        Serial.printf("[WiFi] Starting async scan (passive=%s, max_ms=%d)...\n", 
                      settings.getPassiveScan() ? "true" : "false", MAX_MS_PER_CHAN);
        status = WiFi.scanNetworks(true, SHOW_HIDDEN, settings.getPassiveScan(), MAX_MS_PER_CHAN);
    }
    if (status == WIFI_SCAN_FAILED) {
        Serial.println("[WiFi] scanNetworks call failed immediately");
        _isScanning = false;
        return false;
    }
    if (!_isTracking) {
        led.setState(LedManager::STATE_SCANNING);
    }
    return true;
#else
    return false;
#endif
}

bool WifiAnalyzer::update() {
#ifndef NATIVE_TEST
    if (!_isScanning) {
        uint32_t interval = _isTracking ? 50 : settings.getScanIntervalMs();
        if (millis() - _lastScanEndTime >= interval) {
            triggerScan();
        }
        return false;
    }

    int16_t n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) {
        return false;
    }

    _isScanning = false;
    _lastScanEndTime = millis();
    _lastScanDurationMs = _lastScanEndTime - _scanStartTime;
    _scanCount++;

    if (n < 0) {
        Serial.printf("[WiFi] Scan finished with error code: %d (duration: %lu ms)\n", n, _lastScanDurationMs);
        WiFi.scanDelete();
        return false;
    }

    if (_isTracking) {
        bool found = false;
        for (int16_t i = 0; i < n; i++) {
            String bssidStr = WiFi.BSSIDstr(i);
            if (bssidStr.equalsIgnoreCase(_targetBssid)) {
                int32_t currentRssi = WiFi.RSSI(i);
                recordTargetSample((int8_t)currentRssi);
                found = true;
                break;
            }
        }
        if (!found) {
            recordTargetSample((int8_t)RSSI_FLOOR);
        }
        WiFi.scanDelete();
        led.setState(getTrackerLedStatus());
        return true;
    }

    Serial.printf("[WiFi] Scan completed: %d APs found in %lu ms\n", n, _lastScanDurationMs);

    _accessPoints.clear();
    _accessPoints.reserve(n);

    for (int16_t i = 0; i < n; i++) {
        AccessPointInfo info;
        info.ssid = WiFi.SSID(i);
        info.rssi = WiFi.RSSI(i);
        info.channel = (uint8_t)WiFi.channel(i);
        info.authMode = (uint8_t)WiFi.encryptionType(i);
        info.bssidStr = WiFi.BSSIDstr(i);
        info.isHidden = (info.ssid.length() == 0);

        uint8_t* rawBssid = WiFi.BSSID(i);
        if (rawBssid) {
            memcpy(info.bssid, rawBssid, 6);
        } else {
            memset(info.bssid, 0, 6);
        }

        if (info.isHidden) {
            info.ssid = "[Hidden Network]";
        }

        _accessPoints.push_back(info);
    }

    // Sort APs by RSSI descending
    std::sort(_accessPoints.begin(), _accessPoints.end(), [](const AccessPointInfo& a, const AccessPointInfo& b) {
        return a.rssi > b.rssi;
    });

    processScanResults();
    WiFi.scanDelete();
    led.setState(getEnvironmentStatus());
    return true;
#else
    return false;
#endif
}

static const std::vector<int8_t> s_emptyHistory;

const std::vector<int8_t>& WifiAnalyzer::getRssiHistory(const String& bssidStr) const {
    auto it = _bssidHistory.find(bssidStr);
    if (it != _bssidHistory.end()) {
        return it->second;
    }
    return s_emptyHistory;
}

void WifiAnalyzer::recordRssiSample(const String& bssidStr, int8_t rssi) {
    auto& history = _bssidHistory[bssidStr];
    history.push_back(rssi);
    if (history.size() > MAX_HISTORY_SAMPLES) {
        history.erase(history.begin());
    }
}

void WifiAnalyzer::clearHistory() {
    _bssidHistory.clear();
}

void WifiAnalyzer::processScanResults() {
    for (const auto& ap : _accessPoints) {
        if (ap.bssidStr.length() > 0) {
            recordRssiSample(ap.bssidStr, (int8_t)ap.rssi);
        }
    }
    analyzeChannels(_accessPoints, _channels, _bestChannels);
}

void WifiAnalyzer::setScanResults(const std::vector<AccessPointInfo>& aps) {
    _accessPoints = aps;
    processScanResults();
}

void WifiAnalyzer::startTracking(const String& bssidStr, const String& ssid, uint8_t channel) {
    _isTracking = true;
    _targetBssid = bssidStr;
    _targetSsid = ssid;
    _targetChannel = channel;
    _targetRssi = RSSI_FLOOR;
    _targetRssiHistory.clear();
#ifndef NATIVE_TEST
    WiFi.scanDelete();
#endif
    _isScanning = false;
    _lastScanEndTime = 0;
    triggerScan();
}

void WifiAnalyzer::stopTracking() {
    _isTracking = false;
#ifndef NATIVE_TEST
    WiFi.scanDelete();
#endif
    _isScanning = false;
    _lastScanEndTime = millis();
}

void WifiAnalyzer::recordTargetSample(int8_t rssi) {
    _targetRssi = rssi;
    _targetRssiHistory.push_back(rssi);
    if (_targetRssiHistory.size() > MAX_TRACKER_HISTORY) {
        _targetRssiHistory.erase(_targetRssiHistory.begin());
    }
}

int WifiAnalyzer::getTargetTrend() const {
    if (_targetRssiHistory.size() < 4) return 0;
    size_t sz = _targetRssiHistory.size();
    int sum_recent = _targetRssiHistory[sz - 1] + _targetRssiHistory[sz - 2];
    int sum_older = _targetRssiHistory[sz - 3] + _targetRssiHistory[sz - 4];
    if (sum_recent > sum_older + 2) return 1;
    if (sum_recent < sum_older - 2) return -1;
    return 0;
}

const char* WifiAnalyzer::getTargetProximityStr() const {
    if (_targetRssi >= -50) return "Very Close / Immediate";
    if (_targetRssi >= -65) return "Close Range (Strong)";
    if (_targetRssi >= -75) return "Moderate Distance";
    if (_targetRssi >= -85) return "Weak / Distant";
    return "Faint / Out of Range";
}

LedManager::State WifiAnalyzer::getEnvironmentStatus() const {
    if (_accessPoints.empty()) {
        return LedManager::STATE_CLEAN;
    }
    uint16_t maxAps = 0;
    for (int c = 0; c < CHANNEL_COUNT; c++) {
        if (_channels[c].apCount > maxAps) {
            maxAps = _channels[c].apCount;
        }
    }
    if (maxAps > 5) {
        return LedManager::STATE_CONGESTED_RED;
    } else if (maxAps > 2) {
        return LedManager::STATE_CONGESTED_YELLOW;
    } else {
        return LedManager::STATE_CLEAN;
    }
}

LedManager::State WifiAnalyzer::getTrackerLedStatus() const {
    if (_targetRssi >= -65) {
        return LedManager::STATE_CLEAN;
    } else if (_targetRssi >= -80) {
        return LedManager::STATE_CONGESTED_YELLOW;
    } else {
        return LedManager::STATE_CONGESTED_RED;
    }
}

bool isApFiltered(const AccessPointInfo& ap, uint8_t filterMode) {
    bool isHidden = ap.isHidden || ap.ssid.length() == 0 || ap.ssid.startsWith("[Hidden");
    switch (filterMode) {
        case AP_FILTER_OPEN_ONLY:
            return ap.authMode == WIFI_AUTH_OPEN;
        case AP_FILTER_HIDE_HIDDEN:
            return !isHidden;
        case AP_FILTER_OPEN_AND_VISIBLE:
            return (ap.authMode == WIFI_AUTH_OPEN) && !isHidden;
        case AP_FILTER_ALL:
        default:
            return true;
    }
}

static bool compareSsidCaseInsensitive(const AccessPointInfo& a, const AccessPointInfo& b) {
    bool aHidden = a.isHidden || a.ssid.length() == 0 || a.ssid.startsWith("[Hidden");
    bool bHidden = b.isHidden || b.ssid.length() == 0 || b.ssid.startsWith("[Hidden");
    if (aHidden != bHidden) {
        return !aHidden; // Visible networks come before hidden
    }
    size_t lenA = a.ssid.length();
    size_t lenB = b.ssid.length();
    size_t minLen = (lenA < lenB) ? lenA : lenB;
    for (size_t i = 0; i < minLen; i++) {
        char ca = (char)tolower((unsigned char)a.ssid[i]);
        char cb = (char)tolower((unsigned char)b.ssid[i]);
        if (ca != cb) return ca < cb;
    }
    if (lenA != lenB) return lenA < lenB;
    // Tie-breaker: stronger RSSI first
    return a.rssi > b.rssi;
}

std::vector<size_t> filterAndSortAccessPoints(const std::vector<AccessPointInfo>& aps, uint8_t sortMode, uint8_t filterMode) {
    std::vector<size_t> indices;
    indices.reserve(aps.size());
    for (size_t i = 0; i < aps.size(); i++) {
        if (isApFiltered(aps[i], filterMode)) {
            indices.push_back(i);
        }
    }

    std::stable_sort(indices.begin(), indices.end(), [&](size_t i1, size_t i2) {
        const auto& a = aps[i1];
        const auto& b = aps[i2];
        if (sortMode == AP_SORT_SSID) {
            return compareSsidCaseInsensitive(a, b);
        } else if (sortMode == AP_SORT_CHANNEL) {
            if (a.channel != b.channel) {
                return a.channel < b.channel;
            }
            return a.rssi > b.rssi;
        } else { // AP_SORT_RSSI
            if (a.rssi != b.rssi) {
                return a.rssi > b.rssi;
            }
            return a.channel < b.channel;
        }
    });

    return indices;
}
