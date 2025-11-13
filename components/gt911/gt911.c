#include "gt911/gt911.h"

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define GT911_DEFAULT_ADDR 0x14
#define GT911_STATUS_REG 0x814E
#define GT911_FIRST_POINT_REG 0x8150
#define GT911_POINT_STRIDE 8

static const char *TAG = "gt911";

static uint16_t clamp16(uint16_t value, uint16_t max)
{
    if (max == 0) {
        return value;
    }
    if (value >= max) {
        return max - 1;
    }
    return value;
}

static esp_err_t gt911_i2c_write(gt911_handle_t *handle, uint16_t reg, const uint8_t *data, size_t len)
{
    if (!handle || !data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_RETURN_ON_FALSE(cmd, ESP_ERR_NO_MEM, TAG, "Failed to allocate I2C command");

    esp_err_t err = i2c_master_start(cmd);
    if (err == ESP_OK) {
        err = i2c_master_write_byte(cmd, (handle->i2c_address << 1) | I2C_MASTER_WRITE, true);
    }
    if (err == ESP_OK) {
        err = i2c_master_write_byte(cmd, (reg >> 8) & 0xFF, true);
    }
    if (err == ESP_OK) {
        err = i2c_master_write_byte(cmd, reg & 0xFF, true);
    }
    if (err == ESP_OK) {
        err = i2c_master_write(cmd, data, len, true);
    }
    if (err == ESP_OK) {
        err = i2c_master_stop(cmd);
    }

    if (err == ESP_OK) {
        err = i2c_master_cmd_begin(handle->i2c_port, cmd, pdMS_TO_TICKS(50));
    }

    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t gt911_i2c_read(gt911_handle_t *handle, uint16_t reg, uint8_t *data, size_t len)
{
    if (!handle || !data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_RETURN_ON_FALSE(cmd, ESP_ERR_NO_MEM, TAG, "Failed to allocate I2C command");

    esp_err_t err = i2c_master_start(cmd);
    if (err == ESP_OK) {
        err = i2c_master_write_byte(cmd, (handle->i2c_address << 1) | I2C_MASTER_WRITE, true);
    }
    if (err == ESP_OK) {
        err = i2c_master_write_byte(cmd, (reg >> 8) & 0xFF, true);
    }
    if (err == ESP_OK) {
        err = i2c_master_write_byte(cmd, reg & 0xFF, true);
    }
    if (err == ESP_OK) {
        err = i2c_master_start(cmd);
    }
    if (err == ESP_OK) {
        err = i2c_master_write_byte(cmd, (handle->i2c_address << 1) | I2C_MASTER_READ, true);
    }
    if (err == ESP_OK) {
        if (len > 1) {
            err = i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
        }
    }
    if (err == ESP_OK) {
        err = i2c_master_read_byte(cmd, &data[len - 1], I2C_MASTER_NACK);
    }
    if (err == ESP_OK) {
        err = i2c_master_stop(cmd);
    }

    if (err == ESP_OK) {
        err = i2c_master_cmd_begin(handle->i2c_port, cmd, pdMS_TO_TICKS(50));
    }

    i2c_cmd_link_delete(cmd);
    return err;
}

esp_err_t gt911_init(const gt911_config_t *config, gt911_handle_t *handle)
{
    if (!config || !handle) {
        return ESP_ERR_INVALID_ARG;
    }

    gt911_handle_t temp = {
        .i2c_port = config->i2c_port,
        .irq_io = config->irq_io,
        .i2c_address = config->i2c_address ? config->i2c_address : GT911_DEFAULT_ADDR,
        .logical_max_x = config->logical_max_x,
        .logical_max_y = config->logical_max_y,
        .invert_x = config->invert_x,
        .invert_y = config->invert_y,
        .swap_xy = config->swap_xy,
        .driver_owned = false,
        .initialized = false,
    };

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = config->sda_io,
        .scl_io_num = config->scl_io,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = config->i2c_clock_hz ? config->i2c_clock_hz : 400000,
    };

    esp_err_t err = i2c_param_config(config->i2c_port, &i2c_conf);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(config->i2c_port, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(err));
        return err;
    }
    bool driver_owned = (err == ESP_OK);

    if (config->rst_io != GPIO_NUM_NC) {
        gpio_config_t rst_conf = {
            .pin_bit_mask = BIT64(config->rst_io),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&rst_conf), TAG, "RST GPIO config failed");
        gpio_set_level(config->rst_io, 0);
        vTaskDelay(pdMS_TO_TICKS(15));
        gpio_set_level(config->rst_io, 1);
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    if (config->irq_io != GPIO_NUM_NC) {
        gpio_config_t irq_conf = {
            .pin_bit_mask = BIT64(config->irq_io),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_RETURN_ON_ERROR(gpio_config(&irq_conf), TAG, "IRQ GPIO config failed");
    }

    uint8_t product_id[4] = {0};
    esp_err_t pid_err = gt911_i2c_read(&temp, 0x8140, product_id, sizeof(product_id));
    if (pid_err == ESP_OK) {
        ESP_LOGI(TAG, "GT911 product ID: %c%c%c%c", product_id[0], product_id[1], product_id[2], product_id[3]);
    } else {
        ESP_LOGW(TAG, "Unable to read GT911 product ID (%s)", esp_err_to_name(pid_err));
    }

    temp.logical_max_x = config->logical_max_x ? config->logical_max_x : 1024;
    temp.logical_max_y = config->logical_max_y ? config->logical_max_y : 600;
    temp.invert_x = config->invert_x;
    temp.invert_y = config->invert_y;
    temp.swap_xy = config->swap_xy;
    temp.driver_owned = driver_owned;
    temp.initialized = true;
    *handle = temp;
    return ESP_OK;
}

esp_err_t gt911_read_touch(gt911_handle_t *handle, uint16_t *x, uint16_t *y, bool *touched)
{
    if (!handle || !handle->initialized || !x || !y || !touched) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t status = 0;
    esp_err_t err = gt911_i2c_read(handle, GT911_STATUS_REG, &status, 1);
    if (err != ESP_OK) {
        return err;
    }

    if ((status & 0x80U) == 0U) {
        *touched = false;
        return ESP_OK;
    }

    uint8_t points = status & 0x0FU;
    if (points == 0U) {
        *touched = false;
    } else {
        uint8_t buffer[GT911_POINT_STRIDE] = {0};
        err = gt911_i2c_read(handle, GT911_FIRST_POINT_REG, buffer, sizeof(buffer));
        if (err != ESP_OK) {
            return err;
        }
        uint16_t raw_x = (uint16_t)buffer[1] << 8 | buffer[0];
        uint16_t raw_y = (uint16_t)buffer[3] << 8 | buffer[2];

        if (handle->swap_xy) {
            uint16_t tmp = raw_x;
            raw_x = raw_y;
            raw_y = tmp;
        }

        raw_x = clamp16(raw_x, handle->logical_max_x);
        raw_y = clamp16(raw_y, handle->logical_max_y);

        if (handle->invert_x && handle->logical_max_x > 0) {
            raw_x = handle->logical_max_x - 1 - raw_x;
        }
        if (handle->invert_y && handle->logical_max_y > 0) {
            raw_y = handle->logical_max_y - 1 - raw_y;
        }

        *x = raw_x;
        *y = raw_y;
        *touched = true;
    }

    uint8_t clear = 0;
    esp_err_t clear_err = gt911_i2c_write(handle, GT911_STATUS_REG, &clear, 1);
    if (clear_err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to clear GT911 status: %s", esp_err_to_name(clear_err));
        return clear_err;
    }
    return ESP_OK;
}

esp_err_t gt911_deinit(gt911_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->driver_owned) {
        esp_err_t err = i2c_driver_delete(handle->i2c_port);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "I2C driver delete failed: %s", esp_err_to_name(err));
        }
    }

    handle->initialized = false;
    handle->driver_owned = false;
    return ESP_OK;
}

