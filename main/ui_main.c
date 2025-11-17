#include "ui_main.h"

#include "calc_heating_cable.h"
#include "calc_heating_pad.h"
#include "calc_lighting.h"
#include "calc_misting.h"
#include "calc_substrate.h"
#include "lvgl.h"
#include "ui_keyboard.h"
#include "ui_screens_about.h"
#include "ui_screens_cable.h"
#include "ui_screens_home.h"
#include "ui_screens_lighting.h"
#include "ui_screens_misting.h"
#include "ui_screens_pad.h"
#include "ui_screens_substrate.h"

#define COLOR_BG lv_color_hex(0x0B1220)
#define COLOR_SURFACE lv_color_hex(0x111827)
#define COLOR_TEXT lv_color_hex(0xE2E8F0)
#define COLOR_MUTED lv_color_hex(0x94A3B8)
#define COLOR_ACCENT lv_color_hex(0x22D3EE)

static void apply_theme(void)
{
    lv_display_t *disp = lv_display_get_default();
    lv_theme_t *th = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_TEAL), lv_palette_main(LV_PALETTE_BLUE_GREY),
                                           true, &lv_font_montserrat_20);
    lv_display_set_theme(disp, th);
}

static void style_tab_bar(lv_obj_t *tabview)
{
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tabview);
    lv_obj_set_style_bg_color(tab_bar, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_pad_all(tab_bar, 6, LV_PART_MAIN);
    lv_obj_set_style_text_color(tab_bar, COLOR_TEXT, LV_PART_ITEMS);
    lv_obj_set_style_text_font(tab_bar, &lv_font_montserrat_20, LV_PART_ITEMS);
    lv_obj_set_style_min_height(tab_bar, 48, LV_PART_ITEMS);
    lv_obj_set_style_pad_hor(tab_bar, 12, LV_PART_ITEMS);
}

static void tabview_changed_cb(lv_event_t *e)
{
    lv_obj_t *tabview = lv_event_get_target(e);
    uint32_t idx = lv_tabview_get_tab_act(tabview);
    switch (idx) {
    case 0: // Accueil
    case 6: // Sécurité
        ui_keyboard_set_default_mode(UI_KEYBOARD_MODE_TEXT);
        break;
    default:
        ui_keyboard_set_default_mode(UI_KEYBOARD_MODE_DECIMAL);
        break;
    }
}

void ui_main_init(void)
{
    apply_theme();

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_100, LV_PART_MAIN);

    lv_obj_t *root = lv_obj_create(scr);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(root, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(root, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(root, 14, LV_PART_MAIN);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *title = lv_label_create(root);
    lv_label_set_text(title, "Calculateur Terrarium - Waveshare ESP32-S3 Touch LCD 7B");
    lv_obj_set_style_text_color(title, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t *subtitle = lv_label_create(root);
    lv_label_set_text(subtitle,
                      "Dimensionne tapis/câble chauffant, éclairage 6500K-UVA-UVB, substrat et brumisation."
                      " Touches agrandies et contraste renforcé pour usage tactile.");
    lv_obj_set_style_text_color(subtitle, COLOR_MUTED, LV_PART_MAIN);
    lv_obj_set_width(subtitle, LV_PCT(100));
    lv_label_set_long_mode(subtitle, LV_LABEL_LONG_WRAP);

    lv_obj_t *tabview = lv_tabview_create(root, LV_DIR_TOP, 52);
    lv_obj_set_size(tabview, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_grow(tabview, 1);
    style_tab_bar(tabview);

    lv_obj_t *tab_home = lv_tabview_add_tab(tabview, "Accueil");
    lv_obj_t *tab_pad = lv_tabview_add_tab(tabview, "Tapis");
    lv_obj_t *tab_cable = lv_tabview_add_tab(tabview, "Câble");
    lv_obj_t *tab_light = lv_tabview_add_tab(tabview, "Éclairage");
    lv_obj_t *tab_substrate = lv_tabview_add_tab(tabview, "Substrat");
    lv_obj_t *tab_mist = lv_tabview_add_tab(tabview, "Brumisation");
    lv_obj_t *tab_about = lv_tabview_add_tab(tabview, "Sécurité");

    lv_obj_add_event_cb(tabview, tabview_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    ui_keyboard_create(root);
    ui_keyboard_set_default_mode(UI_KEYBOARD_MODE_DECIMAL);

    ui_screen_home_build(tab_home);
    ui_screen_pad_build(tab_pad);
    ui_screen_cable_build(tab_cable);
    ui_screen_lighting_build(tab_light);
    ui_screen_substrate_build(tab_substrate);
    ui_screen_misting_build(tab_mist);
    ui_screen_about_build(tab_about);
}

