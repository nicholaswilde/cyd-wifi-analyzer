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
    : _isScanning(false), _scanStartTime(0), _lastScanDurationMs(0), _lastScanEndTime(0), _scanCount(0) {
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
    Serial.printf("[WiFi] Starting async scan (passive=%s, max_ms=%d)...\n", 
                  settings.getPassiveScan() ? "true" : "false", MAX_MS_PER_CHAN);
    int16_t status = WiFi.scanNetworks(true, SHOW_HIDDEN, settings.getPassiveScan(), MAX_MS_PER_CHAN);
    if (status == WIFI_SCAN_FAILED) {
        Serial.println("[WiFi] scanNetworks call failed immediately");
        _isScanning = false;
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool WifiAnalyzer::update() {
#ifndef NATIVE_TEST
    if (!_isScanning) {
        if (millis() - _lastScanEndTime >= settings.getScanIntervalMs()) {
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
