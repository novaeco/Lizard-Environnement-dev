#include "ui_screens_substrate.h"

#include <stdio.h>
#include <stdlib.h>

#include "calc_substrate.h"
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

static substrate_type_t type_from_dd(lv_obj_t *dd)
{
    switch (lv_dropdown_get_selected(dd)) {
    case 0:
        return SUBSTRATE_SOIL;
    case 1:
        return SUBSTRATE_COCO;
    case 2:
        return SUBSTRATE_FOREST_BLEND;
    case 3:
        return SUBSTRATE_SAND;
    case 4:
    default:
        return SUBSTRATE_SAND_SOIL;
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
    lv_obj_t *substrate_ta = controls[3];
    lv_obj_t *type_dd = controls[4];
    lv_obj_t *out_label = controls[5];

    substrate_input_t in = {
        .length_cm = parse_float(lv_textarea_get_text(length_ta), 100.0f),
        .depth_cm = parse_float(lv_textarea_get_text(depth_ta), 60.0f),
        .height_cm = parse_float(lv_textarea_get_text(height_ta), 60.0f),
        .substrate_height_cm = parse_float(lv_textarea_get_text(substrate_ta), 8.0f),
        .type = type_from_dd(type_dd),
    };

    substrate_result_t out = {0};
    if (substrate_calculate(&in, &out) && out.valid) {
        char buf[256];
        snprintf(buf,
                 sizeof(buf),
                 "Volume: %.1f L\nMasse: %.1f-%.1f kg (densité %.2f-%.2f kg/L)\nNominal: %.1f kg",
                 out.volume_l,
                 out.mass_min_kg,
                 out.mass_max_kg,
                 out.density_min_kg_per_l,
                 out.density_max_kg_per_l,
                 out.mass_kg);
        lv_label_set_text(out_label, buf);
        storage_save_substrate(&in);
        char buf[192];
        snprintf(buf,
                 sizeof(buf),
                 "Volume: %.1f L\nMasse approximative: %.1f kg (densité %.2f kg/L)",
                 out.volume_l,
                 out.mass_kg,
                 out.density_kg_per_l);
        lv_label_set_text(out_label, buf);
    } else {
        lv_label_set_text(out_label, "Entrées invalides pour le substrat.");
    }
}

void ui_screen_substrate_build(lv_obj_t *parent)
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

    substrate_input_t defaults = {0};
    storage_load_substrate(&defaults);
    char tmp[16];

    lv_obj_t *length_ta; create_input_row(inputs, "Longueur (cm)", &length_ta, "100");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.length_cm); lv_textarea_set_text(length_ta, tmp);
    lv_obj_t *depth_ta; create_input_row(inputs, "Profondeur (cm)", &depth_ta, "60");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.depth_cm); lv_textarea_set_text(depth_ta, tmp);
    lv_obj_t *height_ta; create_input_row(inputs, "Hauteur (cm)", &height_ta, "60");
    snprintf(tmp, sizeof(tmp), "%.0f", defaults.height_cm); lv_textarea_set_text(height_ta, tmp);
    lv_obj_t *substrate_ta; create_input_row(inputs, "Hauteur substrat (cm)", &substrate_ta, "8");
    snprintf(tmp, sizeof(tmp), "%.1f", defaults.substrate_height_cm); lv_textarea_set_text(substrate_ta, tmp);
    lv_obj_t *length_ta; create_input_row(inputs, "Longueur (cm)", &length_ta, "100");
    lv_obj_t *depth_ta; create_input_row(inputs, "Profondeur (cm)", &depth_ta, "60");
    lv_obj_t *height_ta; create_input_row(inputs, "Hauteur (cm)", &height_ta, "60");
    lv_obj_t *substrate_ta; create_input_row(inputs, "Hauteur substrat (cm)", &substrate_ta, "8");

    lv_obj_t *type_cont = lv_obj_create(inputs);
    lv_obj_set_size(type_cont, 220, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(type_cont, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(type_cont, 6, LV_PART_MAIN);
    lv_obj_set_flex_flow(type_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(type_cont, 4, LV_PART_MAIN);
    lv_obj_remove_flag(type_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *type_lbl = lv_label_create(type_cont);
    lv_label_set_text(type_lbl, "Type de substrat");
    lv_obj_t *type_dd = lv_dropdown_create(type_cont);
    lv_dropdown_set_options(type_dd, "Terreau\nFibre coco\nMélange forestier\nSable\nSable/terre");
    lv_dropdown_set_selected(type_dd, defaults.type);
    lv_dropdown_set_selected(type_dd, 2);

    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_width(btn, 180);
    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Calculer");
    lv_obj_center(btn_lbl);

    lv_obj_t *out = lv_label_create(parent);
    lv_obj_set_width(out, LV_PCT(100));
    lv_label_set_long_mode(out, LV_LABEL_LONG_WRAP);
    lv_label_set_text(out, "Résultats substrat en attente.");

    lv_obj_t *help = lv_label_create(parent);
    lv_obj_set_width(help, LV_PCT(100));
    lv_label_set_long_mode(help, LV_LABEL_LONG_WRAP);
    lv_label_set_text(help,
                      "Densités typiques : terreau 0,65-0,85 kg/L, coco 0,45-0,65 kg/L, forêt 0,60-0,80 kg/L, sable 1,5-1,7 kg/L."
                      " Prévoir +10% pour tassement et pertes.");

    static lv_obj_t *controls[6];
    controls[0] = length_ta;
    controls[1] = depth_ta;
    controls[2] = height_ta;
    controls[3] = substrate_ta;
    controls[4] = type_dd;
    controls[5] = out;
    lv_obj_add_event_cb(btn, calculate_cb, LV_EVENT_CLICKED, controls);
}

