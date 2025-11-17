#include "ui_screens_misting.h"

#include <stdio.h>
#include <stdlib.h>

#include "calc_misting.h"
#include "storage.h"
#include "ui_keyboard.h"

static float parse_float(const char *txt, float def)
{
    if (!txt || txt[0] == '\0') {
        return def;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%s", txt);
    for (size_t i = 0; i < sizeof(buf); ++i) {
        if (buf[i] == ',') {
            buf[i] = '.';
        }
    }
    return strtof(buf, NULL);
}

static mist_environment_t env_from_dd(lv_obj_t *dd)
{
    switch (lv_dropdown_get_selected(dd)) {
    case 0:
        return MIST_ENV_TROPICAL;
    case 1:
        return MIST_ENV_TEMPERATE_HUMID;
    case 2:
        return MIST_ENV_SEMI_ARID;
    case 3:
    default:
        return MIST_ENV_DESERTIC;
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
}

static void calculate_cb(lv_event_t *e)
{
    lv_obj_t **controls = lv_event_get_user_data(e);
    lv_obj_t *length_ta = controls[0];
    lv_obj_t *depth_ta = controls[1];
    lv_obj_t *flow_ta = controls[2];
    lv_obj_t *duration_ta = controls[3];
    lv_obj_t *cycle_ta = controls[4];
    lv_obj_t *autonomy_ta = controls[5];
    lv_obj_t *env_dd = controls[6];
    lv_obj_t *out_label = controls[7];

    misting_input_t in = {
        .length_cm = parse_float(lv_textarea_get_text(length_ta), 100.0f),
        .depth_cm = parse_float(lv_textarea_get_text(depth_ta), 60.0f),
        .nozzle_flow_ml_per_min = parse_float(lv_textarea_get_text(flow_ta), 80.0f),
        .cycle_duration_min = parse_float(lv_textarea_get_text(duration_ta), 2.0f),
        .cycles_per_day = (uint32_t)parse_float(lv_textarea_get_text(cycle_ta), 4),
        .autonomy_days = (uint32_t)parse_float(lv_textarea_get_text(autonomy_ta), 3),
        .environment = env_from_dd(env_dd),
    };

    misting_result_t out = {0};
    if (misting_calculate(&in, &out) && out.valid) {
        char buf[256];
        snprintf(buf,
                 sizeof(buf),
                 "Buses recommandées: %u\nConsommation: %.2f L/jour\nRéservoir %.2f L (%.2f L pour 3j, %.2f L pour 7j)%s",
                 out.nozzle_count,
                 out.daily_consumption_l,
                 out.tank_volume_l,
                 out.tank_volume_autonomy3_l,
                 out.tank_volume_autonomy7_l,
                 out.warning_dense_spray ? " (attention : buses très proches, risque de saturation)" : "");
        lv_label_set_text(out_label, buf);
        storage_save_misting(&in);
    } else {
        lv_label_set_text(out_label, "Entrées invalides pour la brumisation.");
    }
}

void ui_screen_misting_build(lv_obj_t *parent)
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

    misting_input_t defaults = {0};
    storage_load_misting(&defaults);
    char tmp[16];

    lv_obj_t *length_ta; create_input_row(inputs, "Longueur (cm)", &length_ta, "100");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.length_cm); lv_textarea_set_text(length_ta, tmp);
    lv_obj_t *depth_ta; create_input_row(inputs, "Profondeur (cm)", &depth_ta, "60");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.depth_cm); lv_textarea_set_text(depth_ta, tmp);
    lv_obj_t *flow_ta; create_input_row(inputs, "Débit buse (mL/min)", &flow_ta, "80");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.nozzle_flow_ml_per_min); lv_textarea_set_text(flow_ta, tmp);
    lv_obj_t *duration_ta; create_input_row(inputs, "Durée cycle (min)", &duration_ta, "2");
    snprintf(tmp, sizeof(tmp), "%.1f", defaults.cycle_duration_min); lv_textarea_set_text(duration_ta, tmp);
    lv_obj_t *cycle_ta; create_input_row(inputs, "Cycles / jour", &cycle_ta, "4");
    snprintf(tmp, sizeof(tmp), "%u", defaults.cycles_per_day); lv_textarea_set_text(cycle_ta, tmp);
    lv_obj_t *autonomy_ta; create_input_row(inputs, "Autonomie (jours)", &autonomy_ta, "3");
    snprintf(tmp, sizeof(tmp), "%u", defaults.autonomy_days); lv_textarea_set_text(autonomy_ta, tmp);

    lv_obj_t *env_cont = lv_obj_create(inputs);
    lv_obj_set_size(env_cont, 220, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(env_cont, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(env_cont, 6, LV_PART_MAIN);
    lv_obj_set_flex_flow(env_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(env_cont, 4, LV_PART_MAIN);
    lv_obj_remove_flag(env_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *env_lbl = lv_label_create(env_cont);
    lv_label_set_text(env_lbl, "Type de milieu");
    lv_obj_t *env_dd = lv_dropdown_create(env_cont);
    lv_dropdown_set_options(env_dd, "Tropical\nTempéré humide\nSemi-aride\nDésertique");
    lv_dropdown_set_selected(env_dd, defaults.environment);

    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_width(btn, 180);
    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Calculer");
    lv_obj_center(btn_lbl);

    lv_obj_t *out = lv_label_create(parent);
    lv_obj_set_width(out, LV_PCT(100));
    lv_label_set_long_mode(out, LV_LABEL_LONG_WRAP);
    lv_label_set_text(out, "Résultats brumisation en attente.");

    lv_obj_t *help = lv_label_create(parent);
    lv_obj_set_width(help, LV_PCT(100));
    lv_label_set_long_mode(help, LV_LABEL_LONG_WRAP);
    lv_label_set_text(help,
                      "Débits busette fine typiques 60-120 mL/min. Couverture 0,08-0,16 m²/buse selon milieu. Ajoute 20% de marge sur le volume, prévois 3 ou 7 jours d'autonomie.");

    static lv_obj_t *controls[8];
    controls[0] = length_ta;
    controls[1] = depth_ta;
    controls[2] = flow_ta;
    controls[3] = duration_ta;
    controls[4] = cycle_ta;
    controls[5] = autonomy_ta;
    controls[6] = env_dd;
    controls[7] = out;
    lv_obj_add_event_cb(btn, calculate_cb, LV_EVENT_CLICKED, controls);
}

