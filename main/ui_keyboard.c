#include "ui_keyboard.h"

static lv_obj_t *s_keyboard;
static const char *const kb_map[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "Bksp", "\n",
    "a", "z", "e", "r", "t", "y", "u", "i", "o", "p", "-", "\n",
    "q", "s", "d", "f", "g", "h", "j", "k", "l", "m", ",", "\n",
    "Shift", "w", "x", "c", "v", "b", "n", ";", ":", "?", "Enter", "\n",
    "123", "Space", "Hide", "", "", "", "", "", "", "", "", ""
};

static void keyboard_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(target, LV_OBJ_FLAG_HIDDEN);
    }
}

static void textarea_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    if (code == LV_EVENT_FOCUSED) {
        if (s_keyboard) {
            lv_keyboard_set_textarea(s_keyboard, ta);
            lv_obj_clear_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
        }
    } else if (code == LV_EVENT_DEFOCUSED) {
        if (s_keyboard) {
            lv_obj_add_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_keyboard_create(lv_obj_t *parent)
{
    if (s_keyboard) {
        return;
    }
    s_keyboard = lv_keyboard_create(parent);
    lv_obj_set_size(s_keyboard, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_align(s_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_map(s_keyboard, kb_map, NULL, NULL);
    lv_keyboard_set_mode(s_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_add_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_keyboard, keyboard_event_cb, LV_EVENT_ALL, NULL);
}

void ui_keyboard_attach(lv_obj_t *textarea)
{
    if (!textarea) {
        return;
    }
    lv_obj_add_event_cb(textarea, textarea_event_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(textarea, textarea_event_cb, LV_EVENT_DEFOCUSED, NULL);
}

