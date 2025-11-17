#include "ui_main.h"

#include "calc_heating_cable.h"
#include "calc_heating_pad.h"
#include "calc_lighting.h"
#include "calc_misting.h"
#include "calc_substrate.h"
#include "lvgl.h"
#include "ui_keyboard.h"
#include "ui_screens_cable.h"
#include "ui_screens_home.h"
#include "ui_screens_lighting.h"
#include "ui_screens_misting.h"
#include "ui_screens_pad.h"
#include "ui_screens_about.h"
#include "ui_screens_substrate.h"

static void apply_theme(void)
{
    lv_display_t *disp = lv_display_get_default();
    lv_theme_t *th = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_lighten(LV_PALETTE_GREY, 2),
                                           false, LV_FONT_DEFAULT);
    lv_display_set_theme(disp, th);
}

void ui_main_init(void)
{
    apply_theme();

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0B1220), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_100, LV_PART_MAIN);

    lv_obj_t *root = lv_obj_create(scr);
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(root, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(root, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *title = lv_label_create(root);
    lv_label_set_text(title, "Calculateur Terrarium - Waveshare ESP32-S3 Touch LCD 7B");
    lv_obj_set_style_text_color(title, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, LV_PART_MAIN);

    lv_obj_t *subtitle = lv_label_create(root);
    lv_label_set_text(subtitle, "Dimensionne tapis/câble chauffant, éclairage 6500K-UVA-UVB, substrat et brumisation.");
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0xCBD5E1), LV_PART_MAIN);

    lv_obj_t *tabview = lv_tabview_create(root, LV_DIR_TOP, 48);
    lv_obj_set_size(tabview, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_grow(tabview, 1);

    lv_obj_t *tab_home = lv_tabview_add_tab(tabview, "Accueil");
    lv_obj_t *tab_pad = lv_tabview_add_tab(tabview, "Tapis");
    lv_obj_t *tab_cable = lv_tabview_add_tab(tabview, "Câble");
    lv_obj_t *tab_light = lv_tabview_add_tab(tabview, "Éclairage");
    lv_obj_t *tab_substrate = lv_tabview_add_tab(tabview, "Substrat");
    lv_obj_t *tab_mist = lv_tabview_add_tab(tabview, "Brumisation");
    lv_obj_t *tab_about = lv_tabview_add_tab(tabview, "Sécurité");

    ui_keyboard_create(root);

    ui_screen_home_build(tab_home);
    ui_screen_pad_build(tab_pad);
    ui_screen_cable_build(tab_cable);
    ui_screen_lighting_build(tab_light);
    ui_screen_substrate_build(tab_substrate);
    ui_screen_misting_build(tab_mist);
    ui_screen_about_build(tab_about);
}

