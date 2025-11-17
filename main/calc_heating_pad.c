#include "calc_heating_pad.h"

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

static float material_coeff(terrarium_material_t m)
{
    switch (m) {
    case TERRARIUM_MATERIAL_WOOD:
        return 0.88f; // bois isolé
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
        return 0.055f; // prudence sur verre
    case TERRARIUM_MATERIAL_WOOD:
        return 0.065f; // bois supporte mieux
    case TERRARIUM_MATERIAL_PVC:
        return 0.050f; // PVC sensible à la chaleur
    case TERRARIUM_MATERIAL_ACRYLIC:
        return 0.045f; // PMMA adoucit vite
    default:
        return 0.050f;
    }
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
    if (in->length_cm <= 0.0f || in->depth_cm <= 0.0f || in->height_cm <= 0.0f) {
        return false;
    }

    heating_pad_result_t r = {0};

    const float ratio = clampf(in->heated_ratio, 0.2f, 0.6f);
    const float floor_area = in->length_cm * in->depth_cm;
    const float heated_area = floor_area * ratio;
    const float heater_side = sqrtf(heated_area);
    const float coeff = material_coeff(in->material);

    /*
     * Modèle issu du tableau fourni (~0,040 W/cm² en moyenne sur 1/3 de surface),
     * ajusté par le coefficient de matériau et par le volume (hauteur) pour éviter
     * la sous-estimation des terrariums hauts.
     */
    const float base_density = 0.040f;
    const float height_factor = clampf(in->height_cm / 50.0f, 0.7f, 1.35f);
    const float power_raw = heated_area * base_density * coeff * height_factor;
    const float power_catalog = round_catalog_power(power_raw);
    const float voltage = (power_catalog <= 18.0f) ? 12.0f : 24.0f;
    const float current = power_catalog / voltage;
    const float resistance = (voltage * voltage) / power_catalog;
    const float density = power_catalog / heated_area;

    r.valid = true;
    r.floor_area_cm2 = floor_area;
    r.heated_area_cm2 = heated_area;
    r.heater_side_cm = roundf(heater_side * 2.0f) / 2.0f;
    r.power_w = power_catalog;
    r.power_density_w_per_cm2 = density;
    r.voltage_v = voltage;
    r.current_a = current;
    r.resistance_ohm = resistance;
    r.warning_density_high = density > density_limit(in->material) * 0.9f;

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

