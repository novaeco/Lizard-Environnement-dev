#include "calc_heating_pad.h"

#include <math.h>
#include <stdio.h>

typedef struct {
    float heated_area_cm2;
    float power_w;
} calibration_point_t;

static const calibration_point_t k_calibration[] = {
    {.heated_area_cm2 = 120.0f, .power_w = 5.0f},
    {.heated_area_cm2 = 184.0f, .power_w = 7.5f},
    {.heated_area_cm2 = 377.0f, .power_w = 15.0f},
    {.heated_area_cm2 = 865.0f, .power_w = 35.0f},
    {.heated_area_cm2 = 1947.0f, .power_w = 78.0f},
};

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

static float material_coeff(terrarium_material_t m)
{
    switch (m) {
    case TERRARIUM_MATERIAL_WOOD:
        return 0.88f; // bois isolé, pertes moindres
    case TERRARIUM_MATERIAL_GLASS:
        return 1.00f; // référence
    case TERRARIUM_MATERIAL_PVC:
        return 0.93f;
    case TERRARIUM_MATERIAL_ACRYLIC:
        return 0.96f;
    default:
        return 1.0f;
    }
}

static float density_limit(terrarium_material_t m)
{
    switch (m) {
    case TERRARIUM_MATERIAL_GLASS:
        return 0.055f; // fiches tapis 0,035-0,055 W/cm² sur verre
    case TERRARIUM_MATERIAL_WOOD:
        return 0.065f; // bois supporte mieux, mais surveiller l'isolation
    case TERRARIUM_MATERIAL_PVC:
        return 0.050f; // PVC sensible à la chaleur (déformation)
    case TERRARIUM_MATERIAL_ACRYLIC:
        return 0.045f; // PMMA adoucit vite
    default:
        return 0.050f;
    }
}

static float interpolated_density(float heated_area_cm2)
{
    // Interpolation linéaire sur les points catalogues fournis (densité moyenne 0,040-0,042 W/cm²)
    const size_t n = sizeof(k_calibration) / sizeof(k_calibration[0]);
    if (heated_area_cm2 <= k_calibration[0].heated_area_cm2) {
        return k_calibration[0].power_w / k_calibration[0].heated_area_cm2;
    }
    for (size_t i = 1; i < n; ++i) {
        const calibration_point_t *a = &k_calibration[i - 1];
        const calibration_point_t *b = &k_calibration[i];
        if (heated_area_cm2 <= b->heated_area_cm2) {
            float t = (heated_area_cm2 - a->heated_area_cm2) / (b->heated_area_cm2 - a->heated_area_cm2);
            float power = a->power_w + t * (b->power_w - a->power_w);
            return power / heated_area_cm2;
        }
    }
    // extrapolation douce au-delà du dernier point avec légère réduction pour limiter les surchauffes sur grandes surfaces
    float last_density = k_calibration[n - 1].power_w / k_calibration[n - 1].heated_area_cm2;
    return last_density * clampf(1947.0f / heated_area_cm2, 0.85f, 1.0f);
}

static float round_catalog_power(float p)
{
    const float steps[] = {5, 7.5f, 10, 12.5f, 15, 20, 25, 30, 35, 40, 50, 60, 78, 100};
    const size_t n = sizeof(steps) / sizeof(steps[0]);
    for (size_t i = 0; i < n; ++i) {
        if (p <= steps[i]) {
            return steps[i];
        }
    }
    return ceilf(p / 25.0f) * 25.0f;
}

bool heating_pad_calculate(const heating_pad_input_t *in, heating_pad_result_t *out)
{
    if (!in || !out) {
        return false;
    }
    if (in->length_cm < 5.0f || in->depth_cm < 5.0f || in->height_cm <= 0.0f) {
        return false;
    }

    heating_pad_result_t r = {0};

    const float ratio = clampf(in->heated_ratio, 0.2f, 0.6f);
    const float floor_area = in->length_cm * in->depth_cm;
    const float heated_area = floor_area * ratio;
    const float heater_side = sqrtf(heated_area);
    const float coeff = material_coeff(in->material);

    // densité issue du tableau + correction hauteur (terrariums hauts = pertes verticales)
    const float density_catalog = interpolated_density(heated_area);
    const float height_factor = clampf(in->height_cm / 50.0f, 0.8f, 1.35f);
    const float power_raw = heated_area * density_catalog * coeff * height_factor;
    const float power_catalog = round_catalog_power(power_raw);
    const float voltage = (power_catalog <= 18.0f) ? 12.0f : 24.0f;
    const float current = power_catalog / voltage;
    const float resistance = (voltage * voltage) / power_catalog;
    const float density = power_catalog / heated_area;
    const float limit = density_limit(in->material);

    r.valid = true;
    r.floor_area_cm2 = floor_area;
    r.heated_area_cm2 = heated_area;
    r.heater_side_cm = roundf(heater_side * 2.0f) / 2.0f;
    r.power_w = power_catalog;
    r.power_density_w_per_cm2 = density;
    r.density_limit_w_per_cm2 = limit;
    r.voltage_v = voltage;
    r.current_a = current;
    r.resistance_ohm = resistance;
    r.warning_density_high = density > limit * 0.9f;
    r.warning_density_over = density > limit;

    *out = r;
    return true;
}

static void log_case(float l, float p, float h, float ratio, terrarium_material_t m)
{
    heating_pad_input_t in = {.length_cm = l, .depth_cm = p, .height_cm = h, .material = m, .heated_ratio = ratio};
    heating_pad_result_t out = {0};
    if (heating_pad_calculate(&in, &out)) {
        printf("[TEST tapis] %.0fx%.0f -> %.1f cm², %.1f W (%.3f W/cm²) %s\n",
               l,
               p,
               out.heated_area_cm2,
               out.power_w,
               out.power_density_w_per_cm2,
               out.warning_density_high ? "ALERTE" : "OK");
    }
}

void heating_pad_run_self_test(void)
{
    log_case(20, 20, 14, 0.33f, TERRARIUM_MATERIAL_GLASS);
    log_case(30, 20, 20, 0.33f, TERRARIUM_MATERIAL_GLASS);
    log_case(40, 30, 30, 0.33f, TERRARIUM_MATERIAL_GLASS);
    log_case(60, 45, 45, 0.33f, TERRARIUM_MATERIAL_GLASS);
    log_case(100, 60, 60, 0.33f, TERRARIUM_MATERIAL_GLASS);
}

