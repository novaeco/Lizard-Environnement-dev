#pragma once

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration statique du contrôleur GT911.
 */
typedef struct {
    i2c_port_t i2c_port;        /*!< Port I²C maître utilisé. */
    gpio_num_t sda_io;          /*!< Broche SDA (pull-up externe recommandé). */
    gpio_num_t scl_io;          /*!< Broche SCL. */
    gpio_num_t rst_io;          /*!< Broche reset (GPIO_NUM_NC si non câblé). */
    gpio_num_t irq_io;          /*!< Broche d'interruption (GPIO_NUM_NC si non câblé). */
    uint32_t i2c_clock_hz;      /*!< Fréquence bus I²C (jusqu'à 1 MHz selon câblage). */
    uint8_t i2c_address;        /*!< Adresse 7 bits (0x14 ou 0x5D). */
    uint16_t logical_max_x;     /*!< Résolution logique X pour LVGL. */
    uint16_t logical_max_y;     /*!< Résolution logique Y pour LVGL. */
    bool invert_x;              /*!< Inverse l'axe X si true. */
    bool invert_y;              /*!< Inverse l'axe Y si true. */
    bool swap_xy;               /*!< Échange X/Y (rotation 90°). */
} gt911_config_t;

typedef struct {
    i2c_port_t i2c_port;
    gpio_num_t sda_io;
    gpio_num_t scl_io;
    gpio_num_t rst_io;
    gpio_num_t irq_io;
    uint8_t i2c_address;
    uint16_t logical_max_x;
    uint16_t logical_max_y;
    bool invert_x;
    bool invert_y;
    bool swap_xy;
    bool driver_owned;
    bool initialized;
} gt911_handle_t;

esp_err_t gt911_init(const gt911_config_t *config, gt911_handle_t *handle);
esp_err_t gt911_read_touch(gt911_handle_t *handle, uint16_t *x, uint16_t *y, bool *touched);
esp_err_t gt911_deinit(gt911_handle_t *handle);

#ifdef __cplusplus
}
#endif

