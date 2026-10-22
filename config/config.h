#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include "catppuccin.h"

// Display Performance Settings
#define DISPLAY_DRAW_BUF_ROWS 30
#define DISPLAY_REFR_PERIOD_MS 20
#define DISPLAY_INDEV_READ_PERIOD_MS 10
#define DEFAULT_SCREEN_ORIENTATION 1  // Landscape

// Catppuccin Flavor Definitions
#define CATPPUCCIN_MOCHA      1
#define CATPPUCCIN_MACCHIATO  2
#define CATPPUCCIN_FRAPPE     3
#define CATPPUCCIN_LATTE      4

// Default Theme Flavor
#define DEFAULT_THEME_FLAVOR CATPPUCCIN_MOCHA

#define TO_LV_COLOR(c) (lv_color_t){.full = (c)}

// Wi-Fi Analyzer Settings
#define SCAN_INTERVAL_MS 3000
#define MAX_MS_PER_CHAN 400
#define PASSIVE_SCAN false
#define SHOW_HIDDEN true

// Signal thresholds
#define RSSI_CEILING -40
#define RSSI_FLOOR -100

// 2.4 GHz Channels
#define MIN_CHANNEL 1
#define MAX_CHANNEL 14
#define CHANNEL_COUNT 14

// Hardware Pins
#define LDR_PIN 34
#define BOOT_BUTTON_PIN 0
#define SPEAKER_PIN 26

// RGB LED Pins (Active LOW)
#ifndef CYD_LED_RED
#define CYD_LED_RED 4
#endif

#ifndef CYD_LED_GREEN
#define CYD_LED_GREEN 16
#endif

#ifndef CYD_LED_BLUE
#define CYD_LED_BLUE 17
#endif

// Default LED Settings
#define DEFAULT_LED_ENABLED true
#define DEFAULT_LED_BRIGHTNESS 255

#endif // CONFIG_H
