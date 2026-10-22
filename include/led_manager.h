#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include "config/config.h"

class LedManager {
public:
    enum State {
        STATE_OFF,
        STATE_SCANNING,
        STATE_CLEAN,
        STATE_CONGESTED_YELLOW,
        STATE_CONGESTED_RED,
        STATE_PULSE_BLUE,
        STATE_PULSE_GREEN,
        STATE_PULSE_YELLOW,
        STATE_PULSE_WHITE,
        STATE_ALERT_RED,
        STATE_CONNECTING = STATE_SCANNING,
        STATE_CONNECTED = STATE_CLEAN,
        STATE_DISCONNECTED = STATE_ALERT_RED,
        STATE_AP_MODE = STATE_CONGESTED_YELLOW
    };

    LedManager(int redPin = CYD_LED_RED, int greenPin = CYD_LED_GREEN, int bluePin = CYD_LED_BLUE);
    void begin();
    void update(unsigned long currentMillis);
    void setState(State state);
    State getState() const;
    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setBrightness(int brightness);
    int getBrightness() const;

private:
    void writePins(bool r, bool g, bool b);
    void turnOffAll();

    int _redPin;
    int _greenPin;
    int _bluePin;
    State _state;
    bool _enabled;
    int _brightness;

    unsigned long _lastToggleTime;
    bool _blinkState;
    unsigned long _stateStartTime;
    bool _stateInitialized;
};

extern LedManager led;

#endif // LED_MANAGER_H
