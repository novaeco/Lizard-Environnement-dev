#pragma once

#include "esp_err.h"
#include "driver/i2c.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    i2c_port_t port;
    uint8_t address;
    int irq_gpio;
} gt911_config_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    bool touched;
} gt911_touch_point_t;

esp_err_t gt911_init(const gt911_config_t *config);
esp_err_t gt911_read_touch(gt911_touch_point_t *point);
