#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"

#include "lvgl.h"

#include "board_waveshare_7b.h"
#include "calc.h"
#include "gt911.h"

#define TAG "app"

static esp_lcd_panel_handle_t s_panel_handle = NULL;
static gt911_touch_point_t s_last_touch = {0};

typedef struct {
    lv_obj_t *length_cm;
    lv_obj_t *width_cm;
    lv_obj_t *height_cm;
    lv_obj_t *substrate_cm;
    lv_obj_t *material_dd;
    lv_obj_t *heater_density;
    lv_obj_t *lux_target;
    lv_obj_t *led_efficiency;
    lv_obj_t *led_unit_power;
    lv_obj_t *uv_target;
    lv_obj_t *uv_module;
    lv_obj_t *mist_density;
    lv_obj_t *heater_label;
    lv_obj_t *lighting_label;
    lv_obj_t *environment_label;
} ui_controls_t;

static ui_controls_t s_ui = {0};

static void lcd_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    if (!s_panel_handle || !area || !px_map) {
        lv_display_flush_ready(disp);
        return;
    }

    int32_t width = lv_area_get_width(area);
    int32_t height = lv_area_get_height(area);
    if (width <= 0 || height <= 0) {
        lv_display_flush_ready(disp);
        return;
    }

    esp_lcd_panel_draw_bitmap(s_panel_handle,
                              area->x1,
                              area->y1,
                              area->x2 + 1,
                              area->y2 + 1,
                              px_map);
    lv_display_flush_ready(disp);
}

static void lv_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(5);
}

static bool touchpad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    gt911_touch_point_t point = {0};
    if (gt911_read_touch(&point) == ESP_OK && point.touched) {
        s_last_touch = point;
        data->point.x = (point.x < WAVESHARE7B_LCD_H_RES) ? point.x : (WAVESHARE7B_LCD_H_RES - 1);
        data->point.y = (point.y < WAVESHARE7B_LCD_V_RES) ? point.y : (WAVESHARE7B_LCD_V_RES - 1);
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->point.x = s_last_touch.x;
        data->point.y = s_last_touch.y;
        data->state = LV_INDEV_STATE_RELEASED;
    }
    return false;
}

static lv_obj_t *create_numeric_field(lv_obj_t *parent, const char *label_text, const char *default_value)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, label_text);

    lv_obj_t *ta = lv_textarea_create(parent);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, 16);
    lv_textarea_set_text(ta, default_value);
    lv_textarea_set_align(ta, LV_TEXT_ALIGN_RIGHT);
    lv_obj_set_width(ta, LV_PCT(100));
    lv_textarea_set_accepted_chars(ta, "0123456789.,");
    return ta;
}

static void update_results_labels(const calc_results_t *results)
{
    if (!results) {
        return;
    }

    if (s_ui.heater_label) {
        lv_label_set_text_fmt(
            s_ui.heater_label,
            "Surface sol: %.0f cm²\n"
            "Surface tapis (~1/3): %.0f cm²\n"
            "Côté tapis: %.1f cm\n"
            "Surface corrigée: %.0f cm²\n"
            "Puissance demandée: %.1f W\n"
            "Puissance catalogue: %u W\n"
            "Tension: %.0f V\n"
            "Courant: %.2f A\n"
            "Résistance: %.1f Ω",
            results->area_floor_cm2,
            results->heater_target_area_cm2,
            results->heater_square_side_cm,
            results->heater_corrected_area_cm2,
            results->heater_power_w,
            results->heater_catalog_power_w,
            results->heater_voltage_v,
            results->heater_current_a,
            results->heater_resistance_ohm);
    }

    if (s_ui.lighting_label) {
        lv_label_set_text_fmt(
            s_ui.lighting_label,
            "Surface lumineuse: %.2f m²\n"
            "Flux requis: %.0f lm\n"
            "Puissance estimée: %.1f W\n"
            "Nombre de LED: %u\n"
            "Puissance totale LED: %.1f W\n"
            "Modules UV: %u",
            results->led_area_m2,
            results->led_luminous_flux_lm,
            results->led_required_power_w,
            results->led_count,
            results->led_total_power_w,
            results->uv_module_count);
    }

    if (s_ui.environment_label) {
        lv_label_set_text_fmt(
            s_ui.environment_label,
            "Volume substrat: %.1f L\n"
            "Buses de brumisation: %u",
            results->substrate_volume_l,
            results->mist_nozzle_count);
    }
}

static float parse_text_value(lv_obj_t *ta, float fallback)
{
    if (!ta) {
        return fallback;
    }
    const char *txt = lv_textarea_get_text(ta);
    if (!txt || txt[0] == '\0') {
        return fallback;
    }
    char *endptr = NULL;
    float value = strtof(txt, &endptr);
    if (endptr == txt) {
        return fallback;
    }
    return value;
}

static void calculate_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    calc_inputs_t inputs;
    calc_get_default_inputs(&inputs);

    inputs.length_cm = parse_text_value(s_ui.length_cm, inputs.length_cm);
    inputs.width_cm = parse_text_value(s_ui.width_cm, inputs.width_cm);
    inputs.height_cm = parse_text_value(s_ui.height_cm, inputs.height_cm);
    inputs.substrate_thickness_cm = parse_text_value(s_ui.substrate_cm, inputs.substrate_thickness_cm);
    inputs.floor_heater_power_density_w_per_cm2 = parse_text_value(s_ui.heater_density, inputs.floor_heater_power_density_w_per_cm2);
    inputs.led_target_lux = parse_text_value(s_ui.lux_target, inputs.led_target_lux);
    inputs.led_efficiency_lm_per_w = parse_text_value(s_ui.led_efficiency, inputs.led_efficiency_lm_per_w);
    inputs.led_unit_power_w = parse_text_value(s_ui.led_unit_power, inputs.led_unit_power_w);
    inputs.uv_target_intensity = parse_text_value(s_ui.uv_target, inputs.uv_target_intensity);
    inputs.uv_module_intensity = parse_text_value(s_ui.uv_module, inputs.uv_module_intensity);
    inputs.mist_nozzle_density_m2 = parse_text_value(s_ui.mist_density, inputs.mist_nozzle_density_m2);

    if (s_ui.material_dd) {
        uint16_t selected = lv_dropdown_get_selected(s_ui.material_dd);
        if (selected < CALC_MATERIAL_COUNT) {
            inputs.floor_material = (calc_material_t)selected;
        }
    }

    calc_results_t results;
    calc_compute(&inputs, &results);
    update_results_labels(&results);
}

static void build_ui(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(scr, 16, 0);
    lv_obj_set_style_pad_row(scr, 12, 0);
    lv_obj_set_style_pad_column(scr, 18, 0);

    lv_obj_t *input_panel = lv_obj_create(scr);
    lv_obj_set_size(input_panel, 470, LV_PCT(100));
    lv_obj_set_flex_flow(input_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(input_panel, 12, 0);
    lv_obj_set_style_pad_row(input_panel, 8, 0);

    lv_obj_t *title = lv_label_create(input_panel);
    lv_label_set_text(title, "Paramètres terrarium");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);

    s_ui.length_cm = create_numeric_field(input_panel, "Longueur (cm)", "120");
    s_ui.width_cm = create_numeric_field(input_panel, "Largeur (cm)", "60");
    s_ui.height_cm = create_numeric_field(input_panel, "Hauteur (cm)", "60");
    s_ui.substrate_cm = create_numeric_field(input_panel, "Épaisseur substrat (cm)", "5");
    s_ui.heater_density = create_numeric_field(input_panel, "Densité tapis (W/cm²)", "0.040");

    lv_obj_t *material_label = lv_label_create(input_panel);
    lv_label_set_text(material_label, "Matériau plancher");

    s_ui.material_dd = lv_dropdown_create(input_panel);
    lv_dropdown_set_options_static(s_ui.material_dd, "Bois\nVerre\nPVC\nAcrylique");
    lv_dropdown_set_selected(s_ui.material_dd, CALC_MATERIAL_GLASS);
    lv_obj_set_width(s_ui.material_dd, LV_PCT(100));

    s_ui.lux_target = create_numeric_field(input_panel, "Lux cibles", "12000");
    s_ui.led_efficiency = create_numeric_field(input_panel, "Efficacité LED (lm/W)", "110");
    s_ui.led_unit_power = create_numeric_field(input_panel, "Puissance par LED (W)", "5");
    s_ui.uv_target = create_numeric_field(input_panel, "Intensité UV cible", "150");
    s_ui.uv_module = create_numeric_field(input_panel, "Intensité par module UV", "80");
    s_ui.mist_density = create_numeric_field(input_panel, "Densité brumisation (m²/buse)", "0.10");

    lv_obj_t *button = lv_button_create(input_panel);
    lv_obj_set_width(button, LV_PCT(100));
    lv_obj_add_event_cb(button, calculate_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_label = lv_label_create(button);
    lv_label_set_text(btn_label, "Calculer");
    lv_obj_center(btn_label);

    lv_obj_t *result_panel = lv_obj_create(scr);
    lv_obj_set_size(result_panel, 470, LV_PCT(100));
    lv_obj_set_flex_flow(result_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(result_panel, 12, 0);
    lv_obj_set_style_pad_row(result_panel, 10, 0);

    lv_obj_t *heater_title = lv_label_create(result_panel);
    lv_label_set_text(heater_title, "Chauffage");
    lv_obj_set_style_text_font(heater_title, &lv_font_montserrat_20, 0);
    s_ui.heater_label = lv_label_create(result_panel);
    lv_label_set_long_mode(s_ui.heater_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_ui.heater_label, LV_PCT(100));

    lv_obj_t *lighting_title = lv_label_create(result_panel);
    lv_label_set_text(lighting_title, "Éclairage / UV");
    lv_obj_set_style_text_font(lighting_title, &lv_font_montserrat_20, 0);
    s_ui.lighting_label = lv_label_create(result_panel);
    lv_label_set_long_mode(s_ui.lighting_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_ui.lighting_label, LV_PCT(100));

    lv_obj_t *environment_title = lv_label_create(result_panel);
    lv_label_set_text(environment_title, "Environnement");
    lv_obj_set_style_text_font(environment_title, &lv_font_montserrat_20, 0);
    s_ui.environment_label = lv_label_create(result_panel);
    lv_label_set_long_mode(s_ui.environment_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_ui.environment_label, LV_PCT(100));

    calc_inputs_t defaults;
    calc_get_default_inputs(&defaults);
    calc_results_t results;
    calc_compute(&defaults, &results);
    update_results_labels(&results);
}

static void init_backlight(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << WAVESHARE7B_PIN_BACKLIGHT) | (1ULL << WAVESHARE7B_PIN_LCD_EN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(WAVESHARE7B_PIN_LCD_EN, 1);
    gpio_set_level(WAVESHARE7B_PIN_BACKLIGHT, 1);

    /*
    // Optionnel : activer via expandeur CH422G si câblé
    // ch422g_set_pin(EXIO6, 1); // LCD_VDD_EN
    // ch422g_set_pin(EXIO2, 1); // Rétroéclairage
    */
}

static void init_display(void)
{
    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = 1,
        .clk_src = LCD_CLK_SRC_PLL160M,
        .timings = {
            .pclk_hz = WAVESHARE7B_LCD_PIXEL_CLOCK_HZ,
            .h_res = WAVESHARE7B_LCD_H_RES,
            .v_res = WAVESHARE7B_LCD_V_RES,
            .hsync_pulse_width = 20,
            .hsync_back_porch = 160,
            .hsync_front_porch = 160,
            .vsync_pulse_width = 3,
            .vsync_back_porch = 23,
            .vsync_front_porch = 12,
            .flags = {
                .pclk_active_neg = 1,
                .hsync_idle_high = 0,
                .vsync_idle_high = 0,
                .de_idle_high = 0,
            },
        },
        .hsync_gpio_num = WAVESHARE7B_PIN_HSYNC,
        .vsync_gpio_num = WAVESHARE7B_PIN_VSYNC,
        .de_gpio_num = WAVESHARE7B_PIN_DE,
        .pclk_gpio_num = WAVESHARE7B_PIN_PCLK,
        .data_gpio_nums = {
            WAVESHARE7B_PIN_B3,
            WAVESHARE7B_PIN_B4,
            WAVESHARE7B_PIN_B5,
            WAVESHARE7B_PIN_B6,
            WAVESHARE7B_PIN_B7,
            WAVESHARE7B_PIN_G2,
            WAVESHARE7B_PIN_G3,
            WAVESHARE7B_PIN_G4,
            WAVESHARE7B_PIN_G5,
            WAVESHARE7B_PIN_G6,
            WAVESHARE7B_PIN_G7,
            WAVESHARE7B_PIN_R3,
            WAVESHARE7B_PIN_R4,
            WAVESHARE7B_PIN_R5,
            WAVESHARE7B_PIN_R6,
            WAVESHARE7B_PIN_R7,
        },
        .disp_gpio_num = -1,
        .flags = {
            .fb_in_psram = 1,
            .double_fb = 0,
            .refresh_on_demand = 0,
        },
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel_handle, true));
}

static void init_lvgl(void)
{
    lv_init();

    size_t buffer_pixels = (WAVESHARE7B_LCD_H_RES * WAVESHARE7B_LCD_V_RES) / 10;
    size_t buffer_size_bytes = buffer_pixels * sizeof(lv_color_t);

    lv_color_t *buf1 = heap_caps_malloc(buffer_size_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    lv_color_t *buf2 = heap_caps_malloc(buffer_size_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf1 || !buf2) {
        ESP_LOGE(TAG, "Échec allocation buffers LVGL (%zu bytes)", buffer_size_bytes);
        abort();
    }

    lv_display_t *display = lv_display_create(WAVESHARE7B_LCD_H_RES, WAVESHARE7B_LCD_V_RES);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, lcd_flush_cb);
    lv_display_set_buffers(display, buf1, buf2, buffer_pixels, LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchpad_read_cb);
}

static void init_lvgl_tick(void)
{
    const esp_timer_create_args_t timer_args = {
        .callback = lv_tick_cb,
        .name = "lv_tick"
    };
    esp_timer_handle_t timer_handle;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handle, 5000));
}

static void init_i2c_touch(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = WAVESHARE7B_PIN_TOUCH_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = WAVESHARE7B_PIN_TOUCH_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = WAVESHARE7B_I2C_TOUCH_SPEED_HZ,
        .clk_flags = 0,
    };
    ESP_ERROR_CHECK(i2c_param_config(WAVESHARE7B_I2C_TOUCH_PORT, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(WAVESHARE7B_I2C_TOUCH_PORT, conf.mode, 0, 0, 0));

    gt911_config_t gt_conf = {
        .port = WAVESHARE7B_I2C_TOUCH_PORT,
        .address = 0x5D,
        .irq_gpio = WAVESHARE7B_PIN_TOUCH_IRQ,
    };
    ESP_ERROR_CHECK(gt911_init(&gt_conf));
}

void app_main(void)
{
    init_backlight();
    init_display();
    init_lvgl();
    init_lvgl_tick();
    init_i2c_touch();

    build_ui();

    while (true) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
