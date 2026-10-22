#include "led_manager.h"

#ifndef NATIVE_TEST
#include <Arduino.h>
#include <math.h>
#else
#include "Arduino.h"
#include <cmath>
#endif

LedManager led(CYD_LED_RED, CYD_LED_GREEN, CYD_LED_BLUE);

LedManager::LedManager(int redPin, int greenPin, int bluePin)
    : _redPin(redPin), _greenPin(greenPin), _bluePin(bluePin),
      _state(STATE_OFF), _enabled(true), _brightness(255),
      _lastToggleTime(0), _blinkState(false), _stateStartTime(0),
      _stateInitialized(false) {}

void LedManager::begin() {
#ifndef NATIVE_TEST
    #if defined(ESP_ARDUINO_VERSION) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcAttach(_redPin, 5000, 8);
        ledcAttach(_greenPin, 5000, 8);
        ledcAttach(_bluePin, 5000, 8);
    #else
        ledcSetup(5, 5000, 8); // channel 5 = red,   5 kHz, 8-bit
        ledcSetup(6, 5000, 8); // channel 6 = green, 5 kHz, 8-bit
        ledcSetup(7, 5000, 8); // channel 7 = blue,  5 kHz, 8-bit
        ledcAttachPin(_redPin,   5);
        ledcAttachPin(_greenPin, 6);
        ledcAttachPin(_bluePin,  7);
    #endif
#else
    pinMode(_redPin, OUTPUT);
    pinMode(_greenPin, OUTPUT);
    pinMode(_bluePin, OUTPUT);
#endif
    turnOffAll();
    _stateInitialized = false;
}

void LedManager::writePins(bool r, bool g, bool b) {
    if (!_enabled) {
        turnOffAll();
        return;
    }

#ifndef NATIVE_TEST
    // Apply Gamma Correction (2.2) for natural human-eye brightness perception
    int pwmBrightness = 0;
    if (_brightness > 0) {
        float normalized = _brightness / 255.0f;
        pwmBrightness = (int)(pow(normalized, 2.2f) * 255.0f);
        if (pwmBrightness == 0) pwmBrightness = 1; // Keep slightly illuminated if > 0
    }

    // Pins are active-LOW: duty 255 = full OFF, 0 = full ON
    int rDuty = r ? (255 - pwmBrightness) : 255;
    int gDuty = g ? (255 - pwmBrightness) : 255;
    int bDuty = b ? (255 - pwmBrightness) : 255;

    #if defined(ESP_ARDUINO_VERSION) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWrite(_redPin, rDuty);
        ledcWrite(_greenPin, gDuty);
        ledcWrite(_bluePin, bDuty);
    #else
        ledcWrite(5, rDuty);
        ledcWrite(6, gDuty);
        ledcWrite(7, bDuty);
    #endif
#else
    // Active-LOW digital simulation for tests
    digitalWrite(_redPin, r ? LOW : HIGH);
    digitalWrite(_greenPin, g ? LOW : HIGH);
    digitalWrite(_bluePin, b ? LOW : HIGH);
#endif
}

void LedManager::turnOffAll() {
#ifndef NATIVE_TEST
    #if defined(ESP_ARDUINO_VERSION) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
        ledcWrite(_redPin, 255);
        ledcWrite(_greenPin, 255);
        ledcWrite(_bluePin, 255);
    #else
        ledcWrite(5, 255);
        ledcWrite(6, 255);
        ledcWrite(7, 255);
    #endif
#else
    digitalWrite(_redPin, HIGH);
    digitalWrite(_greenPin, HIGH);
    digitalWrite(_bluePin, HIGH);
#endif
}

void LedManager::update(unsigned long currentMillis) {
    if (!_enabled) {
        turnOffAll();
        return;
    }

    if (!_stateInitialized) {
        _stateStartTime = currentMillis;
        _lastToggleTime = currentMillis;
        _blinkState = true;
        _stateInitialized = true;
    }

    switch (_state) {
        case STATE_OFF:
            writePins(false, false, false);
            break;

        case STATE_SCANNING:
            // Blue pulse/blink (400ms toggle)
            if (currentMillis - _lastToggleTime >= 400) {
                _blinkState = !_blinkState;
                _lastToggleTime = currentMillis;
            }
            writePins(false, false, _blinkState);
            break;

        case STATE_CLEAN:
            // Solid Green
            writePins(false, true, false);
            break;

        case STATE_CONGESTED_YELLOW:
            // Solid Yellow (Red + Green)
            writePins(true, true, false);
            break;

        case STATE_CONGESTED_RED:
            // Solid Red
            writePins(true, false, false);
            break;

        case STATE_PULSE_BLUE:
            if (currentMillis - _stateStartTime >= 1000) {
                _state = STATE_OFF;
                _stateStartTime = 0;
                writePins(false, false, false);
            } else {
                writePins(false, false, true);
            }
            break;

        case STATE_PULSE_GREEN:
            if (currentMillis - _stateStartTime >= 1000) {
                _state = STATE_OFF;
                _stateStartTime = 0;
                writePins(false, false, false);
            } else {
                writePins(false, true, false);
            }
            break;

        case STATE_PULSE_YELLOW:
            if (currentMillis - _stateStartTime >= 1000) {
                _state = STATE_OFF;
                _stateStartTime = 0;
                writePins(false, false, false);
            } else {
                writePins(true, true, false);
            }
            break;

        case STATE_PULSE_WHITE:
            if (currentMillis - _stateStartTime >= 1000) {
                _state = STATE_OFF;
                _stateStartTime = 0;
                writePins(false, false, false);
            } else {
                writePins(true, true, true);
            }
            break;

        case STATE_ALERT_RED:
            // Rapid blink red (200ms toggle)
            if (currentMillis - _lastToggleTime >= 200) {
                _blinkState = !_blinkState;
                _lastToggleTime = currentMillis;
            }
            writePins(_blinkState, false, false);
            break;
    }
}

void LedManager::setState(State state) {
    if (_state != state) {
        _state = state;
        _stateStartTime = 0;
        _lastToggleTime = 0;
        _blinkState = true;
        _stateInitialized = false;
    }
}

LedManager::State LedManager::getState() const {
    return _state;
}

void LedManager::setEnabled(bool enabled) {
    _enabled = enabled;
    if (!_enabled) {
        turnOffAll();
    }
}

bool LedManager::isEnabled() const {
    return _enabled;
}

void LedManager::setBrightness(int brightness) {
    if (brightness < 0) brightness = 0;
    if (brightness > 255) brightness = 255;
    _brightness = brightness;
}

int LedManager::getBrightness() const {
    return _brightness;
}
