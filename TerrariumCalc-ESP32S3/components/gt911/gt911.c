#include "gt911.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define GT911_REG_COMMAND          0x8040
#define GT911_REG_STATUS           0x814E
#define GT911_REG_FIRST_POINT      0x8150
#define GT911_REG_PRODUCT_ID       0x8140
#define GT911_MAX_POINTS           5

#define GT911_CMD_READ_COORD       0x00
#define GT911_CMD_SOFT_RESET       0x02

#define GT911_DEFAULT_ADDR         0x5D

static const char *TAG = "gt911";

static gt911_config_t s_cfg = {
    .port = I2C_NUM_MAX,
    .address = GT911_DEFAULT_ADDR,
    .irq_gpio = -1,
};

static esp_err_t gt911_write(uint16_t reg, const uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t err = i2c_master_start(cmd);
    if (err == ESP_OK) {
        err = i2c_master_write_byte(cmd, (s_cfg.address << 1) | I2C_MASTER_WRITE, true);
    }
    if (err == ESP_OK) {
        uint8_t header[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
        err = i2c_master_write(cmd, header, sizeof(header), true);
    }
    if (err == ESP_OK && data && len) {
        err = i2c_master_write(cmd, data, len, true);
    }
    if (err == ESP_OK) {
        err = i2c_master_stop(cmd);
    }
    esp_err_t result = i2c_master_cmd_begin(s_cfg.port, cmd, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(cmd);
    return (err == ESP_OK) ? result : err;
}

static esp_err_t gt911_read(uint16_t reg, uint8_t *data, size_t len)
{
    if (!data || !len) {
        return ESP_ERR_INVALID_ARG;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t err = i2c_master_start(cmd);
    if (err == ESP_OK) {
        err = i2c_master_write_byte(cmd, (s_cfg.address << 1) | I2C_MASTER_WRITE, true);
    }
    if (err == ESP_OK) {
        uint8_t header[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
        err = i2c_master_write(cmd, header, sizeof(header), true);
    }
    if (err == ESP_OK) {
        err = i2c_master_start(cmd);
    }
    if (err == ESP_OK) {
        err = i2c_master_write_byte(cmd, (s_cfg.address << 1) | I2C_MASTER_READ, true);
    }
    if (err == ESP_OK) {
        err = i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    }
    if (err == ESP_OK) {
        err = i2c_master_stop(cmd);
    }
    esp_err_t result = i2c_master_cmd_begin(s_cfg.port, cmd, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(cmd);
    return (err == ESP_OK) ? result : err;
}

esp_err_t gt911_init(const gt911_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    s_cfg = *config;
    if (s_cfg.address == 0) {
        s_cfg.address = GT911_DEFAULT_ADDR;
    }

    if (s_cfg.port >= I2C_NUM_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_cfg.irq_gpio >= 0) {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << s_cfg.irq_gpio,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf));
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t product_id[4] = {0};
    esp_err_t err = gt911_read(GT911_REG_PRODUCT_ID, product_id, sizeof(product_id));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "GT911 Product ID: %.*s", (int)sizeof(product_id), (char *)product_id);
    } else {
        ESP_LOGW(TAG, "Failed to read GT911 product ID (%s)", esp_err_to_name(err));
    }

    uint8_t command = GT911_CMD_READ_COORD;
    ESP_RETURN_ON_ERROR(gt911_write(GT911_REG_COMMAND, &command, 1), TAG, "command");

    return ESP_OK;
}

esp_err_t gt911_read_touch(gt911_touch_point_t *point)
{
    if (!point) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(point, 0, sizeof(*point));

    uint8_t status = 0;
    esp_err_t err = gt911_read(GT911_REG_STATUS, &status, 1);
    if (err != ESP_OK) {
        return err;
    }

    uint8_t touch_count = status & 0x0F;
    bool data_ready = status & 0x80;

    if (!data_ready || touch_count == 0 || touch_count > GT911_MAX_POINTS) {
        uint8_t clear = 0;
        gt911_write(GT911_REG_STATUS, &clear, 1);
        return ESP_OK;
    }

    uint8_t buf[8] = {0};
    err = gt911_read(GT911_REG_FIRST_POINT, buf, sizeof(buf));
    if (err != ESP_OK) {
        return err;
    }

    uint16_t x = ((uint16_t)buf[1] << 8) | buf[0];
    uint16_t y = ((uint16_t)buf[3] << 8) | buf[2];

    point->x = x;
    point->y = y;
    point->touched = true;

    uint8_t clear = 0;
    gt911_write(GT911_REG_STATUS, &clear, 1);

    return ESP_OK;
}
