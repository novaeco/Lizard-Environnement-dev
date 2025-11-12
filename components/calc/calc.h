#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TERRARIUM_MATERIAL_WOOD = 0,
    TERRARIUM_MATERIAL_GLASS,
    TERRARIUM_MATERIAL_PVC,
    TERRARIUM_MATERIAL_ACRYLIC,
    TERRARIUM_MATERIAL_COUNT
} terrarium_material_t;

typedef struct {
    float length_cm;
    float width_cm;
    float height_cm;
    float substrate_thickness_cm;
    terrarium_material_t material;
    float target_lux;
    float led_efficiency_lm_per_w;
    float led_power_per_unit_w;
    float uv_target_intensity;
    float uv_module_intensity;
    float mist_density_m2_per_nozzle;
} terrarium_calc_input_t;

typedef struct {
    float floor_area_cm2;
    float floor_area_m2;
    float heater_target_area_cm2;
    float heater_side_cm;
    float heater_power_raw_w;
    float heater_power_catalog_w;
    float heater_voltage_v;
    float heater_current_a;
    float heater_resistance_ohm;
} terrarium_heating_result_t;

typedef struct {
    float luminous_flux_lm;
    float power_w;
    uint32_t led_count;
    float power_per_led_w;
} terrarium_led_result_t;

typedef struct {
    uint32_t module_count;
} terrarium_uv_result_t;

typedef struct {
    float volume_l;
} terrarium_substrate_result_t;

typedef struct {
    uint32_t nozzle_count;
} terrarium_misting_result_t;

typedef struct {
    terrarium_heating_result_t heating;
    terrarium_led_result_t lighting;
    terrarium_uv_result_t uv;
    terrarium_substrate_result_t substrate;
    terrarium_misting_result_t misting;
} terrarium_calc_result_t;

bool terrarium_calc_compute(const terrarium_calc_input_t *input, terrarium_calc_result_t *result);

#ifdef __cplusplus
}
#endif

