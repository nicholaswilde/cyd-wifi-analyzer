#include <Arduino.h>
#include <lvgl.h>
#include "config/config.h"
#include "settings_manager.h"
#include "display.h"
#include "backlight_manager.h"
#include "wifi_analyzer.h"
#include "ui.h"

BacklightManager backlight(TFT_BL, 0, 10.0f);

static unsigned long lastBacklightUpdate = 0;

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n--- ESP32 CYD Wi-Fi Analyzer Starting ---");

    // Initialize Settings from NVS
    settings.begin();

    // Initialize Backlight
    backlight.begin();
    backlight.setManualBrightness(settings.getBrightness());

    // Initialize Display and Touch
    initDisplayAndTouch();

    // Initialize LVGL core & drivers
    initLVGL();

    // Initialize LVGL UI widgets & tabs
    initUI();

    // Initialize Wi-Fi Analyzer engine
    wifiAnalyzer.begin();
    wifiAnalyzer.triggerScan();

    Serial.println("Initialization complete, starting scanner loop.");
}

void loop() {
    // Let LVGL process UI events and render dirty areas
    lv_timer_handler();

    // Check Wi-Fi scan status and update UI upon scan completion
    if (wifiAnalyzer.update()) {
        updateUI();
    }

    // Periodically update backlight from ambient light sensor (LDR) if auto-brightness is enabled
    if (settings.getAutoBrightness()) {
        unsigned long now = millis();
        if (now - lastBacklightUpdate >= 500) {
            lastBacklightUpdate = now;
            uint16_t ldrVal = analogRead(LDR_PIN);
            backlight.update(ldrVal);
        }
    }

    delay(5);
}
