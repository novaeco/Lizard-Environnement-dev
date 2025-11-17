#include "ui_screens_home.h"

void ui_screen_home_build(lv_obj_t *parent)
{
    lv_obj_set_style_pad_all(parent, 16, LV_PART_MAIN);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_opa(parent, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Guide rapide");
    lv_obj_set_style_text_color(title, lv_color_hex(0xE2E8F0), LV_PART_MAIN);

    lv_obj_t *desc = lv_label_create(parent);
    lv_label_set_text(desc,
                      "Saisis les dimensions internes du terrarium puis navigue sur les onglets :\n"
                      "• Tapis : surface chauffée par défaut 1/3, tension auto 12/24 V.\n"
                      "• Câble : longueur issue de la densité surfacique cible.\n"
                      "• Éclairage : flux 6500K, modules UVA/UVB basés sur zones de Ferguson.\n"
                      "• Substrat : volume + masse via densités typiques.\n"
                      "• Brumisation : buses et volume de réservoir avec marge 20%.\n"
                      "Les valeurs sont conservatrices et ne remplacent pas les normes (NF C15-100, CE, fiches fabricants)." );
    lv_label_set_long_mode(desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(desc, LV_PCT(100));
    lv_obj_set_style_text_color(desc, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
}

