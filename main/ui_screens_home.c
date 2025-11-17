#include "ui_screens_home.h"

#define COLOR_TEXT lv_color_hex(0xE2E8F0)
#define COLOR_MUTED lv_color_hex(0x94A3B8)
#define COLOR_SURFACE lv_color_hex(0x111827)

static lv_obj_t *create_help(lv_obj_t *parent, const char *title, const char *body)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_width(panel, LV_PCT(100));
    lv_obj_set_style_bg_color(panel, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 12, LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 8, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x22D3EE), LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(panel, 6, LV_PART_MAIN);

    lv_obj_t *hdr = lv_label_create(panel);
    lv_label_set_text(hdr, title);
    lv_obj_set_style_text_color(hdr, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(hdr, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t *body_lbl = lv_label_create(panel);
    lv_obj_set_width(body_lbl, LV_PCT(100));
    lv_label_set_long_mode(body_lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(body_lbl, body);
    lv_obj_set_style_text_color(body_lbl, COLOR_MUTED, LV_PART_MAIN);

    return panel;
}

void ui_screen_home_build(lv_obj_t *parent)
{
    lv_obj_set_style_pad_all(parent, 16, LV_PART_MAIN);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(parent, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_style_pad_gap(parent, 14, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Guide rapide");
    lv_obj_set_style_text_color(title, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t *desc = lv_label_create(parent);
    lv_label_set_text(desc,
                      "Saisis les dimensions internes du terrarium puis navigue dans les onglets :\n"
                      "• Tapis/Câble : chauffages calculés en densité surfacique sécurisée.\n"
                      "• Éclairage : flux lumineux, UVA/UVB basés sur zones de Ferguson.\n"
                      "• Substrat : volume et masse selon densité typique.\n"
                      "• Brumisation : buses, volume de réservoir avec marge.");
    lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(desc, LV_PCT(100));
    lv_obj_set_style_text_color(desc, COLOR_MUTED, LV_PART_MAIN);

    create_help(parent,
                "Hypothèses et limites",
                "Calculs conservateurs, adaptés à des tensions SELV 12/24 V. Vérifie toujours avec des instruments (thermomètre IR,"
                " UVI-mètre, débitmètre). Les normes électriques (NF C 15-100) et exigences espèces priment sur les valeurs données."
                " Prévoir marge supplémentaire si le terrarium est fortement ventilé ou ouvert.");
}

