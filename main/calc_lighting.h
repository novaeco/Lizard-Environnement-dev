#pragma once

#include "calc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float length_cm;
    float depth_cm;
    float height_cm;
    terrarium_environment_t environment;
    float led_luminous_flux_lm;
    float led_power_w;
    float uva_irradiance_mw_cm2_at_distance;
    float uvb_uvi_at_distance;
    float reference_distance_cm;
} lighting_input_t;

typedef struct {
    bool valid;
    float target_lux;
    float total_flux_lm;
    float total_power_w;
    uint32_t led_count;
    float recommended_distance_cm;
} lighting_led_result_t;

typedef struct {
    bool valid;
    float target_uvi;
    uint32_t module_count;
    float recommended_distance_cm;
    bool warning_high;
} lighting_uv_result_t;

typedef struct {
    lighting_led_result_t led;
    lighting_uv_result_t uva;
    lighting_uv_result_t uvb;
} lighting_result_t;

bool lighting_calculate(const lighting_input_t *in, lighting_result_t *out);
void lighting_run_self_test(void);

#ifdef __cplusplus
}
#endif

