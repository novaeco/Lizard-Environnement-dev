#include "calc_heating_cable.h"

#include <math.h>
#include <stdio.h>

static float clampf(float v, float min, float max)
{
    if (v < min) {
        return min;
    }
    if (v > max) {
        return max;
    }
    return v;
}

static float density_limit(terrarium_material_t m)
{
    switch (m) {
    case TERRARIUM_MATERIAL_GLASS:
        return 0.05f;
    case TERRARIUM_MATERIAL_WOOD:
        return 0.06f;
    case TERRARIUM_MATERIAL_PVC:
        return 0.04f;
    case TERRARIUM_MATERIAL_ACRYLIC:
        return 0.035f;
    default:
        return 0.05f;
    }
}

bool heating_cable_calculate(const heating_cable_input_t *in, heating_cable_result_t *out)
{
    if (!in || !out) {
        return false;
    }
    if (in->length_cm <= 0.0f || in->depth_cm <= 0.0f || in->power_linear_w_per_m <= 0.0f) {
        return false;
    }

    heating_cable_result_t r = {0};
    const float ratio = clampf(in->heated_ratio, 0.25f, 0.6f);
    const float heated_area = in->length_cm * in->depth_cm * ratio;
    const float target_density = clampf(in->target_power_density_w_per_cm2, 0.02f, 0.07f);
    const float target_power = heated_area * target_density;
    const float length_m = target_power / in->power_linear_w_per_m;
    const float resulting_density = (in->power_linear_w_per_m * length_m) / heated_area;

    r.valid = true;
    r.heated_area_cm2 = heated_area;
    r.target_power_w = target_power;
    r.recommended_length_m = length_m;
    r.resulting_density_w_per_cm2 = resulting_density;
    r.warning_density_high = resulting_density > density_limit(in->material) * 0.9f;

    *out = r;
    return true;
}

void heating_cable_run_self_test(void)
{
    heating_cable_input_t in = {
        .length_cm = 120,
        .depth_cm = 60,
        .material = TERRARIUM_MATERIAL_GLASS,
        .heated_ratio = 0.33f,
        .power_linear_w_per_m = 20.0f,
        .supply_voltage_v = 24.0f,
        .target_power_density_w_per_cm2 = 0.035f,
    };
    heating_cable_result_t out = {0};
    if (heating_cable_calculate(&in, &out)) {
        printf("[TEST câble] surface %.0f cm² -> %.1f m, %.3f W/cm²\n",
               out.heated_area_cm2,
               out.recommended_length_m,
               out.resulting_density_w_per_cm2);
    }
}

