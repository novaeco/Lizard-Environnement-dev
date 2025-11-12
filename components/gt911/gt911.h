#pragma once

#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    i2c_port_t i2c_port;
    gpio_num_t sda_io;
    gpio_num_t scl_io;
    gpio_num_t rst_io;
    gpio_num_t irq_io;
    uint32_t i2c_clock_hz;
    uint8_t i2c_address;
} gt911_config_t;

typedef struct {
    i2c_port_t i2c_port;
    gpio_num_t irq_io;
    uint8_t i2c_address;
    bool initialized;
} gt911_handle_t;

esp_err_t gt911_init(const gt911_config_t *config, gt911_handle_t *handle);
esp_err_t gt911_read_touch(gt911_handle_t *handle, uint16_t *x, uint16_t *y, bool *touched);

#ifdef __cplusplus
}
#endif

