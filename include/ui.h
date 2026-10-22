#ifndef UI_H
#define UI_H

#include <lvgl.h>
#include "wifi_analyzer.h"

void initUI();
void updateUI();
void refreshThemeUI();
void setTab(uint16_t index);
uint16_t getTab();

void openTrackerView(const String& bssidStr, const String& ssid, uint8_t channel);
void closeTrackerView();

#endif // UI_H
