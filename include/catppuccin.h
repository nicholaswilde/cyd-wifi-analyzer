#ifndef CATPPUCCIN_H
#define CATPPUCCIN_H

#include <CatppuccinRGB565.h>
#include <lvgl.h>

#define CATPPUCCIN_MOCHA      1
#define CATPPUCCIN_MACCHIATO  2
#define CATPPUCCIN_FRAPPE     3
#define CATPPUCCIN_LATTE      4

const Catppuccin::Palette& getCatppuccinFlavor(int flavor = CATPPUCCIN_MOCHA);
int getCurrentThemeFlavor();
void setCurrentThemeFlavor(int flavor);
uint16_t getChannelColor(uint8_t channel, int flavor = CATPPUCCIN_MOCHA);
lv_color_t getChannelLvColor(uint8_t channel, int flavor = CATPPUCCIN_MOCHA);

#endif // CATPPUCCIN_H
