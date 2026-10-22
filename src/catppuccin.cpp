#include "catppuccin.h"
#include "settings_manager.h"

const Catppuccin::Palette& getCatppuccinFlavor(int flavor) {
    switch (flavor) {
        case CATPPUCCIN_MACCHIATO: return Catppuccin::getPalette(Catppuccin::Flavor::Macchiato);
        case CATPPUCCIN_FRAPPE:    return Catppuccin::getPalette(Catppuccin::Flavor::Frappe);
        case CATPPUCCIN_LATTE:     return Catppuccin::getPalette(Catppuccin::Flavor::Latte);
        case CATPPUCCIN_MOCHA:
        default:                   return Catppuccin::getPalette(Catppuccin::Flavor::Mocha);
    }
}

int getCurrentThemeFlavor() {
    return settings.getThemeFlavor();
}

void setCurrentThemeFlavor(int flavor) {
    settings.setThemeFlavor(flavor);
}

uint16_t getChannelColor(uint8_t channel, int flavor) {
    const auto& p = getCatppuccinFlavor(flavor);
    // Map channels 1-14 to 14 distinct Catppuccin palette colors
    switch ((channel - 1) % 14) {
        case 0:  return p.red;
        case 1:  return p.peach;
        case 2:  return p.yellow;
        case 3:  return p.green;
        case 4:  return p.teal;
        case 5:  return p.sky;
        case 6:  return p.sapphire;
        case 7:  return p.blue;
        case 8:  return p.lavender;
        case 9:  return p.mauve;
        case 10: return p.pink;
        case 11: return p.flamingo;
        case 12: return p.maroon;
        case 13: return p.rosewater;
        default: return p.text;
    }
}

lv_color_t getChannelLvColor(uint8_t channel, int flavor) {
    uint16_t rgb565 = getChannelColor(channel, flavor);
    lv_color_t color;
    color.full = rgb565;
    return color;
}
