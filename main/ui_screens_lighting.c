#include "ui_screens_lighting.h"

#include <stdio.h>
#include <stdlib.h>

#include "calc_lighting.h"
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
    return strtof(txt, NULL);
}

static terrarium_environment_t env_from_dd(lv_obj_t *dd)
{
    switch (lv_dropdown_get_selected(dd)) {
    case 0:
        return TERRARIUM_ENV_TROPICAL;
    case 1:
        return TERRARIUM_ENV_DESERTIC;
    case 2:
        return TERRARIUM_ENV_TEMPERATE_FOREST;
    case 3:
    default:
        return TERRARIUM_ENV_NOCTURNAL;
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
    lv_obj_t *env_dd = controls[3];
    lv_obj_t *flux_ta = controls[4];
    lv_obj_t *led_power_ta = controls[5];
    lv_obj_t *uva_ta = controls[6];
    lv_obj_t *uvb_ta = controls[7];
    lv_obj_t *dist_ta = controls[8];
    lv_obj_t *out_label = controls[9];

    lighting_input_t in = {
        .length_cm = parse_float(lv_textarea_get_text(length_ta), 100.0f),
        .depth_cm = parse_float(lv_textarea_get_text(depth_ta), 60.0f),
        .height_cm = parse_float(lv_textarea_get_text(height_ta), 60.0f),
        .environment = env_from_dd(env_dd),
        .led_luminous_flux_lm = parse_float(lv_textarea_get_text(flux_ta), 150.0f),
        .led_power_w = parse_float(lv_textarea_get_text(led_power_ta), 1.0f),
        .uva_irradiance_mw_cm2_at_distance = parse_float(lv_textarea_get_text(uva_ta), 3.0f),
        .uvb_uvi_at_distance = parse_float(lv_textarea_get_text(uvb_ta), 1.2f),
        .reference_distance_cm = parse_float(lv_textarea_get_text(dist_ta), 30.0f),
    };

    lighting_result_t out = {0};
    if (lighting_calculate(&in, &out)) {
        char buf[420];
        snprintf(buf,
                 sizeof(buf),
                 "Cible %.0f lux : %u LED (%.0f lm, %.1f W) sur %.2f m².\n"
                 "UVB zone Ferguson %.1f-%.1f : %u module(s) à %.0f cm, UVI estimé %.2f (total %.2f)%s%s.\n"
                 "UVA cible %.1f mW/cm² : %u module(s) à %.0f cm, estimé %.2f%s%s.",
        char buf[320];
        snprintf(buf,
                 sizeof(buf),
                 "Cible %.0f lux : %u LED (%.0f lm, %.1f W).\n"
                 "UVB cible UVI %.1f : %u module(s) à %.0f cm%s.\n"
                 "UVA cible %.1f mW/cm² : %u module(s) à %.0f cm%s.",
                 out.led.target_lux,
                 out.led.led_count,
                 out.led.total_flux_lm,
                 out.led.total_power_w,
                 out.led.area_m2,
                 out.uvb.target_uvi_min,
                 out.uvb.target_uvi_max,
                 out.uvb.module_count,
                 out.uvb.recommended_distance_cm,
                 out.uvb.estimated_uvi_at_distance,
                 out.uvb.estimated_total_uvi,
                 out.uvb.warning_high ? " (trop haut)" : "",
                 out.uvb.warning_low ? " (trop bas)" : "",
                 out.uva.target_uvi_min,
                 out.uva.module_count,
                 out.uva.recommended_distance_cm,
                 out.uva.estimated_uvi_at_distance,
                 out.uva.warning_high ? " (trop haut)" : "",
                 out.uva.warning_low ? " (trop bas)" : "");
        lv_label_set_text(out_label, buf);
        storage_save_lighting(&in);
                 out.uvb.target_uvi,
                 out.uvb.module_count,
                 out.uvb.recommended_distance_cm,
                 out.uvb.warning_high ? " (alerte UVI)" : "",
                 out.uva.target_uvi,
                 out.uva.module_count,
                 out.uva.recommended_distance_cm,
                 out.uva.warning_high ? " (alerte UVA)" : "");
        lv_label_set_text(out_label, buf);
    } else {
        lv_label_set_text(out_label, "Entrées invalides pour l'éclairage.");
    }
}

void ui_screen_lighting_build(lv_obj_t *parent)
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

    lighting_input_t defaults = {0};
    storage_load_lighting(&defaults);
    char tmp[16];

    lv_obj_t *length_ta; create_input_row(inputs, "Longueur (cm)", &length_ta, "100");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.length_cm); lv_textarea_set_text(length_ta, tmp);
    lv_obj_t *depth_ta; create_input_row(inputs, "Profondeur (cm)", &depth_ta, "60");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.depth_cm); lv_textarea_set_text(depth_ta, tmp);
    lv_obj_t *height_ta; create_input_row(inputs, "Hauteur (cm)", &height_ta, "60");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.height_cm); lv_textarea_set_text(height_ta, tmp);
    lv_obj_t *flux_ta; create_input_row(inputs, "Flux par LED (lm)", &flux_ta, "150");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.led_luminous_flux_lm); lv_textarea_set_text(flux_ta, tmp);
    lv_obj_t *led_pow_ta; create_input_row(inputs, "Puissance par LED (W)", &led_pow_ta, "1");
    snprintf(tmp, sizeof(tmp), "%.1f", defaults.led_power_w); lv_textarea_set_text(led_pow_ta, tmp);
    lv_obj_t *uva_ta; create_input_row(inputs, "UVA module (mW/cm²)", &uva_ta, "3");
    snprintf(tmp, sizeof(tmp), "%.1f", defaults.uva_irradiance_mw_cm2_at_distance); lv_textarea_set_text(uva_ta, tmp);
    lv_obj_t *uvb_ta; create_input_row(inputs, "UVB module (UVI)", &uvb_ta, "1.2");
    snprintf(tmp, sizeof(tmp), "%.2f", defaults.uvb_uvi_at_distance); lv_textarea_set_text(uvb_ta, tmp);
    lv_obj_t *dist_ta; create_input_row(inputs, "Distance modules (cm)", &dist_ta, "30");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.reference_distance_cm); lv_textarea_set_text(dist_ta, tmp);
    lv_obj_t *length_ta; create_input_row(inputs, "Longueur (cm)", &length_ta, "100");
    lv_obj_t *depth_ta; create_input_row(inputs, "Profondeur (cm)", &depth_ta, "60");
    lv_obj_t *height_ta; create_input_row(inputs, "Hauteur (cm)", &height_ta, "60");
    lv_obj_t *flux_ta; create_input_row(inputs, "Flux par LED (lm)", &flux_ta, "150");
    lv_obj_t *led_pow_ta; create_input_row(inputs, "Puissance par LED (W)", &led_pow_ta, "1");
    lv_obj_t *uva_ta; create_input_row(inputs, "UVA module (mW/cm²)", &uva_ta, "3");
    lv_obj_t *uvb_ta; create_input_row(inputs, "UVB module (UVI)", &uvb_ta, "1.2");
    lv_obj_t *dist_ta; create_input_row(inputs, "Distance modules (cm)", &dist_ta, "30");

    lv_obj_t *env_cont = lv_obj_create(inputs);
    lv_obj_set_size(env_cont, 220, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(env_cont, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(env_cont, 6, LV_PART_MAIN);
    lv_obj_set_flex_flow(env_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(env_cont, 4, LV_PART_MAIN);
    lv_obj_remove_flag(env_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *env_lbl = lv_label_create(env_cont);
    lv_label_set_text(env_lbl, "Type d'environnement");
    lv_obj_t *env_dd = lv_dropdown_create(env_cont);
    lv_dropdown_set_options(env_dd, "Tropical\nDésertique\nForêt tempérée\nNocturne");
    lv_dropdown_set_selected(env_dd, defaults.environment == TERRARIUM_ENV_TROPICAL
                                                 ? 0
                                                 : defaults.environment == TERRARIUM_ENV_DESERTIC
                                                       ? 1
                                                       : defaults.environment == TERRARIUM_ENV_TEMPERATE_FOREST ? 2 : 3);
    lv_dropdown_set_selected(env_dd, 1);

    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_width(btn, 180);
    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Calculer");
    lv_obj_center(btn_lbl);

    lv_obj_t *out = lv_label_create(parent);
    lv_obj_set_width(out, LV_PCT(100));
    lv_label_set_long_mode(out, LV_LABEL_LONG_WRAP);
    lv_label_set_text(out, "Résultats éclairage en attente.");

    lv_obj_t *help = lv_label_create(parent);
    lv_obj_set_width(help, LV_PCT(100));
    lv_label_set_long_mode(help, LV_LABEL_LONG_WRAP);
    lv_label_set_text(help,
                      "Zones de Ferguson : zone 1 (0-1 UVI nocturne), zone 2 (0,7-2 UVI forêt), zone 3 (1-3 UVI tropical), zone 4 (3-6 UVI désert). "
                      "UVI calculé en 1/r² depuis la distance de référence. Toujours vérifier à l'UVI-mètre avant usage.");

    static lv_obj_t *controls[10];
    controls[0] = length_ta;
    controls[1] = depth_ta;
    controls[2] = height_ta;
    controls[3] = env_dd;
    controls[4] = flux_ta;
    controls[5] = led_pow_ta;
    controls[6] = uva_ta;
    controls[7] = uvb_ta;
    controls[8] = dist_ta;
    controls[9] = out;
    lv_obj_add_event_cb(btn, calculate_cb, LV_EVENT_CLICKED, controls);
}

