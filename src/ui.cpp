#include "ui.h"
#include "config/config.h"
#include "catppuccin.h"
#include "backlight_manager.h"
#include "settings_manager.h"
#include "led_manager.h"
#include "display.h"
#include <cstdio>
#include <cstring>
#include <vector>

extern BacklightManager backlight;

static lv_obj_t* s_tabview = nullptr;
static lv_obj_t* s_tab_spectrum = nullptr;
static lv_obj_t* s_tab_networks = nullptr;
static lv_obj_t* s_tab_channels = nullptr;
static lv_obj_t* s_tab_settings = nullptr;

// Spectrum Tab Widgets
static lv_obj_t* s_lbl_spectrum_summary = nullptr;
static lv_obj_t* s_spectrum_view = nullptr;

// Networks Tab Widgets
static lv_obj_t* s_list_networks = nullptr;

// Detail Modal Widgets
static lv_obj_t* s_detail_modal = nullptr;
static String s_modal_bssid = "";
static String s_modal_ssid = "";
static uint8_t s_modal_channel = 1;
static lv_obj_t* s_modal_lbl_signal = nullptr;
static lv_obj_t* s_modal_bar_signal = nullptr;
static lv_obj_t* s_modal_chart_history = nullptr;
static lv_chart_series_t* s_modal_ser_history = nullptr;

// Tracker View Widgets
static lv_obj_t* s_tracker_view = nullptr;
static lv_obj_t* s_tracker_lbl_ssid = nullptr;
static lv_obj_t* s_tracker_lbl_bssid_ch = nullptr;
static lv_obj_t* s_tracker_meter_arc = nullptr;
static lv_obj_t* s_tracker_lbl_rssi = nullptr;
static lv_obj_t* s_tracker_lbl_proximity = nullptr;
static lv_obj_t* s_tracker_lbl_trend = nullptr;
static lv_obj_t* s_tracker_chart = nullptr;
static lv_chart_series_t* s_tracker_ser = nullptr;
static lv_obj_t* s_tracker_btn_audio = nullptr;
static lv_obj_t* s_tracker_lbl_audio = nullptr;
static lv_obj_t* s_tracker_btn_exit = nullptr;
static lv_obj_t* s_tracker_lbl_stats = nullptr;

// Channels Tab Widgets
static lv_obj_t* s_table_channels = nullptr;

// Settings Tab Widgets
static lv_obj_t* s_dropdown_theme = nullptr;
static lv_obj_t* s_slider_brightness = nullptr;
static lv_obj_t* s_switch_auto_bright = nullptr;
static lv_obj_t* s_dropdown_interval = nullptr;
static lv_obj_t* s_switch_passive = nullptr;
static lv_obj_t* s_dropdown_rotation = nullptr;
static lv_obj_t* s_switch_led = nullptr;
static lv_obj_t* s_slider_led_bright = nullptr;
static lv_obj_t* s_btn_reset = nullptr;
static lv_obj_t* s_lbl_info = nullptr;

static lv_obj_t* s_lbl_theme = nullptr;
static lv_obj_t* s_lbl_bright = nullptr;
static lv_obj_t* s_lbl_auto_bright = nullptr;
static lv_obj_t* s_lbl_interval = nullptr;
static lv_obj_t* s_lbl_passive = nullptr;
static lv_obj_t* s_lbl_rotation = nullptr;
static lv_obj_t* s_lbl_led = nullptr;
static lv_obj_t* s_lbl_led_bright = nullptr;

static uint16_t scanIntervalToDropdownIdx(uint32_t ms) {
    if (ms <= 1000) return 0;
    if (ms <= 2000) return 1;
    if (ms <= 3000) return 2;
    return 3;
}

static uint32_t dropdownIdxToScanInterval(uint16_t idx) {
    switch (idx) {
        case 0: return 1000;
        case 1: return 2000;
        case 2: return 3000;
        case 3: return 5000;
        default: return 3000;
    }
}

static uint16_t orientationToDropdownIdx(int ori) {
    return (ori == 3) ? 1 : 0;
}

static int dropdownIdxToOrientation(uint16_t idx) {
    return (idx == 1) ? 3 : 1;
}

static void on_brightness_changed(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
    backlight.setManualBrightness((uint8_t)val);
    settings.setBrightness((int)val);
}

static void on_theme_changed(lv_event_t* e) {
    lv_obj_t* dropdown = lv_event_get_target(e);
    uint16_t selected = lv_dropdown_get_selected(dropdown);
    int flavor = CATPPUCCIN_MOCHA;
    switch (selected) {
        case 0: flavor = CATPPUCCIN_MOCHA; break;
        case 1: flavor = CATPPUCCIN_MACCHIATO; break;
        case 2: flavor = CATPPUCCIN_FRAPPE; break;
        case 3: flavor = CATPPUCCIN_LATTE; break;
        default: flavor = CATPPUCCIN_MOCHA; break;
    }
    setCurrentThemeFlavor(flavor);
    refreshThemeUI();
}

static void on_auto_bright_changed(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    settings.setAutoBrightness(state);
}

static void on_interval_changed(lv_event_t* e) {
    lv_obj_t* dropdown = lv_event_get_target(e);
    uint16_t idx = lv_dropdown_get_selected(dropdown);
    settings.setScanIntervalMs(dropdownIdxToScanInterval(idx));
}

static void on_passive_scan_changed(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    settings.setPassiveScan(state);
}

static void on_rotation_changed(lv_event_t* e) {
    lv_obj_t* dropdown = lv_event_get_target(e);
    uint16_t idx = lv_dropdown_get_selected(dropdown);
    int ori = dropdownIdxToOrientation(idx);
    settings.setScreenOrientation(ori);
    tft.setRotation(ori);
    if (lv_scr_act()) {
        lv_obj_invalidate(lv_scr_act());
    }
}

static void on_led_toggle_changed(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target(e);
    bool en = lv_obj_has_state(sw, LV_STATE_CHECKED);
    settings.setLedEnabled(en);
    led.setEnabled(en);
}

static void on_led_brightness_changed(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    settings.setLedBrightness(val);
    led.setBrightness(val);
}

static void on_reset_settings_clicked(lv_event_t* e) {
    settings.resetToDefaults();

    if (s_dropdown_theme) {
        lv_dropdown_set_selected(s_dropdown_theme, settings.getThemeFlavor() - 1);
    }
    if (s_slider_brightness) {
        lv_slider_set_value(s_slider_brightness, settings.getBrightness(), LV_ANIM_OFF);
        backlight.setManualBrightness((uint8_t)settings.getBrightness());
    }
    if (s_switch_auto_bright) {
        if (settings.getAutoBrightness()) {
            lv_obj_add_state(s_switch_auto_bright, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(s_switch_auto_bright, LV_STATE_CHECKED);
        }
    }
    if (s_dropdown_interval) {
        lv_dropdown_set_selected(s_dropdown_interval, scanIntervalToDropdownIdx(settings.getScanIntervalMs()));
    }
    if (s_switch_passive) {
        if (settings.getPassiveScan()) {
            lv_obj_add_state(s_switch_passive, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(s_switch_passive, LV_STATE_CHECKED);
        }
    }
    if (s_dropdown_rotation) {
        lv_dropdown_set_selected(s_dropdown_rotation, orientationToDropdownIdx(settings.getScreenOrientation()));
        tft.setRotation(settings.getScreenOrientation());
    }
    led.setEnabled(settings.getLedEnabled());
    led.setBrightness(settings.getLedBrightness());
    if (s_switch_led) {
        if (settings.getLedEnabled()) {
            lv_obj_add_state(s_switch_led, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(s_switch_led, LV_STATE_CHECKED);
        }
    }
    if (s_slider_led_bright) {
        lv_slider_set_value(s_slider_led_bright, settings.getLedBrightness(), LV_ANIM_OFF);
    }
    refreshThemeUI();
}

static void playTrackerChirp(int32_t rssi) {
    if (!wifiAnalyzer.isChirpEnabled() || rssi < -95) return;
    int freq = (int)(400 + (rssi + 95) * 35);
    if (freq < 300) freq = 300;
    if (freq > 3000) freq = 3000;
#ifndef NATIVE_TEST
    tone(SPEAKER_PIN, freq, 25);
#endif
}

static void on_tracker_exit_clicked(lv_event_t* e) {
    closeTrackerView();
}

static void on_tracker_audio_toggle_clicked(lv_event_t* e) {
    bool en = !wifiAnalyzer.isChirpEnabled();
    wifiAnalyzer.setChirpEnabled(en);
    if (s_tracker_lbl_audio) {
        lv_label_set_text(s_tracker_lbl_audio, en ? LV_SYMBOL_VOLUME_MAX " Beep: ON" : LV_SYMBOL_MUTE " Beep: OFF");
        const auto& p = getCatppuccinFlavor(getCurrentThemeFlavor());
        lv_obj_set_style_text_color(s_tracker_lbl_audio, en ? TO_LV_COLOR(p.green) : TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    }
}

void closeTrackerView() {
    wifiAnalyzer.stopTracking();
    if (s_tracker_view) {
        lv_obj_del(s_tracker_view);
        s_tracker_view = nullptr;
        s_tracker_lbl_ssid = nullptr;
        s_tracker_lbl_bssid_ch = nullptr;
        s_tracker_meter_arc = nullptr;
        s_tracker_lbl_rssi = nullptr;
        s_tracker_lbl_proximity = nullptr;
        s_tracker_lbl_trend = nullptr;
        s_tracker_chart = nullptr;
        s_tracker_ser = nullptr;
        s_tracker_btn_audio = nullptr;
        s_tracker_lbl_audio = nullptr;
        s_tracker_btn_exit = nullptr;
        s_tracker_lbl_stats = nullptr;
    }
    updateUI();
}

void openTrackerView(const String& bssidStr, const String& ssid, uint8_t channel) {
    if (s_tracker_view) {
        closeTrackerView();
    }

    wifiAnalyzer.startTracking(bssidStr, ssid, channel);
    const auto& p = getCatppuccinFlavor(getCurrentThemeFlavor());

    s_tracker_view = lv_obj_create(lv_scr_act());
    lv_obj_set_size(s_tracker_view, LV_PCT(100), LV_PCT(100));
    lv_obj_center(s_tracker_view);
    lv_obj_clear_flag(s_tracker_view, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_tracker_view, TO_LV_COLOR(p.base), LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_tracker_view, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_tracker_view, 0, LV_PART_MAIN);

    // 1. Header: Target SSID & Details + Exit Button
    s_tracker_lbl_ssid = lv_label_create(s_tracker_view);
    lv_label_set_text(s_tracker_lbl_ssid, ssid.c_str());
    lv_obj_set_style_text_color(s_tracker_lbl_ssid, TO_LV_COLOR(p.mauve), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_tracker_lbl_ssid, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_size(s_tracker_lbl_ssid, 220, 18);
    lv_label_set_long_mode(s_tracker_lbl_ssid, LV_LABEL_LONG_DOT);
    lv_obj_align(s_tracker_lbl_ssid, LV_ALIGN_TOP_LEFT, 4, 2);

    char chBuf[64];
    snprintf(chBuf, sizeof(chBuf), "BSSID: %s | CH %d (%d MHz)", bssidStr.c_str(), (int)channel, (int)channelToFrequency(channel));
    s_tracker_lbl_bssid_ch = lv_label_create(s_tracker_view);
    lv_label_set_text(s_tracker_lbl_bssid_ch, chBuf);
    lv_obj_set_style_text_color(s_tracker_lbl_bssid_ch, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_tracker_lbl_bssid_ch, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(s_tracker_lbl_bssid_ch, LV_ALIGN_TOP_LEFT, 4, 22);

    s_tracker_btn_exit = lv_btn_create(s_tracker_view);
    lv_obj_set_size(s_tracker_btn_exit, 65, 26);
    lv_obj_align(s_tracker_btn_exit, LV_ALIGN_TOP_RIGHT, 0, 2);
    lv_obj_set_style_bg_color(s_tracker_btn_exit, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
    lv_obj_set_style_radius(s_tracker_btn_exit, 4, LV_PART_MAIN);
    lv_obj_add_event_cb(s_tracker_btn_exit, on_tracker_exit_clicked, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* lbl_exit = lv_label_create(s_tracker_btn_exit);
    lv_label_set_text(lbl_exit, LV_SYMBOL_CLOSE " Exit");
    lv_obj_set_style_text_color(lbl_exit, TO_LV_COLOR(p.red), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_exit, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_center(lbl_exit);

    // 2. Left Column: Circular Arc Meter
    s_tracker_meter_arc = lv_arc_create(s_tracker_view);
    lv_obj_set_size(s_tracker_meter_arc, 130, 130);
    lv_obj_align(s_tracker_meter_arc, LV_ALIGN_TOP_LEFT, 6, 42);
    lv_arc_set_rotation(s_tracker_meter_arc, 135);
    lv_arc_set_bg_angles(s_tracker_meter_arc, 0, 270);
    lv_arc_set_range(s_tracker_meter_arc, 0, 100);
    lv_arc_set_value(s_tracker_meter_arc, 0);
    lv_obj_clear_flag(s_tracker_meter_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_style(s_tracker_meter_arc, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(s_tracker_meter_arc, 9, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_tracker_meter_arc, 9, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_tracker_meter_arc, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_tracker_meter_arc, TO_LV_COLOR(p.green), LV_PART_INDICATOR);

    s_tracker_lbl_rssi = lv_label_create(s_tracker_view);
    lv_label_set_text(s_tracker_lbl_rssi, "-- dBm");
    lv_obj_set_style_text_color(s_tracker_lbl_rssi, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_tracker_lbl_rssi, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align_to(s_tracker_lbl_rssi, s_tracker_meter_arc, LV_ALIGN_CENTER, 0, -8);

    s_tracker_lbl_proximity = lv_label_create(s_tracker_view);
    lv_label_set_text(s_tracker_lbl_proximity, "Searching...");
    lv_obj_set_style_text_color(s_tracker_lbl_proximity, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_tracker_lbl_proximity, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align_to(s_tracker_lbl_proximity, s_tracker_meter_arc, LV_ALIGN_CENTER, 0, 16);

    // 3. Right Column: Trend & Rolling Real-time Chart
    s_tracker_lbl_trend = lv_label_create(s_tracker_view);
    lv_label_set_text(s_tracker_lbl_trend, LV_SYMBOL_MINUS " SEARCHING");
    lv_obj_set_style_text_color(s_tracker_lbl_trend, TO_LV_COLOR(p.yellow), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_tracker_lbl_trend, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(s_tracker_lbl_trend, LV_ALIGN_TOP_LEFT, 150, 42);

    s_tracker_chart = lv_chart_create(s_tracker_view);
    lv_obj_set_size(s_tracker_chart, 154, 105);
    lv_obj_align(s_tracker_chart, LV_ALIGN_TOP_LEFT, 150, 66);
    lv_chart_set_type(s_tracker_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(s_tracker_chart, LV_CHART_AXIS_PRIMARY_Y, -100, -30);
    lv_chart_set_point_count(s_tracker_chart, WifiAnalyzer::MAX_TRACKER_HISTORY);
    lv_chart_set_div_line_count(s_tracker_chart, 3, 0);

    lv_obj_set_style_bg_color(s_tracker_chart, TO_LV_COLOR(p.mantle), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_tracker_chart, TO_LV_COLOR(p.surface1), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_tracker_chart, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_tracker_chart, 4, LV_PART_MAIN);

    s_tracker_ser = lv_chart_add_series(s_tracker_chart, TO_LV_COLOR(p.mauve), LV_CHART_AXIS_PRIMARY_Y);
    for (size_t i = 0; i < WifiAnalyzer::MAX_TRACKER_HISTORY; i++) {
        lv_chart_set_next_value(s_tracker_chart, s_tracker_ser, -100);
    }
    lv_chart_refresh(s_tracker_chart);

    // 4. Bottom Controls: Audio Beep Toggle & Stats
    s_tracker_btn_audio = lv_btn_create(s_tracker_view);
    lv_obj_set_size(s_tracker_btn_audio, 110, 28);
    lv_obj_align(s_tracker_btn_audio, LV_ALIGN_BOTTOM_LEFT, 4, -4);
    lv_obj_set_style_bg_color(s_tracker_btn_audio, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
    lv_obj_set_style_radius(s_tracker_btn_audio, 4, LV_PART_MAIN);
    lv_obj_add_event_cb(s_tracker_btn_audio, on_tracker_audio_toggle_clicked, LV_EVENT_CLICKED, nullptr);

    s_tracker_lbl_audio = lv_label_create(s_tracker_btn_audio);
    lv_label_set_text(s_tracker_lbl_audio, wifiAnalyzer.isChirpEnabled() ? LV_SYMBOL_VOLUME_MAX " Beep: ON" : LV_SYMBOL_MUTE " Beep: OFF");
    lv_obj_set_style_text_color(s_tracker_lbl_audio, wifiAnalyzer.isChirpEnabled() ? TO_LV_COLOR(p.green) : TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_tracker_lbl_audio, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_center(s_tracker_lbl_audio);

    s_tracker_lbl_stats = lv_label_create(s_tracker_view);
    lv_label_set_text(s_tracker_lbl_stats, "Rapid Scan: Active (~6 Hz)");
    lv_obj_set_style_text_color(s_tracker_lbl_stats, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_tracker_lbl_stats, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(s_tracker_lbl_stats, LV_ALIGN_BOTTOM_LEFT, 122, -10);
}

static void on_close_modal_clicked(lv_event_t* e) {
    if (s_detail_modal) {
        lv_obj_del(s_detail_modal);
        s_detail_modal = nullptr;
        s_modal_bssid = "";
        s_modal_ssid = "";
        s_modal_channel = 1;
        s_modal_lbl_signal = nullptr;
        s_modal_bar_signal = nullptr;
        s_modal_chart_history = nullptr;
        s_modal_ser_history = nullptr;
    }
}

static void on_track_ap_clicked(lv_event_t* e) {
    String bssid = s_modal_bssid;
    String ssid = s_modal_ssid;
    uint8_t channel = s_modal_channel;
    on_close_modal_clicked(nullptr);
    openTrackerView(bssid, ssid, channel);
}

static void showNetworkDetailModal(const AccessPointInfo& ap) {
    if (s_detail_modal) {
        lv_obj_del(s_detail_modal);
        s_detail_modal = nullptr;
    }

    const auto& p = getCatppuccinFlavor(getCurrentThemeFlavor());
    s_modal_bssid = ap.bssidStr;
    s_modal_ssid = ap.ssid;
    s_modal_channel = ap.channel;

    // Create Modal Frame
    s_detail_modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(s_detail_modal, 296, 216);
    lv_obj_center(s_detail_modal);
    lv_obj_clear_flag(s_detail_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(s_detail_modal, TO_LV_COLOR(p.mantle), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_detail_modal, TO_LV_COLOR(p.surface1), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_detail_modal, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(s_detail_modal, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_detail_modal, 24, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(s_detail_modal, TO_LV_COLOR(p.crust), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(s_detail_modal, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_detail_modal, 8, LV_PART_MAIN);

    // 1. Header: SSID & Close Button
    lv_obj_t* lbl_ssid = lv_label_create(s_detail_modal);
    lv_label_set_text(lbl_ssid, ap.ssid.c_str());
    lv_obj_set_style_text_color(lbl_ssid, TO_LV_COLOR(p.mauve), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_ssid, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_size(lbl_ssid, 230, 20);
    lv_label_set_long_mode(lbl_ssid, LV_LABEL_LONG_DOT);
    lv_obj_align(lbl_ssid, LV_ALIGN_TOP_LEFT, 2, 2);

    lv_obj_t* btn_close = lv_btn_create(s_detail_modal);
    lv_obj_set_size(btn_close, 26, 24);
    lv_obj_align(btn_close, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(btn_close, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_close, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn_close, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_close, on_close_modal_clicked, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* lbl_close_sym = lv_label_create(btn_close);
    lv_label_set_text(lbl_close_sym, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(lbl_close_sym, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_center(lbl_close_sym);

    // 2. Info Row 1: BSSID & Frequency
    char infoRow1[64];
    snprintf(infoRow1, sizeof(infoRow1), "BSSID: %s  |  %d MHz (Ch %d)",
             ap.bssidStr.c_str(),
             channelToFrequency(ap.channel),
             ap.channel);
    lv_obj_t* lbl_row1 = lv_label_create(s_detail_modal);
    lv_label_set_text(lbl_row1, infoRow1);
    lv_obj_set_style_text_color(lbl_row1, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_row1, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(lbl_row1, LV_ALIGN_TOP_LEFT, 2, 24);

    // 3. Info Row 2: Security Type
    char infoRow2[64];
    snprintf(infoRow2, sizeof(infoRow2), "Security: %s", authModeToString(ap.authMode));
    lv_obj_t* lbl_row2 = lv_label_create(s_detail_modal);
    lv_label_set_text(lbl_row2, infoRow2);
    lv_obj_set_style_text_color(lbl_row2, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_row2, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(lbl_row2, LV_ALIGN_TOP_LEFT, 2, 38);

    // 4. Signal Strength & Quality Bar
    char signalBuf[48];
    uint8_t quality = rssiToQuality(ap.rssi);
    snprintf(signalBuf, sizeof(signalBuf), "Signal: %d dBm (%d%% Quality)", (int)ap.rssi, quality);

    s_modal_lbl_signal = lv_label_create(s_detail_modal);
    lv_label_set_text(s_modal_lbl_signal, signalBuf);
    lv_obj_set_style_text_color(s_modal_lbl_signal, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_set_style_text_font(s_modal_lbl_signal, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(s_modal_lbl_signal, LV_ALIGN_TOP_LEFT, 2, 52);

    s_modal_bar_signal = lv_bar_create(s_detail_modal);
    lv_obj_set_size(s_modal_bar_signal, 276, 8);
    lv_obj_align(s_modal_bar_signal, LV_ALIGN_TOP_LEFT, 2, 66);
    lv_bar_set_range(s_modal_bar_signal, 0, 100);
    lv_bar_set_value(s_modal_bar_signal, quality, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_modal_bar_signal, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_modal_bar_signal, getChannelLvColor(ap.channel, getCurrentThemeFlavor()), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_modal_bar_signal, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(s_modal_bar_signal, 4, LV_PART_INDICATOR);

    // 5. Signal Stability Sparkline (Rolling History)
    lv_obj_t* lbl_hist = lv_label_create(s_detail_modal);
    lv_label_set_text(lbl_hist, "Signal Stability (Last 30 Scans):");
    lv_obj_set_style_text_color(lbl_hist, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_hist, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_align(lbl_hist, LV_ALIGN_TOP_LEFT, 2, 78);

    s_modal_chart_history = lv_chart_create(s_detail_modal);
    lv_obj_set_size(s_modal_chart_history, 276, 56);
    lv_obj_align(s_modal_chart_history, LV_ALIGN_TOP_LEFT, 2, 94);
    lv_chart_set_type(s_modal_chart_history, LV_CHART_TYPE_LINE);
    lv_chart_set_range(s_modal_chart_history, LV_CHART_AXIS_PRIMARY_Y, -100, -30);
    lv_chart_set_point_count(s_modal_chart_history, WifiAnalyzer::MAX_HISTORY_SAMPLES);
    lv_chart_set_div_line_count(s_modal_chart_history, 3, 0);

    lv_obj_set_style_bg_color(s_modal_chart_history, TO_LV_COLOR(p.base), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_modal_chart_history, TO_LV_COLOR(p.surface1), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_modal_chart_history, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_modal_chart_history, 4, LV_PART_MAIN);

    s_modal_ser_history = lv_chart_add_series(s_modal_chart_history, getChannelLvColor(ap.channel, getCurrentThemeFlavor()), LV_CHART_AXIS_PRIMARY_Y);

    const auto& history = wifiAnalyzer.getRssiHistory(ap.bssidStr);
    for (size_t h = 0; h < WifiAnalyzer::MAX_HISTORY_SAMPLES; h++) {
        if (h < history.size()) {
            lv_chart_set_next_value(s_modal_chart_history, s_modal_ser_history, (lv_coord_t)history[h]);
        } else {
            lv_chart_set_next_value(s_modal_chart_history, s_modal_ser_history, -100);
        }
    }
    lv_chart_refresh(s_modal_chart_history);

    // 6. Action Buttons: Track Signal & Close
    lv_obj_t* btn_track = lv_btn_create(s_detail_modal);
    lv_obj_set_size(btn_track, 134, 28);
    lv_obj_align(btn_track, LV_ALIGN_BOTTOM_LEFT, 2, -2);
    lv_obj_set_style_bg_color(btn_track, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_track, 4, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_track, on_track_ap_clicked, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* lbl_track = lv_label_create(btn_track);
    lv_label_set_text(lbl_track, LV_SYMBOL_EYE_OPEN " Track Signal");
    lv_obj_set_style_text_color(lbl_track, TO_LV_COLOR(p.mauve), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_track, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_center(lbl_track);

    lv_obj_t* btn_bottom_close = lv_btn_create(s_detail_modal);
    lv_obj_set_size(btn_bottom_close, 134, 28);
    lv_obj_align(btn_bottom_close, LV_ALIGN_BOTTOM_RIGHT, -2, -2);
    lv_obj_set_style_bg_color(btn_bottom_close, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_bottom_close, 4, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_bottom_close, on_close_modal_clicked, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* lbl_bottom_close = lv_label_create(btn_bottom_close);
    lv_label_set_text(lbl_bottom_close, LV_SYMBOL_CLOSE " Close");
    lv_obj_set_style_text_color(lbl_bottom_close, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl_bottom_close, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_center(lbl_bottom_close);
}

static void on_network_clicked(lv_event_t* e) {
    uintptr_t idx = (uintptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    const auto& aps = wifiAnalyzer.getAccessPoints();
    if (idx < aps.size()) {
        showNetworkDetailModal(aps[idx]);
    }
}

static void on_spectrum_draw(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_DRAW_POST) return;

    lv_obj_t* obj = lv_event_get_target(e);
    lv_draw_ctx_t* draw_ctx = lv_event_get_draw_ctx(e);
    if (!draw_ctx) return;

    const auto& p = getCatppuccinFlavor(getCurrentThemeFlavor());

    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    lv_coord_t pad_left = 28;
    lv_coord_t pad_right = 10;
    lv_coord_t pad_top = 18;
    lv_coord_t pad_bottom = 22;

    lv_coord_t left = coords.x1 + pad_left;
    lv_coord_t right = coords.x2 - pad_right;
    lv_coord_t top = coords.y1 + pad_top;
    lv_coord_t bottom = coords.y2 - pad_bottom;
    lv_coord_t width = right - left;
    lv_coord_t height = bottom - top;

    if (width <= 0 || height <= 0) return;

    // 1. Draw dBm horizontal grid lines & labels (-90, -70, -50 dBm)
    lv_draw_line_dsc_t grid_dsc;
    lv_draw_line_dsc_init(&grid_dsc);
    grid_dsc.color = TO_LV_COLOR(p.surface0);
    grid_dsc.width = 1;
    grid_dsc.dash_width = 3;
    grid_dsc.dash_gap = 3;

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = TO_LV_COLOR(p.subtext0);
    label_dsc.font = &lv_font_montserrat_10;

    int dbm_ticks[] = {-90, -70, -50};
    for (int t = 0; t < 3; t++) {
        int dbm = dbm_ticks[t];
        float norm = (float)(dbm - RSSI_FLOOR) / (float)(RSSI_CEILING - RSSI_FLOOR);
        lv_coord_t y = bottom - (lv_coord_t)(norm * height);

        lv_point_t p1 = {left, y};
        lv_point_t p2 = {right, y};
        lv_draw_line(draw_ctx, &grid_dsc, &p1, &p2);

        char dbm_str[8];
        snprintf(dbm_str, sizeof(dbm_str), "%d", dbm);
        lv_area_t txt_area = {(lv_coord_t)(coords.x1 + 2), (lv_coord_t)(y - 5), (lv_coord_t)(left - 2), (lv_coord_t)(y + 10)};
        lv_draw_label(draw_ctx, &label_dsc, &txt_area, dbm_str, NULL);
    }

    // 2. Draw Baseline Axis
    lv_draw_line_dsc_t axis_dsc;
    lv_draw_line_dsc_init(&axis_dsc);
    axis_dsc.color = TO_LV_COLOR(p.surface1);
    axis_dsc.width = 2;
    lv_point_t base1 = {left, bottom};
    lv_point_t base2 = {right, bottom};
    lv_draw_line(draw_ctx, &axis_dsc, &base1, &base2);

    // 3. Channels 1 to 14 ticks and numbers
    float chan_step = (float)width / 15.0f;
    for (int c = 1; c <= 14; c++) {
        lv_coord_t cx = left + (lv_coord_t)(c * chan_step);

        lv_point_t t1 = {cx, bottom};
        lv_point_t t2 = {cx, (lv_coord_t)(bottom + 3)};
        lv_draw_line(draw_ctx, &axis_dsc, &t1, &t2);

        char c_str[8];
        snprintf(c_str, sizeof(c_str), "%d", c);

        bool is_std_channel = (c == 1 || c == 6 || c == 11);
        label_dsc.color = is_std_channel ? TO_LV_COLOR(p.mauve) : TO_LV_COLOR(p.subtext0);

        lv_area_t c_area = {(lv_coord_t)(cx - 8), (lv_coord_t)(bottom + 4), (lv_coord_t)(cx + 8), (lv_coord_t)(bottom + 16)};
        lv_draw_label(draw_ctx, &label_dsc, &c_area, c_str, NULL);
    }

    // 4. Parabolic Frequency Response Curves for detected APs
    const auto& aps = wifiAnalyzer.getAccessPoints();
    float spread = chan_step * 2.0f; // +-2 channels span (20-22 MHz standard bandwidth)

    // Pre-calculate which AP has the peak RSSI on each channel for floating labels
    int peak_ap_for_channel[15];
    for (int i = 0; i < 15; i++) peak_ap_for_channel[i] = -1;
    for (size_t i = 0; i < aps.size(); i++) {
        uint8_t ch = aps[i].channel;
        if (ch >= 1 && ch <= 14) {
            if (peak_ap_for_channel[ch] == -1 || aps[i].rssi > aps[peak_ap_for_channel[ch]].rssi) {
                peak_ap_for_channel[ch] = (int)i;
            }
        }
    }

    lv_draw_line_dsc_t curve_dsc;
    lv_draw_line_dsc_init(&curve_dsc);
    curve_dsc.width = 2;

    for (size_t i = 0; i < aps.size(); i++) {
        const auto& ap = aps[i];
        if (ap.channel < 1 || ap.channel > 14) continue;

        lv_coord_t cx = left + (lv_coord_t)(ap.channel * chan_step);

        int32_t clamped_rssi = ap.rssi;
        if (clamped_rssi < RSSI_FLOOR) clamped_rssi = RSSI_FLOOR;
        if (clamped_rssi > RSSI_CEILING) clamped_rssi = RSSI_CEILING;

        float norm = (float)(clamped_rssi - RSSI_FLOOR) / (float)(RSSI_CEILING - RSSI_FLOOR);
        if (norm < 0.05f) norm = 0.05f;

        float peak_h = norm * (height - 8);
        curve_dsc.color = getChannelLvColor(ap.channel, getCurrentThemeFlavor());

        // Draw parabolic curve with 16 connected segments
        const int SEGMENTS = 16;
        lv_point_t prev_pt;
        bool has_prev = false;

        for (int s = 0; s <= SEGMENTS; s++) {
            float frac = (float)s / (float)SEGMENTS;
            float offset = -spread + frac * (2.0f * spread);
            float curr_x = (float)cx + offset;
            float curr_y = calculateParabolaY(curr_x, (float)cx, spread, peak_h, (float)bottom);

            lv_coord_t px = (lv_coord_t)(curr_x + 0.5f);
            lv_coord_t py = (lv_coord_t)(curr_y + 0.5f);

            if (px < left) px = left;
            if (px > right) px = right;
            if (py > bottom) py = bottom;

            lv_point_t curr_pt = {px, py};

            if (has_prev) {
                lv_draw_line(draw_ctx, &curve_dsc, &prev_pt, &curr_pt);
            }
            prev_pt = curr_pt;
            has_prev = true;
        }

        // Draw SSID & dBm tag at apex for top AP on this channel
        if (peak_ap_for_channel[ap.channel] == (int)i) {
            lv_coord_t apex_y = bottom - (lv_coord_t)peak_h;
            char tag[48];
            if (ap.isHidden || ap.ssid.startsWith("[Hidden")) {
                snprintf(tag, sizeof(tag), "[Hidden] %ddB", (int)ap.rssi);
            } else {
                snprintf(tag, sizeof(tag), "%.8s %ddB", ap.ssid.c_str(), (int)ap.rssi);
            }

            label_dsc.color = curve_dsc.color;
            label_dsc.font = &lv_font_montserrat_10;
            label_dsc.align = LV_TEXT_ALIGN_CENTER;
            label_dsc.flag = LV_TEXT_FLAG_NONE;

            lv_area_t tag_area = {
                (lv_coord_t)(cx - 42),
                (lv_coord_t)(apex_y - 13),
                (lv_coord_t)(cx + 42),
                (lv_coord_t)(apex_y)
            };
            if (tag_area.x1 < left + 2) {
                tag_area.x2 += (left + 2 - tag_area.x1);
                tag_area.x1 = left + 2;
            }
            if (tag_area.x2 > right - 2) {
                tag_area.x1 -= (tag_area.x2 - (right - 2));
                tag_area.x2 = right - 2;
            }
            if (tag_area.y1 < top - 8) {
                tag_area.y1 = top - 8;
                tag_area.y2 = top + 6;
            }

            lv_draw_label(draw_ctx, &label_dsc, &tag_area, tag, NULL);
        }
    }
}

void initUI() {
    const auto& p = getCatppuccinFlavor(getCurrentThemeFlavor());

    lv_obj_t* scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, TO_LV_COLOR(p.base), LV_PART_MAIN);

    // Create Tabview
    s_tabview = lv_tabview_create(scr, LV_DIR_TOP, 34);
    lv_obj_set_style_bg_color(s_tabview, TO_LV_COLOR(p.base), LV_PART_MAIN);
    
    lv_obj_t* tab_btns = lv_tabview_get_tab_btns(s_tabview);
    lv_obj_set_style_bg_color(tab_btns, TO_LV_COLOR(p.mantle), LV_PART_MAIN);
    lv_obj_set_style_text_color(tab_btns, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_set_style_bg_color(tab_btns, TO_LV_COLOR(p.surface0), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(tab_btns, TO_LV_COLOR(p.mauve), LV_PART_ITEMS | LV_STATE_CHECKED);

    s_tab_spectrum = lv_tabview_add_tab(s_tabview, "Spectrum");
    s_tab_networks = lv_tabview_add_tab(s_tabview, "Networks");
    s_tab_channels = lv_tabview_add_tab(s_tabview, "Channels");
    s_tab_settings = lv_tabview_add_tab(s_tabview, "Settings");

    // ----------------------------------------------------
    // Tab 1: Spectrum
    // ----------------------------------------------------
    s_lbl_spectrum_summary = lv_label_create(s_tab_spectrum);
    lv_label_set_text(s_lbl_spectrum_summary, "Scanning Wi-Fi channels...");
    lv_obj_set_style_text_color(s_lbl_spectrum_summary, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_spectrum_summary, LV_ALIGN_TOP_LEFT, 5, 2);

    s_spectrum_view = lv_obj_create(s_tab_spectrum);
    lv_obj_set_size(s_spectrum_view, LV_PCT(100), 150);
    lv_obj_align(s_spectrum_view, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(s_spectrum_view, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_bg_color(s_spectrum_view, TO_LV_COLOR(p.mantle), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_spectrum_view, TO_LV_COLOR(p.surface1), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_spectrum_view, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_spectrum_view, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_spectrum_view, 0, LV_PART_MAIN);

    lv_obj_add_event_cb(s_spectrum_view, on_spectrum_draw, LV_EVENT_DRAW_POST, nullptr);

    // ----------------------------------------------------
    // Tab 2: Networks List
    // ----------------------------------------------------
    s_list_networks = lv_list_create(s_tab_networks);
    lv_obj_set_size(s_list_networks, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_list_networks, TO_LV_COLOR(p.base), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_list_networks, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(s_list_networks, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_right(s_list_networks, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_top(s_list_networks, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(s_list_networks, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_row(s_list_networks, 3, LV_PART_MAIN);

    // ----------------------------------------------------
    // Tab 3: Channels Stats Table
    // ----------------------------------------------------
    s_table_channels = lv_table_create(s_tab_channels);
    lv_obj_set_size(s_table_channels, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_table_channels, TO_LV_COLOR(p.base), LV_PART_MAIN);
    lv_table_set_col_cnt(s_table_channels, 4);
    lv_table_set_col_width(s_table_channels, 0, 36);
    lv_table_set_col_width(s_table_channels, 1, 46);
    lv_table_set_col_width(s_table_channels, 2, 80);
    lv_table_set_col_width(s_table_channels, 3, 108);

    lv_obj_set_style_pad_left(s_table_channels, 4, LV_PART_ITEMS);
    lv_obj_set_style_pad_right(s_table_channels, 4, LV_PART_ITEMS);
    lv_obj_set_style_pad_top(s_table_channels, 6, LV_PART_ITEMS);
    lv_obj_set_style_pad_bottom(s_table_channels, 6, LV_PART_ITEMS);

    lv_table_set_cell_value(s_table_channels, 0, 0, "Ch");
    lv_table_set_cell_value(s_table_channels, 0, 1, "APs");
    lv_table_set_cell_value(s_table_channels, 0, 2, "Peak dBm");
    lv_table_set_cell_value(s_table_channels, 0, 3, "Rating");

    for (int c = 1; c <= 14; c++) {
        char chBuf[8];
        snprintf(chBuf, sizeof(chBuf), "%d", c);
        lv_table_set_cell_value(s_table_channels, c, 0, chBuf);
        lv_table_set_cell_value(s_table_channels, c, 1, "0");
        lv_table_set_cell_value(s_table_channels, c, 2, "--");
        lv_table_set_cell_value(s_table_channels, c, 3, "Clean");
    }

    // ----------------------------------------------------
    // Tab 4: Settings
    // ----------------------------------------------------
    s_lbl_theme = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_theme, "Theme Flavor:");
    lv_obj_set_style_text_color(s_lbl_theme, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_theme, LV_ALIGN_TOP_LEFT, 10, 12);

    s_dropdown_theme = lv_dropdown_create(s_tab_settings);
    lv_dropdown_set_options(s_dropdown_theme, "Mocha\nMacchiato\nFrappe\nLatte");
    lv_dropdown_set_selected(s_dropdown_theme, settings.getThemeFlavor() - 1);
    lv_obj_set_size(s_dropdown_theme, 142, LV_SIZE_CONTENT);
    lv_obj_align(s_dropdown_theme, LV_ALIGN_TOP_LEFT, 132, 6);
    lv_obj_add_event_cb(s_dropdown_theme, on_theme_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    s_lbl_bright = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_bright, "Brightness:");
    lv_obj_set_style_text_color(s_lbl_bright, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_bright, LV_ALIGN_TOP_LEFT, 10, 52);

    s_slider_brightness = lv_slider_create(s_tab_settings);
    lv_slider_set_range(s_slider_brightness, 10, 100);
    lv_slider_set_value(s_slider_brightness, settings.getBrightness(), LV_ANIM_OFF);
    lv_obj_set_size(s_slider_brightness, 142, 12);
    lv_obj_align(s_slider_brightness, LV_ALIGN_TOP_LEFT, 132, 54);
    lv_obj_add_event_cb(s_slider_brightness, on_brightness_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    s_lbl_auto_bright = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_auto_bright, "Auto Bright:");
    lv_obj_set_style_text_color(s_lbl_auto_bright, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_auto_bright, LV_ALIGN_TOP_LEFT, 10, 84);

    s_switch_auto_bright = lv_switch_create(s_tab_settings);
    if (settings.getAutoBrightness()) {
        lv_obj_add_state(s_switch_auto_bright, LV_STATE_CHECKED);
    }
    lv_obj_align(s_switch_auto_bright, LV_ALIGN_TOP_LEFT, 132, 80);
    lv_obj_add_event_cb(s_switch_auto_bright, on_auto_bright_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    s_lbl_interval = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_interval, "Scan Interval:");
    lv_obj_set_style_text_color(s_lbl_interval, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_interval, LV_ALIGN_TOP_LEFT, 10, 126);

    s_dropdown_interval = lv_dropdown_create(s_tab_settings);
    lv_dropdown_set_options(s_dropdown_interval, "1s\n2s\n3s\n5s");
    lv_dropdown_set_selected(s_dropdown_interval, scanIntervalToDropdownIdx(settings.getScanIntervalMs()));
    lv_obj_set_size(s_dropdown_interval, 142, LV_SIZE_CONTENT);
    lv_obj_align(s_dropdown_interval, LV_ALIGN_TOP_LEFT, 132, 120);
    lv_obj_add_event_cb(s_dropdown_interval, on_interval_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    s_lbl_passive = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_passive, "Passive Scan:");
    lv_obj_set_style_text_color(s_lbl_passive, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_passive, LV_ALIGN_TOP_LEFT, 10, 168);

    s_switch_passive = lv_switch_create(s_tab_settings);
    if (settings.getPassiveScan()) {
        lv_obj_add_state(s_switch_passive, LV_STATE_CHECKED);
    }
    lv_obj_align(s_switch_passive, LV_ALIGN_TOP_LEFT, 132, 164);
    lv_obj_add_event_cb(s_switch_passive, on_passive_scan_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    s_lbl_rotation = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_rotation, "Orientation:");
    lv_obj_set_style_text_color(s_lbl_rotation, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_rotation, LV_ALIGN_TOP_LEFT, 10, 210);

    s_dropdown_rotation = lv_dropdown_create(s_tab_settings);
    lv_dropdown_set_options(s_dropdown_rotation, "Landscape (0\xC2\xB0)\nLandscape (180\xC2\xB0)");
    lv_dropdown_set_selected(s_dropdown_rotation, orientationToDropdownIdx(settings.getScreenOrientation()));
    lv_obj_set_size(s_dropdown_rotation, 142, LV_SIZE_CONTENT);
    lv_obj_align(s_dropdown_rotation, LV_ALIGN_TOP_LEFT, 132, 204);
    lv_obj_add_event_cb(s_dropdown_rotation, on_rotation_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    s_lbl_led = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_led, "Status LED:");
    lv_obj_set_style_text_color(s_lbl_led, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_led, LV_ALIGN_TOP_LEFT, 10, 250);

    s_switch_led = lv_switch_create(s_tab_settings);
    if (settings.getLedEnabled()) {
        lv_obj_add_state(s_switch_led, LV_STATE_CHECKED);
    }
    lv_obj_align(s_switch_led, LV_ALIGN_TOP_LEFT, 132, 246);
    lv_obj_add_event_cb(s_switch_led, on_led_toggle_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    s_lbl_led_bright = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_led_bright, "LED Bright:");
    lv_obj_set_style_text_color(s_lbl_led_bright, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_align(s_lbl_led_bright, LV_ALIGN_TOP_LEFT, 10, 290);

    s_slider_led_bright = lv_slider_create(s_tab_settings);
    lv_slider_set_range(s_slider_led_bright, 10, 255);
    lv_slider_set_value(s_slider_led_bright, settings.getLedBrightness(), LV_ANIM_OFF);
    lv_obj_set_size(s_slider_led_bright, 142, 12);
    lv_obj_align(s_slider_led_bright, LV_ALIGN_TOP_LEFT, 132, 292);
    lv_obj_add_event_cb(s_slider_led_bright, on_led_brightness_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    s_btn_reset = lv_btn_create(s_tab_settings);
    lv_obj_set_size(s_btn_reset, 265, 34);
    lv_obj_align(s_btn_reset, LV_ALIGN_TOP_LEFT, 10, 330);
    lv_obj_set_style_bg_color(s_btn_reset, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
    lv_obj_add_event_cb(s_btn_reset, on_reset_settings_clicked, LV_EVENT_CLICKED, nullptr);

    lv_obj_t* lbl_reset_text = lv_label_create(s_btn_reset);
    lv_label_set_text(lbl_reset_text, "Reset Settings to Default");
    lv_obj_set_style_text_color(lbl_reset_text, TO_LV_COLOR(p.red), LV_PART_MAIN);
    lv_obj_center(lbl_reset_text);

    s_lbl_info = lv_label_create(s_tab_settings);
    lv_label_set_text(s_lbl_info, "ESP32 CYD Wi-Fi Analyzer\nEngine: LVGL v8 + Catppuccin RGB565");
    lv_obj_set_style_text_color(s_lbl_info, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_align(s_lbl_info, LV_ALIGN_TOP_LEFT, 10, 376);
}

void updateUI() {
    if (wifiAnalyzer.isTracking()) {
        if (!s_tracker_view) return;
        const auto& p = getCatppuccinFlavor(getCurrentThemeFlavor());
        int32_t rssi = wifiAnalyzer.getTargetRssi();
        uint8_t q = rssiToQuality(rssi);

        if (s_tracker_meter_arc) {
            lv_arc_set_value(s_tracker_meter_arc, q);
            lv_color_t arc_color;
            if (q >= 70) arc_color = TO_LV_COLOR(p.green);
            else if (q >= 45) arc_color = TO_LV_COLOR(p.sapphire);
            else if (q >= 20) arc_color = TO_LV_COLOR(p.yellow);
            else arc_color = TO_LV_COLOR(p.red);
            lv_obj_set_style_arc_color(s_tracker_meter_arc, arc_color, LV_PART_INDICATOR);
        }

        if (s_tracker_lbl_rssi) {
            char rBuf[32];
            if (rssi <= RSSI_FLOOR) {
                snprintf(rBuf, sizeof(rBuf), "-- dBm");
            } else {
                snprintf(rBuf, sizeof(rBuf), "%d dBm", (int)rssi);
            }
            lv_label_set_text(s_tracker_lbl_rssi, rBuf);
            lv_obj_align_to(s_tracker_lbl_rssi, s_tracker_meter_arc, LV_ALIGN_CENTER, 0, -8);
        }

        if (s_tracker_lbl_proximity) {
            lv_label_set_text(s_tracker_lbl_proximity, wifiAnalyzer.getTargetProximityStr());
            lv_obj_align_to(s_tracker_lbl_proximity, s_tracker_meter_arc, LV_ALIGN_CENTER, 0, 16);
        }

        if (s_tracker_lbl_trend) {
            int trend = wifiAnalyzer.getTargetTrend();
            if (trend > 0) {
                lv_label_set_text(s_tracker_lbl_trend, LV_SYMBOL_UP " WARMER");
                lv_obj_set_style_text_color(s_tracker_lbl_trend, TO_LV_COLOR(p.green), LV_PART_MAIN);
            } else if (trend < 0) {
                lv_label_set_text(s_tracker_lbl_trend, LV_SYMBOL_DOWN " COLDER");
                lv_obj_set_style_text_color(s_tracker_lbl_trend, TO_LV_COLOR(p.red), LV_PART_MAIN);
            } else {
                lv_label_set_text(s_tracker_lbl_trend, LV_SYMBOL_MINUS " STABLE");
                lv_obj_set_style_text_color(s_tracker_lbl_trend, TO_LV_COLOR(p.yellow), LV_PART_MAIN);
            }
        }

        if (s_tracker_chart && s_tracker_ser) {
            const auto& hist = wifiAnalyzer.getTargetRssiHistory();
            for (size_t h = 0; h < WifiAnalyzer::MAX_TRACKER_HISTORY; h++) {
                if (h < hist.size()) {
                    s_tracker_ser->y_points[h] = (lv_coord_t)hist[h];
                } else {
                    s_tracker_ser->y_points[h] = -100;
                }
            }
            lv_chart_refresh(s_tracker_chart);
        }

        if (s_tracker_lbl_stats) {
            char statBuf[48];
            snprintf(statBuf, sizeof(statBuf), "Scans: %u | Qual: %d%%", (unsigned)wifiAnalyzer.getScanCount(), q);
            lv_label_set_text(s_tracker_lbl_stats, statBuf);
        }

        playTrackerChirp(rssi);
        return;
    }

    const auto& aps = wifiAnalyzer.getAccessPoints();
    const ChannelInfo* channels = wifiAnalyzer.getChannels();
    const auto& bestChannels = wifiAnalyzer.getBestChannels();

    // 1. Update Spectrum Summary
    char summaryBuf[128];
    if (bestChannels.empty()) {
        snprintf(summaryBuf, sizeof(summaryBuf), "Networks: %d | Scanning...", (int)aps.size());
    } else {
        char chList[32] = "";
        for (size_t i = 0; i < bestChannels.size() && i < 3; i++) {
            char num[6];
            snprintf(num, sizeof(num), "%s%d", (i > 0 ? ", " : ""), bestChannels[i]);
            strncat(chList, num, sizeof(chList) - strlen(chList) - 1);
        }
        snprintf(summaryBuf, sizeof(summaryBuf), "APs: %d | Best Channels: %s", (int)aps.size(), chList);
    }
    lv_label_set_text(s_lbl_spectrum_summary, summaryBuf);

    // 2. Invalidate Spectrum View to trigger parabolic redraw
    if (s_spectrum_view) {
        lv_obj_invalidate(s_spectrum_view);
    }

    // 3. Update Networks List
    lv_obj_clean(s_list_networks);
    const auto& p = getCatppuccinFlavor(getCurrentThemeFlavor());

    for (size_t i = 0; i < aps.size(); i++) {
        const auto& ap = aps[i];

        const char* rawAuth = authModeToString(ap.authMode);
        char shortAuth[10];
        if (strstr(rawAuth, "WPA3")) strcpy(shortAuth, "WPA3");
        else if (strstr(rawAuth, "WPA2")) strcpy(shortAuth, "WPA2");
        else if (strstr(rawAuth, "WPA")) strcpy(shortAuth, "WPA");
        else if (strstr(rawAuth, "WEP")) strcpy(shortAuth, "WEP");
        else strcpy(shortAuth, "Open");

        String displaySsid = ap.ssid;
        if (ap.isHidden || displaySsid.startsWith("[Hidden")) {
            displaySsid = "[Hidden]";
        }

        char btnText[64];
        snprintf(btnText, sizeof(btnText), "Ch%-2d  %-9.9s  %ddBm  %s",
                 ap.channel,
                 displaySsid.c_str(),
                 (int)ap.rssi,
                 shortAuth);

        lv_obj_t* btn = lv_list_add_btn(s_list_networks, LV_SYMBOL_WIFI, btnText);
        lv_obj_set_style_pad_left(btn, 6, LV_PART_MAIN);
        lv_obj_set_style_pad_right(btn, 6, LV_PART_MAIN);
        lv_obj_set_style_pad_top(btn, 5, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(btn, 5, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
        lv_obj_set_style_text_color(btn, TO_LV_COLOR(getChannelColor(ap.channel, getCurrentThemeFlavor())), LV_PART_MAIN);
        lv_obj_set_user_data(btn, (void*)(uintptr_t)i);
        lv_obj_add_event_cb(btn, on_network_clicked, LV_EVENT_CLICKED, nullptr);
    }

    // 4. Update Detail Modal (if currently open for an active AP)
    if (s_detail_modal && s_modal_bssid.length() > 0) {
        for (const auto& ap : aps) {
            if (ap.bssidStr == s_modal_bssid) {
                if (s_modal_lbl_signal) {
                    char sigBuf[48];
                    uint8_t q = rssiToQuality(ap.rssi);
                    snprintf(sigBuf, sizeof(sigBuf), "Signal: %d dBm (%d%% Quality)", (int)ap.rssi, q);
                    lv_label_set_text(s_modal_lbl_signal, sigBuf);
                }
                if (s_modal_bar_signal) {
                    lv_bar_set_value(s_modal_bar_signal, rssiToQuality(ap.rssi), LV_ANIM_ON);
                }
                if (s_modal_chart_history && s_modal_ser_history) {
                    const auto& hist = wifiAnalyzer.getRssiHistory(s_modal_bssid);
                    for (size_t h = 0; h < WifiAnalyzer::MAX_HISTORY_SAMPLES; h++) {
                        if (h < hist.size()) {
                            s_modal_ser_history->y_points[h] = (lv_coord_t)hist[h];
                        } else {
                            s_modal_ser_history->y_points[h] = -100;
                        }
                    }
                    lv_chart_refresh(s_modal_chart_history);
                }
                break;
            }
        }
    }

    // 5. Update Channels Table
    for (int c = 0; c < 14; c++) {
        char apBuf[16];
        snprintf(apBuf, sizeof(apBuf), "%d", channels[c].apCount);
        lv_table_set_cell_value(s_table_channels, c + 1, 1, apBuf);

        char rssiBuf[16];
        if (channels[c].peakRssi <= RSSI_FLOOR) {
            snprintf(rssiBuf, sizeof(rssiBuf), "--");
        } else {
            snprintf(rssiBuf, sizeof(rssiBuf), "%d dBm", (int)channels[c].peakRssi);
        }
        lv_table_set_cell_value(s_table_channels, c + 1, 2, rssiBuf);

        const char* rating = "Good";
        if (channels[c].noiseScore == 0) rating = "Clean";
        else if (channels[c].apCount > 5) rating = "Congested";
        else if (channels[c].apCount > 2) rating = "Fair";
        lv_table_set_cell_value(s_table_channels, c + 1, 3, rating);
    }
}

void refreshThemeUI() {
    const auto& p = getCatppuccinFlavor(getCurrentThemeFlavor());

    lv_obj_set_style_bg_color(lv_scr_act(), TO_LV_COLOR(p.base), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_tabview, TO_LV_COLOR(p.base), LV_PART_MAIN);

    lv_obj_t* tab_btns = lv_tabview_get_tab_btns(s_tabview);
    lv_obj_set_style_bg_color(tab_btns, TO_LV_COLOR(p.mantle), LV_PART_MAIN);
    lv_obj_set_style_text_color(tab_btns, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);
    lv_obj_set_style_bg_color(tab_btns, TO_LV_COLOR(p.surface0), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(tab_btns, TO_LV_COLOR(p.mauve), LV_PART_ITEMS | LV_STATE_CHECKED);

    lv_obj_set_style_text_color(s_lbl_spectrum_summary, TO_LV_COLOR(p.text), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_spectrum_view, TO_LV_COLOR(p.mantle), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_spectrum_view, TO_LV_COLOR(p.surface1), LV_PART_MAIN);

    if (s_detail_modal) {
        lv_obj_set_style_bg_color(s_detail_modal, TO_LV_COLOR(p.mantle), LV_PART_MAIN);
        lv_obj_set_style_border_color(s_detail_modal, TO_LV_COLOR(p.surface1), LV_PART_MAIN);
    }

    if (s_tracker_view) {
        lv_obj_set_style_bg_color(s_tracker_view, TO_LV_COLOR(p.base), LV_PART_MAIN);
        if (s_tracker_chart) {
            lv_obj_set_style_bg_color(s_tracker_chart, TO_LV_COLOR(p.mantle), LV_PART_MAIN);
            lv_obj_set_style_border_color(s_tracker_chart, TO_LV_COLOR(p.surface1), LV_PART_MAIN);
        }
        if (s_tracker_meter_arc) {
            lv_obj_set_style_arc_color(s_tracker_meter_arc, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
        }
    }

    if (s_lbl_theme) lv_obj_set_style_text_color(s_lbl_theme, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_bright) lv_obj_set_style_text_color(s_lbl_bright, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_auto_bright) lv_obj_set_style_text_color(s_lbl_auto_bright, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_interval) lv_obj_set_style_text_color(s_lbl_interval, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_passive) lv_obj_set_style_text_color(s_lbl_passive, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_rotation) lv_obj_set_style_text_color(s_lbl_rotation, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_led) lv_obj_set_style_text_color(s_lbl_led, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_lbl_led_bright) lv_obj_set_style_text_color(s_lbl_led_bright, TO_LV_COLOR(p.text), LV_PART_MAIN);
    if (s_btn_reset) lv_obj_set_style_bg_color(s_btn_reset, TO_LV_COLOR(p.surface0), LV_PART_MAIN);
    if (s_lbl_info) lv_obj_set_style_text_color(s_lbl_info, TO_LV_COLOR(p.subtext0), LV_PART_MAIN);

    updateUI();
}

void setTab(uint16_t index) {
    if (s_tabview && index < 4) {
        lv_tabview_set_act(s_tabview, index, LV_ANIM_OFF);
    }
}

uint16_t getTab() {
    if (s_tabview) {
        return lv_tabview_get_tab_act(s_tabview);
    }
    return 0;
}
