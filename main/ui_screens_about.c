#include "ui_screens_about.h"

#define COLOR_TEXT lv_color_hex(0xE2E8F0)
#define COLOR_MUTED lv_color_hex(0x94A3B8)
#define COLOR_SURFACE lv_color_hex(0x111827)
#define COLOR_ACCENT lv_color_hex(0x22D3EE)

static lv_obj_t *create_panel(lv_obj_t *parent, const char *title, const char *body)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_width(panel, LV_PCT(100));
    lv_obj_set_style_bg_color(panel, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_pad_all(panel, 12, LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 8, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, COLOR_ACCENT, LV_PART_MAIN);
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
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, COLOR_TEXT, LV_PART_MAIN);

    create_panel(parent,
                 "Bonnes pratiques",
                 "Ce calculateur fournit des ordres de grandeur pour la conception de terrariums.\n"
                 "- Respecte les normes basse tension (NF C 15-100, directives CE) : privilégie 12/24 V SELV, protections"
                 " fusibles/disjoncteurs et câbles silicone.\n"
                 "- Mesure la densité surfacique des chauffages avec un thermomètre IR avant l'installation d'animaux.\n"
                 "- UV : contrôle obligatoire à l'UVI-mètre, aligne-toi sur les zones de Ferguson adaptées à l'espèce.\n"
                 "- Humidité/brumisation : ventilation suffisante, bac de rétention et anti-goutte.");

    create_panel(parent,
                 "Hypothèses & limites",
                 "Données utilisées : chauffages 0,03-0,06 W/cm², zones Ferguson UVI 0-6, densités substrats 0,45-1,7 kg/L,"
                 " buses 60-120 mL/min. Les résultats restent indicatifs : valider avec les fiches fabricants, les réglementations"
                 " locales et les besoins biologiques de l'espèce. Installer des protections mécaniques contre les brûlures et"
                 " l'humidité sur les éléments électriques.");
}

