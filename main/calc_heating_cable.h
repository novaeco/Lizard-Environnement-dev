#pragma once

#include "calc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float length_cm;
    float depth_cm;
    terrarium_material_t material;
    float heated_ratio;
    float power_linear_w_per_m;
    float supply_voltage_v;
    float target_power_density_w_per_cm2;
    float spacing_cm;
} heating_cable_input_t;

typedef struct {
    bool valid;
    float heated_area_cm2;
    float target_power_w;
    float recommended_length_m;
    float resulting_density_w_per_cm2;
    float spacing_cm;
    float estimated_current_a;
    float estimated_resistance_ohm;
    bool warning_density_high;
    bool warning_spacing_too_tight;
    bool warning_high_voltage;
    bool warning_density_high;
} heating_cable_result_t;

bool heating_cable_calculate(const heating_cable_input_t *in, heating_cable_result_t *out);
void heating_cable_run_self_test(void);

#ifdef __cplusplus
}
#endif

