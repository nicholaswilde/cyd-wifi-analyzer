#include "settings_manager.h"
#ifndef NATIVE_TEST
#include <Preferences.h>
#else
#include "Preferences.h"
#endif

SettingsManager settings;

SettingsManager::SettingsManager()
    : _themeFlavor(DEFAULT_THEME_FLAVOR),
      _brightness(100),
      _autoBrightness(false),
      _scanIntervalMs(SCAN_INTERVAL_MS),
      _passiveScan(PASSIVE_SCAN),
      _screenOrientation(DEFAULT_SCREEN_ORIENTATION) {}

void SettingsManager::begin() {
    Preferences prefs;
    if (prefs.begin("wifi_analyzer", true)) {
        _themeFlavor = prefs.getInt("theme", DEFAULT_THEME_FLAVOR);
        _brightness = prefs.getInt("brightness", 100);
        _autoBrightness = prefs.getBool("auto_bright", false);
        _scanIntervalMs = prefs.getUInt("scan_interval", SCAN_INTERVAL_MS);
        _passiveScan = prefs.getBool("passive_scan", PASSIVE_SCAN);
        _screenOrientation = prefs.getInt("orientation", DEFAULT_SCREEN_ORIENTATION);
        prefs.end();
    }
}

void SettingsManager::setThemeFlavor(int flavor) {
    _themeFlavor = flavor;
    Preferences prefs;
    if (prefs.begin("wifi_analyzer", false)) {
        prefs.putInt("theme", _themeFlavor);
        prefs.end();
    }
}

void SettingsManager::setBrightness(int brightness) {
    if (brightness < 10) brightness = 10;
    if (brightness > 100) brightness = 100;
    _brightness = brightness;
    Preferences prefs;
    if (prefs.begin("wifi_analyzer", false)) {
        prefs.putInt("brightness", _brightness);
        prefs.end();
    }
}

void SettingsManager::setAutoBrightness(bool enabled) {
    _autoBrightness = enabled;
    Preferences prefs;
    if (prefs.begin("wifi_analyzer", false)) {
        prefs.putBool("auto_bright", _autoBrightness);
        prefs.end();
    }
}

void SettingsManager::setScanIntervalMs(uint32_t ms) {
    if (ms < 500) ms = 500;
    _scanIntervalMs = ms;
    Preferences prefs;
    if (prefs.begin("wifi_analyzer", false)) {
        prefs.putUInt("scan_interval", _scanIntervalMs);
        prefs.end();
    }
}

void SettingsManager::setPassiveScan(bool passive) {
    _passiveScan = passive;
    Preferences prefs;
    if (prefs.begin("wifi_analyzer", false)) {
        prefs.putBool("passive_scan", _passiveScan);
        prefs.end();
    }
}

void SettingsManager::setScreenOrientation(int orientation) {
    _screenOrientation = orientation;
    Preferences prefs;
    if (prefs.begin("wifi_analyzer", false)) {
        prefs.putInt("orientation", _screenOrientation);
        prefs.end();
    }
}

void SettingsManager::resetToDefaults() {
    _themeFlavor = DEFAULT_THEME_FLAVOR;
    _brightness = 100;
    _autoBrightness = false;
    _scanIntervalMs = SCAN_INTERVAL_MS;
    _passiveScan = PASSIVE_SCAN;
    _screenOrientation = DEFAULT_SCREEN_ORIENTATION;

    Preferences prefs;
    if (prefs.begin("wifi_analyzer", false)) {
        prefs.clear();
        prefs.end();
    }
}
