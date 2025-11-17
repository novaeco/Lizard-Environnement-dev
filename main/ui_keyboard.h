#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_KEYBOARD_MODE_TEXT = 0,
    UI_KEYBOARD_MODE_NUMERIC,
    UI_KEYBOARD_MODE_DECIMAL,
} ui_keyboard_mode_t;

void ui_keyboard_create(lv_obj_t *parent);
void ui_keyboard_set_default_mode(ui_keyboard_mode_t mode);
void ui_keyboard_attach(lv_obj_t *textarea, ui_keyboard_mode_t mode);
void ui_keyboard_attach_text(lv_obj_t *textarea);
void ui_keyboard_attach_numeric(lv_obj_t *textarea, bool allow_decimal);

#ifdef __cplusplus
}
#endif

