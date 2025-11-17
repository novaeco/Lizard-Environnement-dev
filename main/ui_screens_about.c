#include "ui_screens_about.h"

void ui_screen_about_build(lv_obj_t *parent)
{
    lv_obj_set_style_pad_all(parent, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(parent, 12, LV_PART_MAIN);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_opa(parent, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);

    lv_obj_t *title = lv_label_create(parent);
    lv_obj_set_width(title, LV_PCT(100));
    lv_label_set_text(title, "À propos & sécurité");
    lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, LV_PART_MAIN);

    lv_obj_t *body = lv_label_create(parent);
    lv_obj_set_width(body, LV_PCT(100));
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_label_set_text(body,
                      "Ce calculateur fournit des ordres de grandeur pour la conception de terrariums.\n"
                      "- Respecte les normes basse tension (NF C 15-100, directives CE) : privilégie 12/24 V SELV, protections "
                      "fusibles/disjoncteurs, câbles souples silicone.\n"
                      "- Vérifie la densité surfacique des chauffages avec un thermomètre infrarouge avant l'installation d'animaux.\n"
                      "- UV : mesure impérative à l'UVI-mètre, aligne-toi sur les zones de Ferguson adaptées à l'espèce.\n"
                      "- Humidité/brumisation : prévois une ventilation suffisante et un bac de rétention.\n"
                      "Sources : fiches fabricants tapis/câbles chauffants (0,03-0,06 W/cm²), zones Ferguson (UVI 0-6), densités substrats "
                      "horticoles (0,45-1,7 kg/L), buses 60-120 mL/min.");
}

