#pragma once

#include "calc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float length_cm;
    float depth_cm;
    float height_cm;
    terrarium_material_t material;
    float heated_ratio;
} heating_pad_input_t;

typedef struct {
    bool valid;
    float floor_area_cm2;
    float heated_area_cm2;
    float heater_side_cm;
    float power_w;
    float power_density_w_per_cm2;
    float density_limit_w_per_cm2;
    float voltage_v;
    float current_a;
    float resistance_ohm;
    bool warning_density_high;
    bool warning_density_over;
    bool warning_density_near_limit;
} heating_pad_result_t;

bool heating_pad_calculate(const heating_pad_input_t *in, heating_pad_result_t *out);
void heating_pad_run_self_test(void);

#ifdef __cplusplus
}
#endif

