#include "calc.h"

#include <math.h>
#include <stddef.h>

static float material_coefficient(terrarium_material_t material)
{
    switch (material) {
    case TERRARIUM_MATERIAL_WOOD:
        return 0.85f;
    case TERRARIUM_MATERIAL_GLASS:
        return 1.0f;
    case TERRARIUM_MATERIAL_PVC:
        return 0.90f;
    case TERRARIUM_MATERIAL_ACRYLIC:
        return 0.95f;
    default:
        return 1.0f;
    }
}

static float round_to_step(float value, float step)
{
    if (step <= 0.0f) {
        return value;
    }
    float scaled = value / step;
    float rounded = roundf(scaled);
    return rounded * step;
}

static float round_up_catalog_power(float value)
{
    static const float catalog[] = {
        5.0f, 7.5f, 10.0f, 12.5f, 15.0f, 20.0f, 25.0f, 30.0f, 35.0f,
        40.0f, 50.0f, 60.0f, 78.0f, 100.0f
    };
    size_t count = sizeof(catalog) / sizeof(catalog[0]);

    for (size_t i = 0; i < count; ++i) {
        if (value <= catalog[i]) {
            return catalog[i];
        }
    }
    return catalog[count - 1];
}

static uint32_t ceil_positive(float value)
{
    if (value <= 0.0f) {
        return 0U;
    }
    return (uint32_t)ceilf(value - 1e-6f);
}

bool terrarium_calc_compute(const terrarium_calc_input_t *input, terrarium_calc_result_t *result)
{
    if (!input || !result) {
        return false;
    }

    if (input->length_cm <= 0.0f || input->width_cm <= 0.0f || input->height_cm <= 0.0f ||
        input->substrate_thickness_cm < 0.0f || input->material >= TERRARIUM_MATERIAL_COUNT) {
        return false;
    }

    terrarium_calc_result_t temp = {0};

    const float floor_area_cm2 = input->length_cm * input->width_cm;
    const float floor_area_m2 = floor_area_cm2 / 10000.0f;
    const float heater_target_area_cm2 = floor_area_cm2 / 3.0f;
    const float heater_side_cm = round_to_step(sqrtf(heater_target_area_cm2), 0.5f);
    const float coeff = material_coefficient(input->material);
    const float heater_power_raw = heater_target_area_cm2 * 0.040f * coeff;
    const float heater_power_catalog = round_up_catalog_power(heater_power_raw);
    const float heater_voltage = (heater_power_catalog <= 18.0f) ? 12.0f : 24.0f;
    const float heater_current = heater_power_catalog / heater_voltage;
    const float heater_resistance = (heater_voltage * heater_voltage) / heater_power_catalog;

    temp.heating.floor_area_cm2 = floor_area_cm2;
    temp.heating.floor_area_m2 = floor_area_m2;
    temp.heating.heater_target_area_cm2 = heater_target_area_cm2;
    temp.heating.heater_side_cm = heater_side_cm;
    temp.heating.heater_power_raw_w = heater_power_raw;
    temp.heating.heater_power_catalog_w = heater_power_catalog;
    temp.heating.heater_voltage_v = heater_voltage;
    temp.heating.heater_current_a = heater_current;
    temp.heating.heater_resistance_ohm = heater_resistance;

    if (input->target_lux > 0.0f && input->led_efficiency_lm_per_w > 0.0f && input->led_power_per_unit_w > 0.0f) {
        const float luminous_flux = input->target_lux * floor_area_m2;
        const float led_power = luminous_flux / input->led_efficiency_lm_per_w;
        const float led_units = led_power / input->led_power_per_unit_w;
        const uint32_t led_count = ceil_positive(led_units);
        temp.lighting.luminous_flux_lm = luminous_flux;
        temp.lighting.power_w = led_power;
        temp.lighting.led_count = led_count;
        temp.lighting.power_per_led_w = input->led_power_per_unit_w;
    } else {
        temp.lighting.luminous_flux_lm = 0.0f;
        temp.lighting.power_w = 0.0f;
        temp.lighting.led_count = 0U;
        temp.lighting.power_per_led_w = input->led_power_per_unit_w;
    }

    if (input->uv_target_intensity > 0.0f && input->uv_module_intensity > 0.0f) {
        const float uv_units = input->uv_target_intensity / input->uv_module_intensity;
        temp.uv.module_count = ceil_positive(uv_units);
    } else {
        temp.uv.module_count = 0U;
    }

    temp.substrate.volume_l = (floor_area_cm2 * input->substrate_thickness_cm) / 1000.0f;

    if (input->mist_density_m2_per_nozzle > 0.0f) {
        const float nozzle_units = floor_area_m2 / input->mist_density_m2_per_nozzle;
        temp.misting.nozzle_count = ceil_positive(nozzle_units);
    } else {
        temp.misting.nozzle_count = 0U;
    }

    *result = temp;
    return true;
}

