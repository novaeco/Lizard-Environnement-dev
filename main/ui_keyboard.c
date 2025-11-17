#include "ui_keyboard.h"

#include <string.h>

#define KB_COLOR_BG lv_color_hex(0x111827)
#define KB_COLOR_KEY lv_color_hex(0x1F2937)
#define KB_COLOR_TEXT lv_color_hex(0xE2E8F0)

typedef struct {
    ui_keyboard_mode_t mode;
} kb_binding_t;

static lv_obj_t *s_keyboard;
static ui_keyboard_mode_t s_default_mode = UI_KEYBOARD_MODE_TEXT;

static const char *const s_text_map[] = {
    "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  "0",  "Bksp", "\n",
    "a",  "z",  "e",  "r",  "t",  "y",  "u",  "i",  "o",  "p",  "é",   "è",   "\n",
    "q",  "s",  "d",  "f",  "g",  "h",  "j",  "k",  "l",  "m",  "à",   "ù",   "\n",
    "Shift", "w",  "x",  "c",  "v",  "b",  "n",  "ç",  "-",  "'",  "Enter", "\n",
    "123", "Space", ".",  ",",  "?",  "!",  "Hide", "",   "",   "",   "",   ""
};

static const char *const s_numeric_map[] = {
    "1",  "2",  "3",  "4",  "5",  "Bksp", "\n",
    "6",  "7",  "8",  "9",  "0",  "Enter", "\n",
    "+",  "-",  ".",  ",",  "00", "ABC",   "\n",
    "Space", "Hide", "",   "",   "",   "",     "",     "",     "",     "",     "",     ""
};

static void keyboard_apply_layout(ui_keyboard_mode_t mode)
{
    if (!s_keyboard) {
        return;
    }

    switch (mode) {
    case UI_KEYBOARD_MODE_TEXT:
        lv_keyboard_set_map(s_keyboard, LV_KEYBOARD_MODE_USER_1, s_text_map, NULL);
        lv_keyboard_set_mode(s_keyboard, LV_KEYBOARD_MODE_USER_1);
        break;
    case UI_KEYBOARD_MODE_NUMERIC:
    case UI_KEYBOARD_MODE_DECIMAL:
        lv_keyboard_set_map(s_keyboard, LV_KEYBOARD_MODE_USER_2, s_numeric_map, NULL);
        lv_keyboard_set_mode(s_keyboard, LV_KEYBOARD_MODE_USER_2);
        break;
    }
}

static void keyboard_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (code != LV_EVENT_VALUE_CHANGED) {
        return;
    }

    uint16_t id = lv_btnmatrix_get_selected_btn(kb);
    const char *txt = lv_btnmatrix_get_btn_text(kb, id);
    if (!txt) {
        return;
    }

    if (strcmp(txt, "123") == 0) {
        keyboard_apply_layout(UI_KEYBOARD_MODE_NUMERIC);
    } else if (strcmp(txt, "ABC") == 0) {
        keyboard_apply_layout(UI_KEYBOARD_MODE_TEXT);
    } else if (strcmp(txt, "Hide") == 0) {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

static void textarea_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    kb_binding_t *binding = lv_event_get_user_data(e);

    if (code == LV_EVENT_FOCUSED) {
        if (!s_keyboard) {
            return;
        }

        ui_keyboard_mode_t mode = binding ? binding->mode : s_default_mode;
        keyboard_apply_layout(mode);
        lv_keyboard_set_textarea(s_keyboard, ta);

        switch (mode) {
        case UI_KEYBOARD_MODE_TEXT:
            lv_textarea_set_accepted_chars(ta, NULL);
            break;
        case UI_KEYBOARD_MODE_NUMERIC:
            lv_textarea_set_accepted_chars(ta, "0123456789+-");
            break;
        case UI_KEYBOARD_MODE_DECIMAL:
            lv_textarea_set_accepted_chars(ta, "0123456789+-,.");
            break;
        }

        lv_obj_clear_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
    } else if (code == LV_EVENT_DEFOCUSED || code == LV_EVENT_CANCEL) {
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
    lv_obj_set_style_max_height(s_keyboard, 240, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_keyboard, KB_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_keyboard, lv_color_hex(0x10B981), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_keyboard, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_keyboard, 6, LV_PART_MAIN);

    lv_obj_set_style_bg_color(s_keyboard, KB_COLOR_KEY, LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_keyboard, KB_COLOR_TEXT, LV_PART_ITEMS);
    lv_obj_set_style_min_width(s_keyboard, 64, LV_PART_ITEMS);
    lv_obj_set_style_min_height(s_keyboard, 48, LV_PART_ITEMS);
    lv_obj_set_style_radius(s_keyboard, 6, LV_PART_ITEMS);
    lv_obj_set_style_text_font(s_keyboard, &lv_font_montserrat_20, LV_PART_ITEMS);

    keyboard_apply_layout(s_default_mode);
    lv_obj_add_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_keyboard, keyboard_event_cb, LV_EVENT_ALL, NULL);
}

void ui_keyboard_set_default_mode(ui_keyboard_mode_t mode)
{
    s_default_mode = mode;
    keyboard_apply_layout(mode);
}

void ui_keyboard_attach(lv_obj_t *textarea, ui_keyboard_mode_t mode)
{
    if (!textarea) {
        return;
    }

    kb_binding_t *binding = lv_mem_alloc(sizeof(kb_binding_t));
    if (!binding) {
        return;
    }
    binding->mode = mode;

    lv_obj_add_event_cb(textarea, textarea_event_cb, LV_EVENT_FOCUSED, binding);
    lv_obj_add_event_cb(textarea, textarea_event_cb, LV_EVENT_DEFOCUSED, binding);
}

void ui_keyboard_attach_text(lv_obj_t *textarea)
{
    ui_keyboard_attach(textarea, UI_KEYBOARD_MODE_TEXT);
}

void ui_keyboard_attach_numeric(lv_obj_t *textarea, bool allow_decimal)
{
    ui_keyboard_attach(textarea, allow_decimal ? UI_KEYBOARD_MODE_DECIMAL : UI_KEYBOARD_MODE_NUMERIC);
}

