#include "calc_lighting.h"

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

// Lux cibles issus des guides terrariophiles (Arcadia/Exo Terra) pour l'éclairage général
static float target_lux_for_env(terrarium_environment_t env)
{
    switch (env) {
    case TERRARIUM_ENV_TROPICAL:
        return 12000.0f; // jungle claire (10-15 klux)
    case TERRARIUM_ENV_DESERTIC:
        return 20000.0f; // zone ensoleillée pour Pogona
    case TERRARIUM_ENV_TEMPERATE_FOREST:
        return 8000.0f;
    case TERRARIUM_ENV_NOCTURNAL:
        return 2000.0f;
    default:
        return 10000.0f;
    }
}

// Plages Ferguson (norme) utilisées pour recaler l'objectif UVI
static void ferguson_range(terrarium_environment_t env, float *min_uvi, float *max_uvi)
{
    switch (env) {
    case TERRARIUM_ENV_DESERTIC:
        *min_uvi = 3.0f;
        *max_uvi = 6.0f;
        break;
    case TERRARIUM_ENV_TROPICAL:
        *min_uvi = 1.0f;
        *max_uvi = 3.0f;
        break;
    case TERRARIUM_ENV_TEMPERATE_FOREST:
        *min_uvi = 0.7f;
        *max_uvi = 2.0f;
        break;
    case TERRARIUM_ENV_NOCTURNAL:
    default:
        *min_uvi = 0.0f;
        *max_uvi = 1.0f;
        break;
    }
}

static float recommended_distance_for_env(terrarium_environment_t env)
{
    switch (env) {
    case TERRARIUM_ENV_DESERTIC:
        return 30.0f;
    case TERRARIUM_ENV_TROPICAL:
        return 25.0f;
    case TERRARIUM_ENV_TEMPERATE_FOREST:
        return 20.0f;
    case TERRARIUM_ENV_NOCTURNAL:
        return 15.0f;
    default:
        return 25.0f;
    }
}

static float project_irradiance(float value_at_ref, float ref_cm, float target_cm)
{
    // loi en 1/r^p légèrement adoucie (p=1.9) pour refléter les réflecteurs UV
    const float exponent = 1.9f;
    const float ref = fmaxf(ref_cm, 1.0f);
    const float tgt = fmaxf(target_cm, 1.0f);
    return value_at_ref * powf(ref / tgt, exponent);
}

bool lighting_calculate(const lighting_input_t *in, lighting_result_t *out)
{
    if (!in || !out) {
        return false;
    }
    if (in->length_cm <= 0.0f || in->depth_cm <= 0.0f || in->led_luminous_flux_lm <= 0.0f || in->led_power_w <= 0.0f) {
        return false;
    }

    lighting_result_t r = {0};
    const float floor_area_m2 = (in->length_cm * in->depth_cm) / 10000.0f;
    const float target_lux = target_lux_for_env(in->environment);
    const float total_flux = target_lux * floor_area_m2;
    const float led_units = total_flux / in->led_luminous_flux_lm;
    const uint32_t led_count = (uint32_t)ceilf(led_units - 1e-3f);
    const float total_power = led_count * in->led_power_w;

    r.led.valid = led_count > 0;
    r.led.target_lux = target_lux;
    r.led.total_flux_lm = total_flux;
    r.led.led_count = led_count;
    r.led.total_power_w = total_power;
    r.led.recommended_distance_cm = recommended_distance_for_env(in->environment);
    r.led.area_m2 = floor_area_m2;

    const float ref_dist = (in->reference_distance_cm > 0.0f) ? in->reference_distance_cm : 30.0f;
    float uvi_min = 0.0f, uvi_max = 0.0f;
    ferguson_range(in->environment, &uvi_min, &uvi_max);
    const float target_mid = (uvi_min + uvi_max) * 0.5f;

    const float base_distance = recommended_distance_for_env(in->environment);
    const float target_distance = clampf(in->height_cm > 0.0f ? in->height_cm * 0.7f : base_distance, 10.0f, 80.0f);

    const float uvb_per_module = in->uvb_uvi_at_distance;
    if (uvb_per_module > 0.0f) {
        const float projected = project_irradiance(uvb_per_module, ref_dist, target_distance);
        const float uvb_units = target_mid / fmaxf(projected, 0.05f);
        r.uvb.module_count = (uint32_t)ceilf(uvb_units - 1e-3f);
        r.uvb.target_uvi_min = uvi_min;
        r.uvb.target_uvi_max = uvi_max;
        r.uvb.valid = r.uvb.module_count > 0;
        r.uvb.recommended_distance_cm = target_distance;
        r.uvb.estimated_uvi_at_distance = projected;
        r.uvb.estimated_total_uvi = projected * r.uvb.module_count;
        r.uvb.warning_high = r.uvb.estimated_total_uvi > (uvi_max * 1.2f);
        r.uvb.warning_low = r.uvb.estimated_total_uvi < (uvi_min * 0.8f);
    }

    if (in->uva_irradiance_mw_cm2_at_distance > 0.0f) {
        const float target_uva = clampf(target_mid * 15.0f, 1.5f, 25.0f);
        const float projected = project_irradiance(in->uva_irradiance_mw_cm2_at_distance, ref_dist, target_distance);
        const float uva_units = target_uva / fmaxf(projected, 0.05f);
        r.uva.module_count = (uint32_t)ceilf(uva_units - 1e-3f);
        r.uva.target_uvi_min = target_uva;
        r.uva.target_uvi_max = target_uva;
        r.uva.valid = r.uva.module_count > 0;
        r.uva.recommended_distance_cm = target_distance;
        r.uva.estimated_uvi_at_distance = projected;
        r.uva.estimated_total_uvi = projected * r.uva.module_count;
        r.uva.warning_high = r.uva.estimated_total_uvi > target_uva * 1.2f;
        r.uva.warning_low = r.uva.estimated_total_uvi < target_uva * 0.8f;
    }

    r.valid = r.led.valid || r.uvb.valid || r.uva.valid;
    *out = r;
    return true;
}

void lighting_run_self_test(void)
{
    lighting_input_t in = {
        .length_cm = 100,
        .depth_cm = 60,
        .height_cm = 60,
        .environment = TERRARIUM_ENV_DESERTIC,
        .led_luminous_flux_lm = 150.0f,
        .led_power_w = 1.0f,
        .uva_irradiance_mw_cm2_at_distance = 3.0f,
        .uvb_uvi_at_distance = 1.2f,
        .reference_distance_cm = 30.0f,
    };
    lighting_result_t out = {0};
    if (lighting_calculate(&in, &out)) {
        printf("[TEST éclairage] LED=%u (%.0f lm, %.0f lux cible), UVB=%u (%.2f-%.2f UVI), UVA=%u\n",
               out.led.led_count,
               out.led.total_flux_lm,
               out.led.target_lux,
               out.uvb.module_count,
               out.uvb.target_uvi_min,
               out.uvb.target_uvi_max,
               out.uva.module_count);
    }
}
