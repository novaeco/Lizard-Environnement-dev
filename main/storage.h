#pragma once

#include "calc_heating_cable.h"
#include "calc_heating_pad.h"
#include "calc_lighting.h"
#include "calc_substrate.h"
#include "calc_misting.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t storage_init(void);

esp_err_t storage_load_heating_pad(heating_pad_input_t *in);
esp_err_t storage_save_heating_pad(const heating_pad_input_t *in);

esp_err_t storage_load_heating_cable(heating_cable_input_t *in);
esp_err_t storage_save_heating_cable(const heating_cable_input_t *in);

esp_err_t storage_load_lighting(lighting_input_t *in);
esp_err_t storage_save_lighting(const lighting_input_t *in);

esp_err_t storage_load_substrate(substrate_input_t *in);
esp_err_t storage_save_substrate(const substrate_input_t *in);

esp_err_t storage_load_misting(misting_input_t *in);
esp_err_t storage_save_misting(const misting_input_t *in);

#ifdef __cplusplus
}
#endif

