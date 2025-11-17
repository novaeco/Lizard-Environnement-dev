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
        return 0.05f; // fabricants 0,03-0,05 W/cm² max sur verre (catalogue) + alertes à 90 % (prudent)
        return 0.05f;
    case TERRARIUM_MATERIAL_WOOD:
        return 0.06f; // bois mieux tolérant, plafond catalogue Zoo Med/Exo Terra
    case TERRARIUM_MATERIAL_PVC:
        return 0.04f; // PVC ramolli >60 °C, abaissement -20 % vs verre (prudent)
    case TERRARIUM_MATERIAL_ACRYLIC:
        return 0.035f; // PMMA déformable, limite abaissée (prudent)
    default:
        return 0.05f;
    }
}

static float default_target_density(terrarium_material_t m)
{
    switch (m) {
    case TERRARIUM_MATERIAL_WOOD:
        return 0.045f;
    case TERRARIUM_MATERIAL_GLASS:
        return 0.038f;
    case TERRARIUM_MATERIAL_PVC:
        return 0.030f;
    case TERRARIUM_MATERIAL_ACRYLIC:
        return 0.028f;
    default:
        return 0.038f;
    }
}

bool heating_cable_calculate(const heating_cable_input_t *in, heating_cable_result_t *out)
{
    if (!in || !out) {
        return false;
    }
    if (in->length_cm < 10.0f || in->depth_cm < 10.0f || in->power_linear_w_per_m <= 0.0f) {
    if (in->length_cm <= 0.0f || in->depth_cm <= 0.0f || in->power_linear_w_per_m <= 0.0f) {
        return false;
    }

    heating_cable_result_t r = {0};
    const float ratio = clampf(in->heated_ratio, 0.25f, 0.6f);
    const float heated_area = in->length_cm * in->depth_cm * ratio;
    const float spacing = clampf(in->spacing_cm > 0.0f ? in->spacing_cm : 4.0f, 2.5f, 10.0f);
    const float target_density = clampf(in->target_power_density_w_per_cm2 > 0.0f ? in->target_power_density_w_per_cm2
                                                                                 : default_target_density(in->material),
                                        0.02f,
                                        0.07f);
    const float target_power = heated_area * target_density;
    const float length_from_power_m = target_power / in->power_linear_w_per_m;
    const float length_from_geometry_m = heated_area / (spacing * 100.0f); // découpe serpentin simple
    const float length_m = fmaxf(length_from_power_m, length_from_geometry_m);
    const float resulting_density = (in->power_linear_w_per_m * length_m) / heated_area;
    const float limit = density_limit(in->material); // catalogue + marge 10 % via alerte 90 %
    const float r_per_m = (in->supply_voltage_v > 0.0f) ? ((in->supply_voltage_v * in->supply_voltage_v) / in->power_linear_w_per_m) : 0.0f;
    const float resistance = (r_per_m > 0.0f) ? r_per_m * length_m : 0.0f;
    const float current = (resistance > 0.0f && in->supply_voltage_v > 0.0f) ? in->supply_voltage_v / resistance : 0.0f;
    const float target_density = clampf(in->target_power_density_w_per_cm2, 0.02f, 0.07f);
    const float target_power = heated_area * target_density;
    const float length_m = target_power / in->power_linear_w_per_m;
    const float resulting_density = (in->power_linear_w_per_m * length_m) / heated_area;

    r.valid = true;
    r.heated_area_cm2 = heated_area;
    r.target_power_w = target_power;
    r.recommended_length_m = length_m;
    r.resulting_density_w_per_cm2 = resulting_density;
    r.spacing_cm = spacing;
    r.estimated_resistance_ohm = resistance;
    r.estimated_current_a = current;
    r.warning_density_high = resulting_density > limit * 0.9f;
    r.warning_spacing_too_tight = spacing < 3.0f;
    r.warning_high_voltage = in->supply_voltage_v >= 220.0f; // 230 V traité uniquement pour vérification théorique
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
        .spacing_cm = 4.0f,
    };
    heating_cable_result_t out = {0};
    if (heating_cable_calculate(&in, &out)) {
        printf("[TEST câble] surface %.0f cm² -> %.2f m (%.1f cm d'espacement), %.3f W/cm²\n",
               out.heated_area_cm2,
               out.recommended_length_m,
                out.spacing_cm,
    };
    heating_cable_result_t out = {0};
    if (heating_cable_calculate(&in, &out)) {
        printf("[TEST câble] surface %.0f cm² -> %.1f m, %.3f W/cm²\n",
               out.heated_area_cm2,
               out.recommended_length_m,
               out.resulting_density_w_per_cm2);
    }
}

