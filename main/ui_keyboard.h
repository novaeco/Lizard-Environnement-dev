#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_keyboard_create(lv_obj_t *parent);
void ui_keyboard_attach(lv_obj_t *textarea);

#ifdef __cplusplus
}
#endif

