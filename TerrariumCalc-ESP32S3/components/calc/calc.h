#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CALC_MATERIAL_WOOD = 0,
    CALC_MATERIAL_GLASS,
    CALC_MATERIAL_PVC,
    CALC_MATERIAL_ACRYLIC,
    CALC_MATERIAL_COUNT
} calc_material_t;

typedef struct {
    float length_cm;
    float width_cm;
    float height_cm;
    float substrate_thickness_cm;
    float floor_heater_power_density_w_per_cm2;
    float led_target_lux;
    float led_efficiency_lm_per_w;
    float led_unit_power_w;
    float uv_target_intensity;
    float uv_module_intensity;
    float mist_nozzle_density_m2;
    calc_material_t floor_material;
} calc_inputs_t;

typedef struct {
    float area_floor_cm2;
    float heater_target_area_cm2;
    float heater_square_side_cm;
    float heater_corrected_area_cm2;
    float heater_power_w;
    float heater_voltage_v;
    float heater_current_a;
    float heater_resistance_ohm;
    unsigned heater_catalog_power_w;

    float led_area_m2;
    float led_luminous_flux_lm;
    float led_required_power_w;
    unsigned led_count;
    float led_total_power_w;

    unsigned uv_module_count;

    float substrate_volume_l;

    unsigned mist_nozzle_count;
} calc_results_t;

void calc_get_default_inputs(calc_inputs_t *inputs);
void calc_compute(const calc_inputs_t *inputs, calc_results_t *results);

const char *calc_material_to_string(calc_material_t material);
float calc_material_coefficient(calc_material_t material);

#ifdef __cplusplus
}
#endif
