#include "sd_card_manager.h"
#include "config/config.h"

#ifndef NATIVE_TEST
#include <SPI.h>
#include <SD.h>
#include <FS.h>

static SPIClass sdSPI(VSPI);
static bool sdMounted = false;
static bool sdCardDetected = false;

bool SdCardManager::begin() {
    if (sdMounted) {
        return true;
    }

    Serial.println("[SD] Initializing SD card...");

    sdSPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    sdMounted = SD.begin(SD_CS_PIN, sdSPI, 4000000, "/sd", 5, true);

    if (sdMounted) {
        Serial.println("[SD] SD card initialized successfully.");
        sdCardDetected = true;
    } else {
        Serial.println("[SD] Failed to initialize SD card.");
        sdCardDetected = false;
    }
    return sdMounted;
}

bool SdCardManager::isMounted() {
    return sdMounted;
}

void SdCardManager::end() {
    if (sdMounted) {
        SD.end();
        sdMounted = false;
        Serial.println("[SD] SD card unmounted.");
    }
    sdSPI.end();
}

bool SdCardManager::isCardPresent() {
    return sdCardDetected;
}

#else
// Host-native mock implementation for testing
#include "SD.h"
#include "FS.h"
#include "SPI.h"

extern bool mock_sd_card_present;
extern bool mock_sd_card_mounted;

bool SdCardManager::begin() {
    if (mock_sd_card_present) {
        mock_sd_card_mounted = true;
        return true;
    }
    mock_sd_card_mounted = false;
    return false;
}

bool SdCardManager::isMounted() {
    return mock_sd_card_mounted;
}

void SdCardManager::end() {
    mock_sd_card_mounted = false;
}

bool SdCardManager::isCardPresent() {
    return mock_sd_card_present;
}
#endif
