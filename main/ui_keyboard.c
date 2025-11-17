#include "ui_keyboard.h"

#include <string.h>

static lv_obj_t *s_keyboard;

static const char *const text_map[] = {
    "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  "0",  "Bksp", "\n",
    "a",  "z",  "e",  "r",  "t",  "y",  "u",  "i",  "o",  "p",  "û",   "é",   "\n",
    "q",  "s",  "d",  "f",  "g",  "h",  "j",  "k",  "l",  "m",  "è",   "à",   "\n",
    "Shift", "w",  "x",  "c",  "v",  "b",  "n",  "ç",  "ù",  ".",  "Enter", "\n",
    "123", "Space", "Hide", "",   "",   "",   "",   "",   "",   "",   "",   ""
};

static const char *const num_map[] = {
    "1",  "2",  "3",  "4",  "5",  "Bksp", "\n",
    "6",  "7",  "8",  "9",  "0",  "Enter", "\n",
    "+",  "-",  ".",  ",",  "00", "ABC",   "\n",
    "Space", "Hide", "",   "",   "",   "",     "",     "",     "",     "",     "",     ""
};

typedef struct {
    bool numeric;
    bool allow_decimal;
} kb_binding_t;

static void keyboard_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(target, LV_OBJ_FLAG_HIDDEN);
    } else if (code == LV_EVENT_VALUE_CHANGED) {
        uint16_t id = lv_btnmatrix_get_selected_btn(target);
        const char *txt = lv_btnmatrix_get_btn_text(target, id);
        if (txt) {
            if (strcmp(txt, "123") == 0) {
                lv_keyboard_set_map(s_keyboard, num_map, NULL, NULL);
            } else if (strcmp(txt, "ABC") == 0) {
                lv_keyboard_set_map(s_keyboard, text_map, NULL, NULL);
            } else if (strcmp(txt, "Hide") == 0) {
                lv_obj_add_flag(target, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void textarea_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    kb_binding_t *binding = lv_event_get_user_data(e);
    if (code == LV_EVENT_FOCUSED) {
        if (s_keyboard) {
            lv_keyboard_set_textarea(s_keyboard, ta);
            if (binding && binding->numeric) {
                lv_keyboard_set_map(s_keyboard, num_map, NULL, NULL);
                lv_textarea_set_accepted_chars(ta, binding->allow_decimal ? "0123456789.," : "0123456789");
            } else {
                lv_keyboard_set_map(s_keyboard, text_map, NULL, NULL);
                lv_textarea_set_accepted_chars(ta, NULL);
            }
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
    lv_obj_set_style_max_height(s_keyboard, 220, LV_PART_MAIN);
    lv_obj_align(s_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_map(s_keyboard, text_map, NULL, NULL);
    lv_keyboard_set_mode(s_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_add_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_keyboard, keyboard_event_cb, LV_EVENT_ALL, NULL);
}

static kb_binding_t *alloc_binding(bool numeric, bool allow_decimal)
{
    kb_binding_t *b = lv_mem_alloc(sizeof(kb_binding_t));
    if (!b) {
        return NULL;
    }
    b->numeric = numeric;
    b->allow_decimal = allow_decimal;
    return b;
}

void ui_keyboard_attach_text(lv_obj_t *textarea)
{
    if (!textarea) {
        return;
    }
    kb_binding_t *binding = alloc_binding(false, false);
    lv_obj_add_event_cb(textarea, textarea_event_cb, LV_EVENT_FOCUSED, binding);
    lv_obj_add_event_cb(textarea, textarea_event_cb, LV_EVENT_DEFOCUSED, binding);
}

void ui_keyboard_attach_numeric(lv_obj_t *textarea, bool allow_decimal)
{
    if (!textarea) {
        return;
    }
    kb_binding_t *binding = alloc_binding(true, allow_decimal);
    lv_obj_add_event_cb(textarea, textarea_event_cb, LV_EVENT_FOCUSED, binding);
    lv_obj_add_event_cb(textarea, textarea_event_cb, LV_EVENT_DEFOCUSED, binding);
}

