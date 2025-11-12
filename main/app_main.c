#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_idf_version.h"
#include "soc/soc_caps.h"
#if SOC_PSRAM_SUPPORTED
#include "esp_psram.h"
#endif
#include "lvgl.h"

#include "board_waveshare_7b.h"
#include "calc.h"
#include "gt911.h"

static const char *TAG = "app";

static esp_lcd_panel_handle_t s_panel_handle;
static esp_timer_handle_t s_lvgl_tick_timer;
static gt911_handle_t s_touch_handle;
static bool s_lvgl_task_wdt_registered;
#if SOC_PSRAM_SUPPORTED
static bool s_psram_available;

static bool query_psram_once(void)
{
    static bool s_checked;
    if (!s_checked) {
        s_psram_available = esp_psram_is_initialized();
        if (!s_psram_available) {
            ESP_LOGW(TAG, "PSRAM not detected, using internal SRAM buffers");
        }
        s_checked = true;
    }
    return s_psram_available;
}
#else
static bool query_psram_once(void)
{
    return false;
}
#endif

typedef struct {
    lv_obj_t *length_cm;
    lv_obj_t *width_cm;
    lv_obj_t *height_cm;
    lv_obj_t *material_dd;
    lv_obj_t *substrate_cm;
    lv_obj_t *lux_target;
    lv_obj_t *led_efficiency;
    lv_obj_t *led_power;
    lv_obj_t *uv_target;
    lv_obj_t *uv_module;
    lv_obj_t *mist_density;
    lv_obj_t *status_label;
    lv_obj_t *heater_label;
    lv_obj_t *lighting_label;
    lv_obj_t *uv_label;
    lv_obj_t *substrate_label;
    lv_obj_t *misting_label;
} terrarium_ui_ctx_t;

static bool rgb_panel_color_trans_cb(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_data)
{
    (void)panel;
    (void)edata;
    lv_display_t *disp = (lv_display_t *)user_data;
    if (disp != NULL) {
        lv_display_flush_ready(disp);
    }
    return false;
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    const int32_t x1 = area->x1;
    const int32_t y1 = area->y1;
    const int32_t x2 = area->x2;
    const int32_t y2 = area->y2;

    esp_err_t err = esp_lcd_panel_draw_bitmap(s_panel_handle, x1, y1, x2 + 1, y2 + 1, px_map);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Panel draw bitmap failed: %s", esp_err_to_name(err));
        lv_display_flush_ready(disp);
    }
}

static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(5);
}

static void configure_task_wdt(void)
{
#if CONFIG_ESP_TASK_WDT_INIT
    const esp_task_wdt_config_t config = {
        .timeout_ms = 12000,
        .trigger_panic = true,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    };
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
    esp_err_t err = esp_task_wdt_reconfigure(&config);
#else
    esp_err_t err = esp_task_wdt_init(config.timeout_ms / 1000, config.trigger_panic);
#endif
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "Task WDT reconfigure failed: %s", esp_err_to_name(err));
    }
#endif
}

static void lvgl_touch_read_cb(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    (void)indev_drv;
    uint16_t x = 0;
    uint16_t y = 0;
    bool pressed = false;
    if (gt911_read_touch(&s_touch_handle, &x, &y, &pressed) == ESP_OK) {
        data->point.x = x;
        data->point.y = y;
        data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void lvgl_task(void *arg)
{
    (void)arg;
    esp_err_t wdt_err = esp_task_wdt_add(NULL);
    if (wdt_err == ESP_OK) {
        s_lvgl_task_wdt_registered = true;
    } else if (wdt_err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "Failed to register LVGL task to WDT: %s", esp_err_to_name(wdt_err));
    }
    while (true) {
        lv_timer_handler();
        if (s_lvgl_task_wdt_registered) {
            esp_task_wdt_reset();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void init_backlight(void)
{
    if (BOARD_LCD_PIN_BACKLIGHT != GPIO_NUM_NC) {
        gpio_config_t bk_conf = {
            .pin_bit_mask = BIT64(BOARD_LCD_PIN_BACKLIGHT),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&bk_conf));
        gpio_set_level(BOARD_LCD_PIN_BACKLIGHT, BOARD_LCD_BK_LIGHT_OFF_LEVEL);
    }
}

static void enable_backlight(void)
{
    if (BOARD_LCD_PIN_BACKLIGHT != GPIO_NUM_NC) {
        gpio_set_level(BOARD_LCD_PIN_BACKLIGHT, BOARD_LCD_BK_LIGHT_ON_LEVEL);
    }
}

static void init_display(void)
{
    init_backlight();

    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_width = BOARD_LCD_DATA_WIDTH,
        .psram_trans_align = 64,
        .sram_trans_align = 4,
        .hsync_gpio_num = BOARD_LCD_PIN_HSYNC,
        .vsync_gpio_num = BOARD_LCD_PIN_VSYNC,
        .de_gpio_num = BOARD_LCD_PIN_DE,
        .pclk_gpio_num = BOARD_LCD_PIN_PCLK,
        .disp_gpio_num = -1,
        .data_gpio_nums = {
            BOARD_LCD_PIN_DATA0,
            BOARD_LCD_PIN_DATA1,
            BOARD_LCD_PIN_DATA2,
            BOARD_LCD_PIN_DATA3,
            BOARD_LCD_PIN_DATA4,
            BOARD_LCD_PIN_DATA5,
            BOARD_LCD_PIN_DATA6,
            BOARD_LCD_PIN_DATA7,
            BOARD_LCD_PIN_DATA8,
            BOARD_LCD_PIN_DATA9,
            BOARD_LCD_PIN_DATA10,
            BOARD_LCD_PIN_DATA11,
            BOARD_LCD_PIN_DATA12,
            BOARD_LCD_PIN_DATA13,
            BOARD_LCD_PIN_DATA14,
            BOARD_LCD_PIN_DATA15,
        },
        .timings = {
            .pclk_hz = BOARD_LCD_PIXEL_CLOCK_HZ,
            .h_res = BOARD_LCD_H_RES,
            .v_res = BOARD_LCD_V_RES,
            .hsync_pulse_width = BOARD_LCD_TIMING_HPW,
            .hsync_back_porch = BOARD_LCD_TIMING_HBP,
            .hsync_front_porch = BOARD_LCD_TIMING_HFP,
            .vsync_pulse_width = BOARD_LCD_TIMING_VPW,
            .vsync_back_porch = BOARD_LCD_TIMING_VBP,
            .vsync_front_porch = BOARD_LCD_TIMING_VFP,
        },
    };
    panel_config.timings.flags.pclk_active_neg = BOARD_LCD_TIMING_PCLK_ACTIVE_NEG;
    panel_config.timings.flags.hsync_idle_low = true;
    panel_config.timings.flags.vsync_idle_low = true;
    panel_config.timings.flags.de_idle_high = false;
    bool has_psram = query_psram_once();
    panel_config.flags.fb_in_psram = has_psram;

    if (!has_psram) {
        ESP_LOGE(TAG,
                 "PSRAM requis : activez CONFIG_SPIRAM pour allouer le framebuffer %ux%u RGB16",
                 BOARD_LCD_H_RES,
                 BOARD_LCD_V_RES);
        ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    }

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel_handle, true));
    enable_backlight();
}

static void init_lvgl(void)
{
    lv_init();

    size_t buffer_pixels = (BOARD_LCD_H_RES * BOARD_LCD_V_RES) / 10;
    size_t buffer_bytes = buffer_pixels * sizeof(lv_color_t);

    uint32_t preferred_caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
    uint32_t fallback_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;

    lv_color_t *buf1 = NULL;
    lv_color_t *buf2 = NULL;

    if (query_psram_once()) {
        buf1 = heap_caps_malloc(buffer_bytes, preferred_caps);
        buf2 = heap_caps_malloc(buffer_bytes, preferred_caps);
        if (!buf1 || !buf2) {
            ESP_LOGW(TAG, "Insufficient PSRAM, falling back to internal SRAM buffers");
            if (buf1) {
                heap_caps_free(buf1);
                buf1 = NULL;
            }
            if (buf2) {
                heap_caps_free(buf2);
                buf2 = NULL;
            }
        }
    }

    if (!buf1) {
        buf1 = heap_caps_malloc(buffer_bytes, fallback_caps);
    }
    if (!buf2) {
        buf2 = heap_caps_malloc(buffer_bytes, fallback_caps);
    }
    ESP_ERROR_CHECK(buf1 ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(buf2 ? ESP_OK : ESP_ERR_NO_MEM);

    lv_display_t *display = lv_display_create(BOARD_LCD_H_RES, BOARD_LCD_V_RES);
    lv_display_set_flush_cb(display, lvgl_flush_cb);
    lv_display_set_buffers(display, buf1, buf2, buffer_pixels, LV_DISPLAY_RENDER_MODE_PARTIAL);

    const esp_lcd_rgb_panel_event_callbacks_t callbacks = {
        .on_color_trans_done = rgb_panel_color_trans_cb,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(s_panel_handle, &callbacks, display));

    esp_timer_create_args_t timer_args = {
        .callback = &lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_lvgl_tick_timer, 5000));

    xTaskCreatePinnedToCore(lvgl_task, "lvgl", 8192, NULL, 4, NULL, 1);
}

static void init_touch(void)
{
    gt911_config_t cfg = {
        .i2c_port = BOARD_GT911_I2C_PORT,
        .sda_io = BOARD_GT911_SDA_IO,
        .scl_io = BOARD_GT911_SCL_IO,
        .rst_io = BOARD_GT911_RST_IO,
        .irq_io = BOARD_GT911_IRQ_IO,
        .i2c_clock_hz = BOARD_GT911_I2C_FREQ_HZ,
        .i2c_address = 0x14,
    };

    esp_err_t err = gt911_init(&cfg, &s_touch_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "GT911 init failed: %s", esp_err_to_name(err));
        return;
    }

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lvgl_touch_read_cb);
}

static float get_textarea_float(lv_obj_t *ta)
{
    const char *txt = lv_textarea_get_text(ta);
    if (!txt) {
        return 0.0f;
    }
    char buffer[32];
    size_t len = strlen(txt);
    if (len >= sizeof(buffer)) {
        len = sizeof(buffer) - 1;
    }
    memcpy(buffer, txt, len);
    buffer[len] = '\0';
    for (size_t i = 0; i < len; ++i) {
        if (buffer[i] == ',') {
            buffer[i] = '.';
        }
    }
    return strtof(buffer, NULL);
}

static terrarium_material_t get_material_from_dropdown(lv_obj_t *dd)
{
    uint16_t sel = lv_dropdown_get_selected(dd);
    switch (sel) {
    case 0:
        return TERRARIUM_MATERIAL_WOOD;
    case 1:
        return TERRARIUM_MATERIAL_GLASS;
    case 2:
        return TERRARIUM_MATERIAL_PVC;
    case 3:
        return TERRARIUM_MATERIAL_ACRYLIC;
    default:
        return TERRARIUM_MATERIAL_GLASS;
    }
}

static void update_results(terrarium_ui_ctx_t *ctx, const terrarium_calc_result_t *res)
{
    float led_total_power = res->lighting.led_count * res->lighting.power_per_led_w;

    lv_label_set_text_fmt(
        ctx->heater_label,
        "Surface sol : %.0f cm² (%.2f m²)\n"
        "Tapis visé : %.0f cm² (côté %.1f cm)\n"
        "Puissance brute : %.1f W\n"
        "Catalogue : %.1f W @ %.0f V (I=%.2f A, R=%.1f Ω)",
        res->heating.floor_area_cm2,
        res->heating.floor_area_m2,
        res->heating.heater_target_area_cm2,
        res->heating.heater_side_cm,
        res->heating.heater_power_raw_w,
        res->heating.heater_power_catalog_w,
        res->heating.heater_voltage_v,
        res->heating.heater_current_a,
        res->heating.heater_resistance_ohm);

    lv_label_set_text_fmt(
        ctx->lighting_label,
        "Flux lumineux : %.0f lm\n"
        "Puissance LED : %.1f W\n"
        "Modules requis : %u (%.1f W/unité, %.1f W total)",
        res->lighting.luminous_flux_lm,
        res->lighting.power_w,
        (unsigned)res->lighting.led_count,
        res->lighting.power_per_led_w,
        led_total_power);

    lv_label_set_text_fmt(
        ctx->uv_label,
        "Modules UVA/UVB requis : %u",
        (unsigned)res->uv.module_count);

    lv_label_set_text_fmt(
        ctx->substrate_label,
        "Volume de substrat : %.1f L",
        res->substrate.volume_l);

    lv_label_set_text_fmt(
        ctx->misting_label,
        "Buses de brumisation : %u",
        (unsigned)res->misting.nozzle_count);

    lv_label_set_text(ctx->status_label, "Calcul réalisé ✔️");
}

static void calculate_event_cb(lv_event_t *e)
{
    terrarium_ui_ctx_t *ctx = (terrarium_ui_ctx_t *)lv_event_get_user_data(e);
    terrarium_calc_input_t input = {
        .length_cm = get_textarea_float(ctx->length_cm),
        .width_cm = get_textarea_float(ctx->width_cm),
        .height_cm = get_textarea_float(ctx->height_cm),
        .substrate_thickness_cm = get_textarea_float(ctx->substrate_cm),
        .material = get_material_from_dropdown(ctx->material_dd),
        .target_lux = get_textarea_float(ctx->lux_target),
        .led_efficiency_lm_per_w = get_textarea_float(ctx->led_efficiency),
        .led_power_per_unit_w = get_textarea_float(ctx->led_power),
        .uv_target_intensity = get_textarea_float(ctx->uv_target),
        .uv_module_intensity = get_textarea_float(ctx->uv_module),
        .mist_density_m2_per_nozzle = get_textarea_float(ctx->mist_density),
    };

    terrarium_calc_result_t result = {0};
    if (terrarium_calc_compute(&input, &result)) {
        update_results(ctx, &result);
    } else {
        lv_label_set_text(ctx->status_label, "Entrées invalides – vérifiez les champs.");
    }
}

static lv_obj_t *create_labeled_textarea(lv_obj_t *parent, const char *title, const char *placeholder, const char *default_value)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 300, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(cont, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, LV_PART_MAIN);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, title);

    lv_obj_t *ta = lv_textarea_create(cont);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_placeholder_text(ta, placeholder);
    lv_textarea_set_max_length(ta, 16);
    lv_textarea_set_accepted_chars(ta, "0123456789.,");
    lv_obj_set_width(ta, LV_PCT(100));
    if (default_value) {
        lv_textarea_set_text(ta, default_value);
    }
    return ta;
}

static lv_obj_t *create_labeled_dropdown(lv_obj_t *parent, const char *title, const char *options, uint16_t default_sel)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 300, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(cont, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, LV_PART_MAIN);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, title);

    lv_obj_t *dd = lv_dropdown_create(cont);
    lv_dropdown_set_options(dd, options);
    lv_dropdown_set_selected(dd, default_sel);
    lv_obj_set_width(dd, LV_PCT(100));
    return dd;
}

static void build_ui(terrarium_ui_ctx_t *ctx)
{
    lv_obj_t *root = lv_obj_create(lv_screen_active());
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(root, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(root, 16, LV_PART_MAIN);
    lv_obj_set_scroll_dir(root, LV_DIR_VER);

    lv_obj_t *title = lv_label_create(root);
    lv_label_set_text(title, "TerrariumCalc – ESP32-S3 (Waveshare Touch LCD 7B)");

    lv_obj_t *subtitle = lv_label_create(root);
    lv_label_set_text(subtitle, "Calculez les équipements recommandés à partir des dimensions et contraintes lumineuses.");

    lv_obj_t *inputs_panel = lv_obj_create(root);
    lv_obj_set_size(inputs_panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(inputs_panel, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(inputs_panel, 16, LV_PART_MAIN);
    lv_obj_set_style_radius(inputs_panel, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(inputs_panel, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_bg_color(inputs_panel, lv_palette_main(LV_PALETTE_BLUE_GREY), LV_PART_MAIN);
    lv_obj_set_flex_flow(inputs_panel, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(inputs_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    ctx->length_cm = create_labeled_textarea(inputs_panel, "Longueur (cm)", "Ex: 120", "120");
    ctx->width_cm = create_labeled_textarea(inputs_panel, "Largeur (cm)", "Ex: 60", "60");
    ctx->height_cm = create_labeled_textarea(inputs_panel, "Hauteur (cm)", "Ex: 60", "60");
    ctx->substrate_cm = create_labeled_textarea(inputs_panel, "Épaisseur substrat (cm)", "Ex: 8", "8");
    ctx->lux_target = create_labeled_textarea(inputs_panel, "Cible d'éclairement (lux)", "Ex: 12000", "12000");
    ctx->led_efficiency = create_labeled_textarea(inputs_panel, "Efficacité LED (lm/W)", "Ex: 160", "160");
    ctx->led_power = create_labeled_textarea(inputs_panel, "Puissance par LED (W)", "Ex: 5", "5");
    ctx->uv_target = create_labeled_textarea(inputs_panel, "Intensité UV cible", "Ex: 150", "150");
    ctx->uv_module = create_labeled_textarea(inputs_panel, "Intensité par module UV", "Ex: 75", "75");
    ctx->mist_density = create_labeled_textarea(inputs_panel, "Densité brumisation (m²/buse)", "Ex: 0.10", "0.10");
    ctx->material_dd = create_labeled_dropdown(inputs_panel, "Matériau du plancher", "Bois\nVerre\nPVC\nAcrylique", 1);

    lv_obj_t *button = lv_button_create(root);
    lv_obj_set_height(button, LV_SIZE_CONTENT);
    lv_obj_set_width(button, 240);
    lv_obj_set_style_pad_all(button, 12, LV_PART_MAIN);
    lv_obj_set_style_radius(button, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(button, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
    lv_obj_center(button);

    lv_obj_t *btn_label = lv_label_create(button);
    lv_label_set_text(btn_label, "Calculer");
    lv_obj_center(btn_label);

    lv_obj_t *status = lv_label_create(root);
    lv_label_set_text(status, "Saisissez les paramètres puis appuyez sur Calculer.");
    ctx->status_label = status;

    lv_obj_t *results_panel = lv_obj_create(root);
    lv_obj_set_size(results_panel, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(results_panel, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(results_panel, 12, LV_PART_MAIN);
    lv_obj_set_style_radius(results_panel, 12, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(results_panel, LV_OPA_10, LV_PART_MAIN);
    lv_obj_set_style_bg_color(results_panel, lv_palette_lighten(LV_PALETTE_GREY, 1), LV_PART_MAIN);
    lv_obj_set_flex_flow(results_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(results_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    ctx->heater_label = lv_label_create(results_panel);
    lv_label_set_text(ctx->heater_label, "Tapis chauffant : en attente de calcul.");
    lv_label_set_long_mode(ctx->heater_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ctx->heater_label, LV_PCT(100));

    ctx->lighting_label = lv_label_create(results_panel);
    lv_label_set_text(ctx->lighting_label, "Éclairage LED : en attente de calcul.");
    lv_label_set_long_mode(ctx->lighting_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ctx->lighting_label, LV_PCT(100));

    ctx->uv_label = lv_label_create(results_panel);
    lv_label_set_text(ctx->uv_label, "Modules UV : en attente de calcul.");
    lv_label_set_long_mode(ctx->uv_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ctx->uv_label, LV_PCT(100));

    ctx->substrate_label = lv_label_create(results_panel);
    lv_label_set_text(ctx->substrate_label, "Substrat : en attente de calcul.");
    lv_label_set_long_mode(ctx->substrate_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ctx->substrate_label, LV_PCT(100));

    ctx->misting_label = lv_label_create(results_panel);
    lv_label_set_text(ctx->misting_label, "Brumisation : en attente de calcul.");
    lv_label_set_long_mode(ctx->misting_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ctx->misting_label, LV_PCT(100));

    lv_obj_add_event_cb(button, calculate_event_cb, LV_EVENT_CLICKED, ctx);
}

void app_main(void)
{
    configure_task_wdt();
    init_display();
    init_lvgl();
    init_touch();

    static terrarium_ui_ctx_t ui_ctx = {0};
    build_ui(&ui_ctx);
}

