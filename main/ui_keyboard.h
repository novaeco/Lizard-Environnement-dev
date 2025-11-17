#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_keyboard_create(lv_obj_t *parent);
void ui_keyboard_attach_text(lv_obj_t *textarea);
void ui_keyboard_attach_numeric(lv_obj_t *textarea, bool allow_decimal);

#ifdef __cplusplus
}
#endif

