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
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "soc/soc_caps.h"
#if SOC_PSRAM_SUPPORTED
#include "esp_psram.h"
#endif
#include "lvgl.h"

#include "board_waveshare_7b.h"
#include "calc_heating_cable.h"
#include "calc_heating_pad.h"
#include "calc_lighting.h"
#include "calc_misting.h"
#include "calc_substrate.h"
#include "gt911/gt911.h"
#include "storage.h"
#include "ui_main.h"

__attribute__((weak)) void board_ch422g_enable(void) {}

static const char *TAG = "app";

static esp_lcd_panel_handle_t s_panel_handle;
static esp_timer_handle_t s_lvgl_tick_timer;
static gt911_handle_t s_touch_handle;
static TaskHandle_t s_lvgl_task_handle;
static lv_display_t *s_display;
static lv_indev_t *s_touch_indev;
static bool s_lvgl_task_wdt_registered;
#if SOC_PSRAM_SUPPORTED
static bool s_psram_available;
#endif

typedef struct {
    lv_color_t *buf1;
    lv_color_t *buf2;
} lvgl_buffers_t;

static lvgl_buffers_t s_lvgl_buffers;

static bool query_psram_once(void);
static esp_err_t init_touch(void);
static void deinit_touch(void);
static esp_err_t start_lvgl_task(void);
static void enter_safe_fault_state(const char *stage, esp_err_t err);

static void enter_safe_fault_state(const char *stage, esp_err_t err)
{
    if (stage) {
        if (err == ESP_OK) {
            ESP_LOGE(TAG, "Arrêt dans l'état %s sans code d'erreur explicite", stage);
        } else {
            ESP_LOGE(TAG, "Echec critique pendant %s: %s (0x%x)", stage, esp_err_to_name(err), (unsigned)err);
        }
    }
#if SOC_PSRAM_SUPPORTED
    if (!query_psram_once()) {
        ESP_LOGW(TAG,
                 "PSRAM indisponible. Vérifiez le câblage / l'activation et recompiliez sans CONFIG_SPIRAM si la carte n'en possède pas.");
    }
#endif
#if CONFIG_ESP_TASK_WDT_INIT
    esp_err_t wdt_err = esp_task_wdt_deinit();
    if (wdt_err != ESP_OK && wdt_err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "Impossible de désactiver le Task WDT: %s", esp_err_to_name(wdt_err));
    } else {
        ESP_LOGI(TAG, "Task WDT désactivé pour conserver la console série stable");
    }
#endif
    ESP_LOGE(TAG, "Système en pause. Corrige la cause puis redémarre l'ESP32-S3.");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#if SOC_PSRAM_SUPPORTED
static bool query_psram_once(void)
{
    static bool s_checked;
    if (!s_checked) {
        bool initialised = esp_psram_is_initialized();
        if (!initialised) {
            esp_err_t init_err = esp_psram_init();
            if (init_err == ESP_OK) {
                initialised = esp_psram_is_initialized();
            } else if (init_err == ESP_ERR_INVALID_STATE) {
                initialised = esp_psram_is_initialized();
                ESP_LOGI(TAG, "PSRAM already initialised by bootloader");
            } else {
                ESP_LOGW(TAG, "Failed to initialise PSRAM: %s", esp_err_to_name(init_err));
            }
        }

        size_t psram_bytes = 0;
        if (initialised) {
            psram_bytes = esp_psram_get_size();
            if (psram_bytes > 0) {
                s_psram_available = true;
                ESP_LOGI(TAG, "PSRAM ready: %u KB detected", (unsigned)(psram_bytes / 1024));
            } else {
                ESP_LOGW(TAG, "PSRAM reports zero size despite initialisation");
                s_psram_available = false;
            }
        } else {
            ESP_LOGW(TAG, "PSRAM not initialised; CONFIG_SPIRAM may be disabled");
            s_psram_available = false;
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
    esp_err_t err = esp_task_wdt_reconfigure(&config);
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
    if (!s_touch_handle.initialized) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
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
    s_lvgl_task_handle = xTaskGetCurrentTaskHandle();
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

static esp_err_t init_display(void)
{
    board_ch422g_enable();
    init_backlight();

    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_width = BOARD_LCD_DATA_WIDTH,
        .dma_burst_size = 64,
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
        ESP_LOGE(TAG, "PSRAM non initialisée : impossible d'allouer le framebuffer %ux%u RGB16", BOARD_LCD_H_RES, BOARD_LCD_V_RES);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Initialising RGB panel with framebuffer in PSRAM (%ux%u RGB16)", BOARD_LCD_H_RES, BOARD_LCD_V_RES);

    esp_err_t err = esp_lcd_new_rgb_panel(&panel_config, &s_panel_handle);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_lcd_panel_reset(s_panel_handle);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_lcd_panel_init(s_panel_handle);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_lcd_panel_disp_on_off(s_panel_handle, true);
    if (err != ESP_OK) {
        return err;
    }
    enable_backlight();
    return ESP_OK;
}

static void deinit_display(void)
{
    if (s_panel_handle) {
        esp_lcd_panel_disp_on_off(s_panel_handle, false);
        esp_lcd_panel_del(s_panel_handle);
        s_panel_handle = NULL;
    }
    if (BOARD_LCD_PIN_BACKLIGHT != GPIO_NUM_NC) {
        gpio_set_level(BOARD_LCD_PIN_BACKLIGHT, BOARD_LCD_BK_LIGHT_OFF_LEVEL);
    }
}

static esp_err_t init_lvgl(void)
{
    lv_init();

    size_t buffer_pixels = (BOARD_LCD_H_RES * BOARD_LCD_V_RES) / 10;
    size_t buffer_bytes = buffer_pixels * sizeof(lv_color_t);

    uint32_t preferred_caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
    uint32_t fallback_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;

    lv_color_t *buf1 = NULL;
    lv_color_t *buf2 = NULL;
    bool buffers_in_psram = false;

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
        } else {
            buffers_in_psram = true;
        }
    }

    if (!buf1) {
        buf1 = heap_caps_malloc(buffer_bytes, fallback_caps);
    }
    if (!buf2) {
        buf2 = heap_caps_malloc(buffer_bytes, fallback_caps);
    }
    if (!buf1 || !buf2) {
        heap_caps_free(buf1);
        heap_caps_free(buf2);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "LVGL draw buffers: %zu bytes each allocated in %s", buffer_bytes, buffers_in_psram ? "PSRAM" : "internal SRAM");

    s_display = lv_display_create(BOARD_LCD_H_RES, BOARD_LCD_V_RES);
    lv_display_set_color_format(s_display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(s_display, lvgl_flush_cb);
    lv_display_set_buffers(s_display, buf1, buf2, buffer_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    s_lvgl_buffers.buf1 = buf1;
    s_lvgl_buffers.buf2 = buf2;

    const esp_lcd_rgb_panel_event_callbacks_t callbacks = {
        .on_color_trans_done = rgb_panel_color_trans_cb,
    };
    esp_err_t err = esp_lcd_rgb_panel_register_event_callbacks(s_panel_handle, &callbacks, s_display);
    if (err != ESP_OK) {
        return err;
    }

    esp_timer_create_args_t timer_args = {
        .callback = &lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    err = esp_timer_create(&timer_args, &s_lvgl_tick_timer);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_timer_start_periodic(s_lvgl_tick_timer, 5000);
    if (err != ESP_OK) {
        return err;
    }

    return ESP_OK;
}

static esp_err_t start_lvgl_task(void)
{
    if (s_lvgl_task_handle) {
        return ESP_OK;
    }
    BaseType_t created = xTaskCreatePinnedToCore(lvgl_task, "lvgl", 8192, NULL, 4, &s_lvgl_task_handle, 1);
    if (created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LVGL task");
        s_lvgl_task_handle = NULL;
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void deinit_lvgl(void)
{
    deinit_touch();
    TaskHandle_t task = s_lvgl_task_handle;
    if (s_lvgl_task_wdt_registered) {
        esp_task_wdt_delete(task);
        s_lvgl_task_wdt_registered = false;
    }
    if (task) {
        vTaskDelete(task);
    }
    if (s_lvgl_tick_timer) {
        esp_timer_stop(s_lvgl_tick_timer);
        esp_timer_delete(s_lvgl_tick_timer);
        s_lvgl_tick_timer = NULL;
    }
    if (s_touch_indev) {
        lv_indev_delete(s_touch_indev);
        s_touch_indev = NULL;
    }
    if (s_display) {
        lv_display_delete(s_display);
        s_display = NULL;
    }
    if (s_lvgl_buffers.buf1) {
        heap_caps_free(s_lvgl_buffers.buf1);
        s_lvgl_buffers.buf1 = NULL;
    }
    if (s_lvgl_buffers.buf2) {
        heap_caps_free(s_lvgl_buffers.buf2);
        s_lvgl_buffers.buf2 = NULL;
    }
    s_lvgl_task_handle = NULL;
}

static esp_err_t init_touch(void)
{
    gt911_config_t cfg = {
        .i2c_port = BOARD_GT911_I2C_PORT,
        .sda_io = BOARD_GT911_SDA_IO,
        .scl_io = BOARD_GT911_SCL_IO,
        .rst_io = BOARD_GT911_RST_IO,
        .irq_io = BOARD_GT911_IRQ_IO,
        .i2c_clock_hz = BOARD_GT911_I2C_FREQ_HZ,
        .i2c_address = BOARD_GT911_I2C_ADDR,
        .logical_max_x = BOARD_LCD_H_RES,
        .logical_max_y = BOARD_LCD_V_RES,
        .invert_x = false,
        .invert_y = true,
        .swap_xy = false,
    };

    esp_err_t err = gt911_init(&cfg, &s_touch_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "GT911 init failed: %s", esp_err_to_name(err));
        return err;
    }

    s_touch_indev = lv_indev_create();
    if (!s_touch_indev) {
        gt911_deinit(&s_touch_handle);
        s_touch_handle.initialized = false;
        ESP_LOGE(TAG, "Failed to allocate LVGL input device");
        return ESP_ERR_NO_MEM;
    }
    lv_indev_set_type(s_touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_touch_indev, lvgl_touch_read_cb);
    if (s_display) {
        lv_indev_set_display(s_touch_indev, s_display);
    }
    return ESP_OK;
}

static void deinit_touch(void)
{
    if (s_touch_handle.initialized) {
        gt911_deinit(&s_touch_handle);
    }
}

static void run_self_tests(void)
{
    heating_pad_run_self_test();
    heating_cable_run_self_test();
    lighting_run_self_test();
    substrate_run_self_test();
    misting_run_self_test();
}

void app_main(void)
{
    ESP_ERROR_CHECK(storage_init());
    esp_err_t err = init_display();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Display init failed: %s", esp_err_to_name(err));
        deinit_display();
        enter_safe_fault_state("initialisation de l'écran", err);
    }

    err = init_lvgl();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LVGL init failed: %s", esp_err_to_name(err));
        deinit_lvgl();
        deinit_display();
        enter_safe_fault_state("initialisation LVGL", err);
    }

    configure_task_wdt();
    ui_main_init();

    err = init_touch();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Touch controller unavailable (%s)", esp_err_to_name(err));
    }

    err = start_lvgl_task();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Unable to start LVGL task (%s)", esp_err_to_name(err));
        deinit_lvgl();
        deinit_display();
        enter_safe_fault_state("démarrage de la tache LVGL", err);
    }

    run_self_tests();
}

