#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"
#include "lvgl.h"

#define LCD_H_RES               800
#define LCD_V_RES               480
#define LCD_RGB_DATA_WIDTH      16
#define LCD_PIXEL_CLOCK_HZ      (18 * 1000 * 1000)
#define LVGL_TICK_PERIOD_MS     2
#define LVGL_BUFFER_HEIGHT      40

#define LCD_PIN_HSYNC           (GPIO_NUM_39)
#define LCD_PIN_VSYNC           (GPIO_NUM_41)
#define LCD_PIN_DE              (GPIO_NUM_40)
#define LCD_PIN_PCLK            (GPIO_NUM_42)
#define LCD_PIN_DATA0           (GPIO_NUM_8)
#define LCD_PIN_DATA1           (GPIO_NUM_3)
#define LCD_PIN_DATA2           (GPIO_NUM_46)
#define LCD_PIN_DATA3           (GPIO_NUM_9)
#define LCD_PIN_DATA4           (GPIO_NUM_1)
#define LCD_PIN_DATA5           (GPIO_NUM_5)
#define LCD_PIN_DATA6           (GPIO_NUM_6)
#define LCD_PIN_DATA7           (GPIO_NUM_7)
#define LCD_PIN_DATA8           (GPIO_NUM_15)
#define LCD_PIN_DATA9           (GPIO_NUM_16)
#define LCD_PIN_DATA10          (GPIO_NUM_4)
#define LCD_PIN_DATA11          (GPIO_NUM_45)
#define LCD_PIN_DATA12          (GPIO_NUM_48)
#define LCD_PIN_DATA13          (GPIO_NUM_47)
#define LCD_PIN_DATA14          (GPIO_NUM_21)
#define LCD_PIN_DATA15          (GPIO_NUM_14)
#define LCD_PIN_DISP_EN         (GPIO_NUM_38)

static const char *TAG = "terrarium";

static void lvgl_tick(void *arg)
{
    (void) arg;
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel = lv_display_get_driver_data(disp);
    if (!panel) {
        lv_display_flush_ready(disp);
        return;
    }

    const lv_color_t *color_map = (const lv_color_t *)px_map;
    esp_err_t err = esp_lcd_panel_draw_bitmap(panel, area->x1, area->y1,
                                              area->x2 + 1, area->y2 + 1,
                                              (void *)color_map);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to draw bitmap (%s)", esp_err_to_name(err));
    }

    lv_display_flush_ready(disp);
}

static bool rgb_panel_color_trans_done_cb(esp_lcd_panel_handle_t panel,
                                          const esp_lcd_rgb_panel_event_data_t *event_data,
                                          void *user_ctx)
{
    (void) panel;
    (void) event_data;
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static esp_lcd_panel_handle_t lcd_init_rgb_panel(void)
{
    esp_lcd_panel_handle_t panel = NULL;

    const esp_lcd_rgb_timing_t panel_timing = {
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .h_res = LCD_H_RES,
        .v_res = LCD_V_RES,
        .hsync_pulse_width = 4,
        .hsync_back_porch = 40,
        .hsync_front_porch = 8,
        .vsync_pulse_width = 4,
        .vsync_back_porch = 20,
        .vsync_front_porch = 8,
        .flags = {
            .hsync_idle_low = true,
            .vsync_idle_low = true,
            .de_idle_high = true,
            .pclk_active_neg = true,
            .pclk_idle_high = false,
        },
    };

    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = LCD_RGB_DATA_WIDTH,
        .psram_trans_align = 64,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = panel_timing,
        .hsync_gpio_num = LCD_PIN_HSYNC,
        .vsync_gpio_num = LCD_PIN_VSYNC,
        .de_gpio_num = LCD_PIN_DE,
        .pclk_gpio_num = LCD_PIN_PCLK,
        .data_gpio_nums = {
            LCD_PIN_DATA0,
            LCD_PIN_DATA1,
            LCD_PIN_DATA2,
            LCD_PIN_DATA3,
            LCD_PIN_DATA4,
            LCD_PIN_DATA5,
            LCD_PIN_DATA6,
            LCD_PIN_DATA7,
            LCD_PIN_DATA8,
            LCD_PIN_DATA9,
            LCD_PIN_DATA10,
            LCD_PIN_DATA11,
            LCD_PIN_DATA12,
            LCD_PIN_DATA13,
            LCD_PIN_DATA14,
            LCD_PIN_DATA15,
        },
        .disp_gpio_num = LCD_PIN_DISP_EN,
        .flags = {
            .fb_in_psram = 1,
            .refresh_on_demand = 0,
        },
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel));

    if (panel_config.disp_gpio_num >= 0) {
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
    }

    return panel;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initialising LVGL %d.%d.%d", LV_VERSION_MAJOR, LV_VERSION_MINOR, LV_VERSION_PATCH);

    lv_init();

    esp_lcd_panel_handle_t panel_handle = lcd_init_rgb_panel();

    static lv_color_t lvgl_buf1[LCD_H_RES * LVGL_BUFFER_HEIGHT];
    static lv_color_t lvgl_buf2[LCD_H_RES * LVGL_BUFFER_HEIGHT];

    lv_display_t *disp = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_driver_data(disp, panel_handle);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_draw_buffers(disp, lvgl_buf1, lvgl_buf2,
                                sizeof(lvgl_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_color_trans_done = rgb_panel_color_trans_done_cb,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, disp));

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lvgl_tick,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lvgl_tick",
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LVGL_TICK_PERIOD_MS * 1000));

    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Terrarium Calculator ready");
    lv_obj_center(label);

    while (true) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
