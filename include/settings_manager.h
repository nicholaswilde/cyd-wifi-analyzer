#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <stdint.h>
#include "config/config.h"

#ifdef NATIVE_TEST
#include "Arduino.h"
#else
#include <Arduino.h>
#endif

class SettingsManager {
public:
    SettingsManager();
    void begin();

    int getThemeFlavor() const { return _themeFlavor; }
    void setThemeFlavor(int flavor);

    int getBrightness() const { return _brightness; }
    void setBrightness(int brightness);

    bool getAutoBrightness() const { return _autoBrightness; }
    void setAutoBrightness(bool enabled);

    uint32_t getScanIntervalMs() const { return _scanIntervalMs; }
    void setScanIntervalMs(uint32_t ms);

    bool getPassiveScan() const { return _passiveScan; }
    void setPassiveScan(bool passive);

    int getScreenOrientation() const { return _screenOrientation; }
    void setScreenOrientation(int orientation);

    bool getLedEnabled() const { return _ledEnabled; }
    void setLedEnabled(bool enabled);

    int getLedBrightness() const { return _ledBrightness; }
    void setLedBrightness(int brightness);

    void resetToDefaults();

private:
    int _themeFlavor;
    int _brightness;
    bool _autoBrightness;
    uint32_t _scanIntervalMs;
    bool _passiveScan;
    int _screenOrientation;
    bool _ledEnabled;
    int _ledBrightness;
};

extern SettingsManager settings;

#endif // SETTINGS_MANAGER_H
