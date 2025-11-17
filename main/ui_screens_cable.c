#include "ui_screens_cable.h"

#include <stdio.h>
#include <stdlib.h>

#include "calc_heating_cable.h"
#include "storage.h"
#include "ui_keyboard.h"

#define COLOR_TEXT lv_color_hex(0xE2E8F0)
#define COLOR_MUTED lv_color_hex(0x94A3B8)
#define COLOR_SURFACE lv_color_hex(0x111827)
#define COLOR_ACCENT lv_color_hex(0x22D3EE)

static float parse_decimal(const char *txt, float def)
{
    if (!txt || txt[0] == '\0') {
        return def;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%s", txt);
    for (size_t i = 0; i < sizeof(buf); ++i) {
        if (buf[i] == ',') {
            buf[i] = '.';
        }
    }
    char *end = NULL;
    float v = strtof(buf, &end);
    if (end == buf) {
        return def;
    }
    return v;
}

static terrarium_material_t material_from_dd(lv_obj_t *dd)
{
    switch (lv_dropdown_get_selected(dd)) {
    case 0:
        return TERRARIUM_MATERIAL_WOOD;
    case 1:
        return TERRARIUM_MATERIAL_GLASS;
    case 2:
        return TERRARIUM_MATERIAL_PVC;
    case 3:
    default:
        return TERRARIUM_MATERIAL_ACRYLIC;
    }
}

static lv_obj_t *create_card(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_style_bg_color(card, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(card, 10, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 8, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 2, LV_PART_MAIN);
    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW_WRAP);
    return card;
}

static lv_obj_t *create_help_block(lv_obj_t *parent, const char *title, const char *body)
{
    lv_obj_t *block = lv_obj_create(parent);
    lv_obj_set_width(block, LV_PCT(100));
    lv_obj_set_style_bg_color(block, COLOR_SURFACE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(block, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_pad_all(block, 12, LV_PART_MAIN);
    lv_obj_set_style_radius(block, 8, LV_PART_MAIN);
    lv_obj_set_style_border_color(block, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_border_width(block, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(block, 6, LV_PART_MAIN);

    lv_obj_t *hdr = lv_label_create(block);
    lv_label_set_text(hdr, title);
    lv_obj_set_style_text_color(hdr, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_set_style_text_font(hdr, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t *body_lbl = lv_label_create(block);
    lv_obj_set_width(body_lbl, LV_PCT(100));
    lv_label_set_long_mode(body_lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(body_lbl, body);
    lv_obj_set_style_text_color(body_lbl, COLOR_MUTED, LV_PART_MAIN);

    return block;
}

static lv_obj_t *create_input_row(lv_obj_t *parent, const char *label, const char *placeholder)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 240, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(cont, 6, LV_PART_MAIN);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(cont);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_color(lbl, COLOR_TEXT, LV_PART_MAIN);

    lv_obj_t *ta = lv_textarea_create(cont);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, placeholder);
    lv_textarea_set_max_length(ta, 8);
    lv_obj_set_width(ta, LV_PCT(100));
    lv_obj_set_style_min_height(ta, 48, LV_PART_MAIN);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_20, LV_PART_MAIN);
    ui_keyboard_attach(ta, UI_KEYBOARD_MODE_DECIMAL);

    return ta;
}

static void calculate_cb(lv_event_t *e)
{
    lv_obj_t **controls = lv_event_get_user_data(e);
    lv_obj_t *length_ta = controls[0];
    lv_obj_t *depth_ta = controls[1];
    lv_obj_t *ratio_ta = controls[2];
    lv_obj_t *density_ta = controls[3];
    lv_obj_t *power_linear_ta = controls[4];
    lv_obj_t *material_dd = controls[5];
    lv_obj_t *spacing_ta = controls[6];
    lv_obj_t *supply_ta = controls[7];
    lv_obj_t *out_label = controls[8];

    heating_cable_input_t in = {
        .length_cm = parse_decimal(lv_textarea_get_text(length_ta), 100.0f),
        .depth_cm = parse_decimal(lv_textarea_get_text(depth_ta), 60.0f),
        .heated_ratio = parse_decimal(lv_textarea_get_text(ratio_ta), 0.33f),
        .target_power_density_w_per_cm2 = parse_decimal(lv_textarea_get_text(density_ta), 0.035f),
        .power_linear_w_per_m = parse_decimal(lv_textarea_get_text(power_linear_ta), 20.0f),
        .material = material_from_dd(material_dd),
        .spacing_cm = parse_decimal(lv_textarea_get_text(spacing_ta), 4.0f),
        .supply_voltage_v = parse_decimal(lv_textarea_get_text(supply_ta), 24.0f),
    };

    heating_cable_result_t out = {0};
    if (heating_cable_calculate(&in, &out) && out.valid) {
        char buf[320];
        snprintf(buf,
                 sizeof(buf),
                 "Surface chauffée: %.0f cm²\n"
                 "Longueur câble: %.2f m (pas %.1f cm)\n"
                 "Puissance cible: %.1f W (%.3f W/cm²)\n"
                 "I estimé: %.2f A, R≈%.1f Ω @ %.0f V\n%s%s%s",
                 out.heated_area_cm2,
                 out.recommended_length_m,
                 out.spacing_cm,
                 out.target_power_w,
                 out.resulting_density_w_per_cm2,
                 out.estimated_current_a,
                 out.estimated_resistance_ohm,
                 in.supply_voltage_v,
                 out.warning_density_high ? "Alerte densité : réduire la puissance ou augmenter la surface. " : "Densité ok. ",
                 out.warning_spacing_too_tight ? "Spirale trop serrée (<3 cm). " : "",
                 out.warning_high_voltage ? "230 V uniquement théorique : préférer 12/24 V SELV." : "");
        lv_label_set_text(out_label, buf);
        storage_save_heating_cable(&in);
    } else {
        lv_label_set_text(out_label, "Entrées invalides pour le calcul de câble chauffant.");
    }
}

void ui_screen_cable_build(lv_obj_t *parent)
{
    lv_obj_set_style_pad_all(parent, 14, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(parent, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(parent, 12, LV_PART_MAIN);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);

    lv_obj_t *inputs = create_card(parent);

    heating_cable_input_t defaults = {0};
    storage_load_heating_cable(&defaults);

    char tmp[16];
    lv_obj_t *length_ta = create_input_row(inputs, "Longueur (cm)", "100");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.length_cm);
    lv_textarea_set_text(length_ta, tmp);

    lv_obj_t *depth_ta = create_input_row(inputs, "Profondeur (cm)", "60");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.depth_cm);
    lv_textarea_set_text(depth_ta, tmp);

    lv_obj_t *ratio_ta = create_input_row(inputs, "Ratio chauffé", "0.33");
    snprintf(tmp, sizeof(tmp), "%.2f", defaults.heated_ratio);
    lv_textarea_set_text(ratio_ta, tmp);

    lv_obj_t *density_ta = create_input_row(inputs, "Densité cible (W/cm²)", "0.035");
    snprintf(tmp, sizeof(tmp), "%.3f", defaults.target_power_density_w_per_cm2);
    lv_textarea_set_text(density_ta, tmp);

    lv_obj_t *power_lin_ta = create_input_row(inputs, "Puissance linéique (W/m)", "20");
    snprintf(tmp, sizeof(tmp), "%.1f", defaults.power_linear_w_per_m);
    lv_textarea_set_text(power_lin_ta, tmp);

    lv_obj_t *spacing_ta = create_input_row(inputs, "Pas des spires (cm)", "4");
    snprintf(tmp, sizeof(tmp), "%.1f", defaults.spacing_cm);
    lv_textarea_set_text(spacing_ta, tmp);

    lv_obj_t *supply_ta = create_input_row(inputs, "Tension câble (V)", "24");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.supply_voltage_v);
    lv_textarea_set_text(supply_ta, tmp);

    lv_obj_t *mat_cont = lv_obj_create(inputs);
    lv_obj_set_size(mat_cont, 240, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(mat_cont, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(mat_cont, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(mat_cont, 6, LV_PART_MAIN);
    lv_obj_set_flex_flow(mat_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(mat_cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *mat_lbl = lv_label_create(mat_cont);
    lv_label_set_text(mat_lbl, "Matériau plancher");
    lv_obj_set_style_text_color(mat_lbl, COLOR_TEXT, LV_PART_MAIN);

    lv_obj_t *material_dd = lv_dropdown_create(mat_cont);
    lv_dropdown_set_options(material_dd, "Bois\nVerre\nPVC\nAcrylique");
    lv_dropdown_set_selected(material_dd, defaults.material);
    lv_obj_set_style_min_height(material_dd, 44, LV_PART_MAIN);
    lv_obj_set_style_text_font(material_dd, &lv_font_montserrat_20, LV_PART_MAIN);

    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_width(btn, 200);
    lv_obj_set_style_min_height(btn, 52, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_text_font(btn, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 10, LV_PART_MAIN);
    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Calculer");
    lv_obj_set_style_text_color(btn_lbl, COLOR_TEXT, LV_PART_MAIN);
    lv_obj_center(btn_lbl);

    lv_obj_t *out = lv_label_create(parent);
    lv_obj_set_width(out, LV_PCT(100));
    lv_label_set_long_mode(out, LV_LABEL_LONG_WRAP);
    lv_label_set_text(out, "Résultats câble chauffant en attente.");
    lv_obj_set_style_text_color(out, COLOR_TEXT, LV_PART_MAIN);

    create_help_block(parent,
                      "Aide & limites",
                      "Espacement conseillé 3-6 cm. Densité cible : verre 0,03-0,04 W/cm², bois 0,04-0,05 W/cm²."
                      " Les valeurs 230 V sont indicatives : privilégier 12/24 V SELV avec disjoncteur différentiel et protection"
                      " mécanique du câble.");

    static lv_obj_t *controls[9];
    controls[0] = length_ta;
    controls[1] = depth_ta;
    controls[2] = ratio_ta;
    controls[3] = density_ta;
    controls[4] = power_lin_ta;
    controls[5] = material_dd;
    controls[6] = spacing_ta;
    controls[7] = supply_ta;
    controls[8] = out;
    lv_obj_add_event_cb(btn, calculate_cb, LV_EVENT_CLICKED, controls);
}

