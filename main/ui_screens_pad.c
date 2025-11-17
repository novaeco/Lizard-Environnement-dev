#include "ui_screens_pad.h"

#include <stdio.h>
#include <stdlib.h>

#include "calc_heating_pad.h"
#include "storage.h"
#include "ui_keyboard.h"

static float parse_float(const char *txt, float def)
{
    if (!txt || txt[0] == '\0') {
        return def;
    }
    char *end = NULL;
    float v = strtof(txt, &end);
    if (end && *end == '\0') {
        return v;
    }
    // Essaye avec virgule décimale
    char buf[16];
    snprintf(buf, sizeof(buf), "%s", txt);
    for (size_t i = 0; i < sizeof(buf); ++i) {
        if (buf[i] == ',') {
            buf[i] = '.';
        }
    }
    return strtof(buf, NULL);
    return strtof(txt, NULL);
}

static terrarium_material_t material_from_dd(lv_obj_t *dd)
{
    uint16_t sel = lv_dropdown_get_selected(dd);
    switch (sel) {
    case 0:
        return TERRARIUM_MATERIAL_WOOD;
    case 1:
        return TERRARIUM_MATERIAL_GLASS;
    case 2:
        return TERRARIUM_MATERIAL_PVC;
    case 3:
        return TERRARIUM_MATERIAL_ACRYLIC;
    default:
        return TERRARIUM_MATERIAL_GLASS;
    }
}

static void create_input_row(lv_obj_t *parent, const char *label, lv_obj_t **ta, const char *placeholder)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 220, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(cont, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(cont, 4, LV_PART_MAIN);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(cont);
    lv_label_set_text(lbl, label);

    *ta = lv_textarea_create(cont);
    lv_textarea_set_one_line(*ta, true);
    lv_textarea_set_placeholder_text(*ta, placeholder);
    lv_textarea_set_max_length(*ta, 8);
    lv_obj_set_width(*ta, LV_PCT(100));
    ui_keyboard_attach_numeric(*ta, true);
    ui_keyboard_attach(*ta);
}

static void calculate_cb(lv_event_t *e)
{
    lv_obj_t **controls = lv_event_get_user_data(e);
    lv_obj_t *length_ta = controls[0];
    lv_obj_t *depth_ta = controls[1];
    lv_obj_t *height_ta = controls[2];
    lv_obj_t *ratio_ta = controls[3];
    lv_obj_t *material_dd = controls[4];
    lv_obj_t *out_label = controls[5];

    heating_pad_input_t in = {
        .length_cm = parse_float(lv_textarea_get_text(length_ta), 100.0f),
        .depth_cm = parse_float(lv_textarea_get_text(depth_ta), 60.0f),
        .height_cm = parse_float(lv_textarea_get_text(height_ta), 60.0f),
        .heated_ratio = parse_float(lv_textarea_get_text(ratio_ta), 0.33f),
        .material = material_from_dd(material_dd),
    };
    heating_pad_result_t out = {0};
    if (heating_pad_calculate(&in, &out) && out.valid) {
        char buf[256];
        snprintf(buf,
                 sizeof(buf),
                 "Surface chauffée: %.0f cm² (≈%.1f cm de côté)\n"
                 "Puissance catalogue: %.1f W (densité %.3f W/cm²)\n"
                 "Tension %.0f V, I=%.2f A, R=%.1f Ω\n"
                 "Limite matière: %.3f W/cm²\n%s%s",
                 "Tension %.0f V, I=%.2f A, R=%.1f Ω\n%s",
                 out.heated_area_cm2,
                 out.heater_side_cm,
                 out.power_w,
                 out.power_density_w_per_cm2,
                 out.voltage_v,
                 out.current_a,
                 out.resistance_ohm,
                 out.density_limit_w_per_cm2,
                 out.warning_density_over ? "ALERTE : densité dépasse la limite matière. " : "",
                 (!out.warning_density_over && out.warning_density_high)
                     ? "Avertissement : densité proche de la limite, réduire le ratio ou la puissance."
                     : "Densité dans la plage sécurisée.");
        lv_label_set_text(out_label, buf);
        storage_save_heating_pad(&in);
                 out.warning_density_high ? "Alerte : densité proche de la limite matière." : "Densité dans la plage sécurisée.");
        lv_label_set_text(out_label, buf);
    } else {
        lv_label_set_text(out_label, "Entrées invalides pour le calcul de tapis chauffant.");
    }
}

void ui_screen_pad_build(lv_obj_t *parent)
{
    lv_obj_set_style_pad_all(parent, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(parent, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(parent, 12, LV_PART_MAIN);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);

    lv_obj_t *inputs = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(inputs, LV_OPA_10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(inputs, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(inputs, 12, LV_PART_MAIN);
    lv_obj_set_flex_flow(inputs, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_width(inputs, LV_PCT(100));

    heating_pad_input_t defaults = {0};
    storage_load_heating_pad(&defaults);

    lv_obj_t *length_ta; create_input_row(inputs, "Longueur (cm)", &length_ta, "100");
    char tmp[16];
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.length_cm);
    lv_textarea_set_text(length_ta, tmp);
    lv_obj_t *depth_ta; create_input_row(inputs, "Profondeur (cm)", &depth_ta, "60");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.depth_cm);
    lv_textarea_set_text(depth_ta, tmp);
    lv_obj_t *height_ta; create_input_row(inputs, "Hauteur (cm)", &height_ta, "60");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.height_cm);
    lv_textarea_set_text(height_ta, tmp);
    lv_obj_t *ratio_ta; create_input_row(inputs, "Ratio surface chauffée (0.2-0.6)", &ratio_ta, "0.33");
    snprintf(tmp, sizeof(tmp), "%.2f", defaults.heated_ratio);
    lv_textarea_set_text(ratio_ta, tmp);
    lv_obj_t *length_ta; create_input_row(inputs, "Longueur (cm)", &length_ta, "100");
    lv_obj_t *depth_ta; create_input_row(inputs, "Profondeur (cm)", &depth_ta, "60");
    lv_obj_t *height_ta; create_input_row(inputs, "Hauteur (cm)", &height_ta, "60");
    lv_obj_t *ratio_ta; create_input_row(inputs, "Ratio surface chauffée (0.2-0.6)", &ratio_ta, "0.33");

    lv_obj_t *mat_cont = lv_obj_create(inputs);
    lv_obj_set_size(mat_cont, 220, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(mat_cont, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(mat_cont, 6, LV_PART_MAIN);
    lv_obj_set_flex_flow(mat_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(mat_cont, 4, LV_PART_MAIN);
    lv_obj_remove_flag(mat_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *mat_lbl = lv_label_create(mat_cont);
    lv_label_set_text(mat_lbl, "Matériau plancher");
    lv_obj_t *material_dd = lv_dropdown_create(mat_cont);
    lv_dropdown_set_options(material_dd, "Bois\nVerre\nPVC\nAcrylique");
    lv_dropdown_set_selected(material_dd, defaults.material == TERRARIUM_MATERIAL_WOOD
                                                 ? 0
                                                 : defaults.material == TERRARIUM_MATERIAL_GLASS
                                                       ? 1
                                                       : defaults.material == TERRARIUM_MATERIAL_PVC ? 2 : 3);
    lv_dropdown_set_selected(material_dd, 1);

    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_width(btn, 180);
    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Calculer");
    lv_obj_center(btn_lbl);

    lv_obj_t *out = lv_label_create(parent);
    lv_obj_set_width(out, LV_PCT(100));
    lv_label_set_long_mode(out, LV_LABEL_LONG_WRAP);
    lv_label_set_text(out, "Résultats tapis chauffant en attente.");

    lv_obj_t *help = lv_label_create(parent);
    lv_obj_set_width(help, LV_PCT(100));
    lv_label_set_long_mode(help, LV_LABEL_LONG_WRAP);
    lv_label_set_text(help,
                      "Hypothèses : densité catalogue ≈0,04 W/cm² (tableau fourni), limite matière 0,045-0,065 W/cm² selon support. "
                      "Tension 12 V <=18 W sinon 24 V. Réduis le ratio chauffé si la densité approche la limite.");

    static lv_obj_t *controls[6];
    controls[0] = length_ta;
    controls[1] = depth_ta;
    controls[2] = height_ta;
    controls[3] = ratio_ta;
    controls[4] = material_dd;
    controls[5] = out;
    lv_obj_add_event_cb(btn, calculate_cb, LV_EVENT_CLICKED, controls);
}

