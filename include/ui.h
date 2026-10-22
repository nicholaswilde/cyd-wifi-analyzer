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
void openDetailModal();
void closeDetailModal();
void openTrackerDemo();
void showUIStatusMessage(const char* message);

#endif // UI_H
