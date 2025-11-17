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
// et ajustés sans marge (norme/catalogue) car peu critiques thermiquement
static float target_lux_for_env(terrarium_environment_t env)
{
    switch (env) {
    case TERRARIUM_ENV_TROPICAL:
        return 12000.0f; // jungle claire (10-15 klux)
        return 12000.0f; // jungle claire
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

// UVI cibles basés sur Ferguson et al. 2010 (norme scientifique) avec alerte ±20 % (prudent)
static float target_uvi_for_env(terrarium_environment_t env)
{
    switch (env) {
    case TERRARIUM_ENV_TROPICAL:
        return 2.0f; // zone Ferguson 2-3
    case TERRARIUM_ENV_DESERTIC:
        return 4.0f; // zone Ferguson 3-4
    case TERRARIUM_ENV_TEMPERATE_FOREST:
        return 1.5f;
    case TERRARIUM_ENV_NOCTURNAL:
        return 0.5f;
    default:
        return 1.5f;
    }
}

// Distances recommandées depuis fiches Arcadia/Exo Terra T5 UVB (catalogue)
// valeurs centrales, à affiner avec UVI-mètre ; marge de sécurité via alertes 80-120 %
static float recommended_distance_for_env(terrarium_environment_t env)
{
    switch (env) {
    case TERRARIUM_ENV_DESERTIC:
        return 30.0f; // reptile solaire type Pogona
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

// Plages Ferguson (norme) utilisées pour encadrer l'alerte : warning si <80 % ou >120 %
static void ferguson_range(terrarium_environment_t env, float *min_uvi, float *max_uvi)
{
    switch (env) {
    case TERRARIUM_ENV_DESERTIC:
        *min_uvi = 3.0f;
        *max_uvi = 6.0f;
        break; // zone 3-4
    case TERRARIUM_ENV_TROPICAL:
        *min_uvi = 1.0f;
        *max_uvi = 3.0f;
        break; // zone 2-3
    case TERRARIUM_ENV_TEMPERATE_FOREST:
        *min_uvi = 0.7f;
        *max_uvi = 2.0f;
        break; // zone 2
    case TERRARIUM_ENV_NOCTURNAL:
    default:
        *min_uvi = 0.0f;
        *max_uvi = 1.0f;
        break; // zone 1
    }
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
    const float target_distance = clampf(in->height_cm > 0.0f ? in->height_cm * 0.7f : r.led.recommended_distance_cm,
                                         10.0f,
                                         60.0f);

    const float uvb_per_module = in->uvb_uvi_at_distance;
    if (uvb_per_module > 0.0f) {
        const float projected = uvb_per_module * powf(ref_dist / target_distance, 2.0f);
        const float uvb_units = target_mid / fmaxf(projected, 0.1f);
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
        const float target_uva = clampf(target_mid * 15.0f, 2.0f, 20.0f);
        const float projected = in->uva_irradiance_mw_cm2_at_distance * powf(ref_dist / target_distance, 2.0f);
        const float uva_units = target_uva / fmaxf(projected, 0.1f);
        r.uva.module_count = (uint32_t)ceilf(uva_units - 1e-3f);
        r.uva.target_uvi_min = target_uva;
        r.uva.target_uvi_max = target_uva;
        r.uva.valid = r.uva.module_count > 0;
        r.uva.recommended_distance_cm = target_distance;
        r.uva.estimated_uvi_at_distance = projected;
        r.uva.estimated_total_uvi = projected * r.uva.module_count;
        r.uva.warning_high = projected > target_uva * 1.6f;
        r.uva.warning_low = projected < target_uva * 0.5f;

    const float ref_dist = (in->reference_distance_cm > 0.0f) ? in->reference_distance_cm : 30.0f;
    const float target_uvi = target_uvi_for_env(in->environment);
    const float uvb_per_module = in->uvb_uvi_at_distance;
    if (uvb_per_module > 0.0f) {
        const float uvb_units = target_uvi / uvb_per_module;
        r.uvb.module_count = (uint32_t)ceilf(uvb_units - 1e-3f);
        r.uvb.target_uvi = target_uvi;
        r.uvb.valid = r.uvb.module_count > 0;
        r.uvb.recommended_distance_cm = ref_dist;
        r.uvb.warning_high = uvb_per_module * r.uvb.module_count > target_uvi * 1.5f;
    }

    if (in->uva_irradiance_mw_cm2_at_distance > 0.0f) {
        const float target_uva = clampf(target_uvi * 15.0f, 2.0f, 20.0f);
        const float uva_units = target_uva / in->uva_irradiance_mw_cm2_at_distance;
        r.uva.module_count = (uint32_t)ceilf(uva_units - 1e-3f);
        r.uva.target_uvi = target_uva;
        r.uva.valid = r.uva.module_count > 0;
        r.uva.recommended_distance_cm = ref_dist;
        r.uva.warning_high = in->uva_irradiance_mw_cm2_at_distance * r.uva.module_count > target_uva * 1.6f;
    }

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
        printf("[TEST éclairage] LED=%u (%.0f lm), UVB=%u, UVA=%u\n",
               out.led.led_count,
               out.led.total_flux_lm,
               out.uvb.module_count,
               out.uva.module_count);
    }
}

