#ifndef DISPLAY_H
#define DISPLAY_H

#include <TFT_eSPI.h>
#include <lvgl.h>

extern TFT_eSPI tft;

void initDisplayAndTouch();
void initLVGL();
void mapTouchCoordinates(int16_t raw_x, int16_t raw_y, int16_t& out_x, int16_t& out_y, int orientation);

#endif // DISPLAY_H
