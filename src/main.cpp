#include <Arduino.h>
#include <lvgl.h>
#include "config/config.h"
#include "settings_manager.h"
#include "display.h"
#include "backlight_manager.h"
#include "wifi_analyzer.h"
#include "led_manager.h"
#include "screenshot_manager.h"
#include "sd_card_manager.h"
#include "survey_manager.h"
#include "button_manager.h"
#include "ui.h"

BacklightManager backlight(TFT_BL, 0, 10.0f);
ButtonManager buttonManager(BOOT_BUTTON_PIN);

static unsigned long lastBacklightUpdate = 0;

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n--- ESP32 CYD Wi-Fi Analyzer Starting ---");

    // Initialize Settings from NVS
    settings.begin();
    wifiAnalyzer.setRegDomain((uint8_t)settings.getRegDomain());

    // Initialize Screenshot Manager (USB Serial frame dump listener)
    ScreenshotManager::init();

    // Initialize RGB Status LED
    led.begin();
    led.setEnabled(settings.getLedEnabled());
    led.setBrightness(settings.getLedBrightness());

    // Initialize physical BOOT button (GPIO 0)
    buttonManager.begin();

    // Initialize MicroSD Card Manager
    SdCardManager::begin();

    // Initialize Display and Touch (must run before backlight so tft.init does not clobber LEDC PWM pin)
    initDisplayAndTouch();

    // Initialize Backlight (configured after display init so LEDC controls TFT_BL)
    backlight.begin();
    if (settings.getAutoBrightness()) {
        uint16_t ldrVal = analogRead(LDR_PIN);
        backlight.update(ldrVal);
    } else {
        backlight.setManualBrightness(settings.getBrightness());
    }

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
    // Check USB Serial for screenshot capture or tab navigation commands
    ScreenshotManager::checkSerial();

    // Check physical BOOT button actions
    ButtonAction action = buttonManager.update(millis());
    if (action == ButtonAction::SINGLE_PRESS) {
        Serial.println("[System] BOOT button single press: Triggering Wi-Fi rescan...");
        showUIStatusMessage("Scanning Wi-Fi...");
        wifiAnalyzer.triggerScan();
        led.setState(LedManager::STATE_SCANNING);
    } else if (action == ButtonAction::LONG_PRESS) {
        Serial.println("[System] BOOT button long press: Capturing screenshot to SD...");
        std::string filename = ScreenshotManager::generateFilename(nullptr, millis());
        bool ok = ScreenshotManager::captureToSD(filename.c_str());
        if (ok) {
            showUIStatusMessage("Screenshot saved!");
            led.setState(LedManager::STATE_PULSE_WHITE);
        } else {
            showUIStatusMessage("SD Card Missing!");
            led.setState(LedManager::STATE_ALERT_RED);
        }
    }

    // Non-blocking RGB status LED updates
    led.update(millis());

    // Let LVGL process UI events and render dirty areas
    lv_timer_handler();

    // Check Wi-Fi scan status and update UI upon scan completion
    if (wifiAnalyzer.update()) {
        updateUI();
        if (settings.getScanSurveyEnabled() && SdCardManager::isCardPresent()) {
            SurveyManager::appendScanToSurvey(wifiAnalyzer.getAccessPoints());
        }
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
