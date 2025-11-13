#include "calc.h"

#include <math.h>
#include <stddef.h>

#define TERRARIUM_DIMENSION_MIN_CM   10.0f
#define TERRARIUM_DIMENSION_MAX_CM   400.0f
#define TERRARIUM_SUBSTRATE_MAX_CM   40.0f
#define TERRARIUM_LED_EFFICIENCY_MAX 320.0f
#define TERRARIUM_LED_POWER_MAX_W    80.0f
#define TERRARIUM_MIST_DENSITY_MIN   0.01f
#define TERRARIUM_MIST_DENSITY_MAX   1.0f

static float clampf(float value, float min, float max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

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
    const size_t count = sizeof(catalog) / sizeof(catalog[0]);

    for (size_t i = 0; i < count; ++i) {
        if (value <= catalog[i]) {
            return catalog[i];
        }
    }

    /*
     * When the requested power exceeds the predefined catalog, fall back to a
     * conservative rounding strategy that increases in 25 W steps. This avoids
     * returning the maximum catalog entry (100 W) for higher requirements and
     * prevents undersizing large terrariums.
     */
    const float step = 25.0f;
    return ceilf(value / step) * step;
}

static uint32_t ceil_positive(float value)
{
    if (value <= 0.0f) {
        return 0U;
    }
    float rounded = ceilf(value - 1e-6f);
    if (rounded < 1.0f) {
        rounded = 1.0f;
    }
    return (uint32_t)rounded;
}

bool terrarium_calc_compute(const terrarium_calc_input_t *input, terrarium_calc_result_t *result)
{
    if (!input || !result) {
        return false;
    }

    if (input->length_cm <= 0.0f || input->width_cm <= 0.0f || input->height_cm <= 0.0f ||
        input->material >= TERRARIUM_MATERIAL_COUNT) {
        return false;
    }

    terrarium_calc_result_t temp = {0};
    bool clamped = false;

    const float length_cm = clampf(input->length_cm, TERRARIUM_DIMENSION_MIN_CM, TERRARIUM_DIMENSION_MAX_CM);
    const float width_cm = clampf(input->width_cm, TERRARIUM_DIMENSION_MIN_CM, TERRARIUM_DIMENSION_MAX_CM);
    const float height_cm = clampf(input->height_cm, TERRARIUM_DIMENSION_MIN_CM, TERRARIUM_DIMENSION_MAX_CM);
    const float substrate_thickness_cm = clampf(input->substrate_thickness_cm, 0.0f, TERRARIUM_SUBSTRATE_MAX_CM);
    const float led_eff = clampf(input->led_efficiency_lm_per_w, 0.0f, TERRARIUM_LED_EFFICIENCY_MAX);
    const float led_power_unit = clampf(input->led_power_per_unit_w, 0.0f, TERRARIUM_LED_POWER_MAX_W);
    float mist_density = input->mist_density_m2_per_nozzle;
    if (mist_density < 0.0f) {
        mist_density = TERRARIUM_MIST_DENSITY_MIN;
    } else if (mist_density > TERRARIUM_MIST_DENSITY_MAX) {
        mist_density = TERRARIUM_MIST_DENSITY_MAX;
    }

    if (length_cm != input->length_cm || width_cm != input->width_cm || height_cm != input->height_cm ||
        substrate_thickness_cm != input->substrate_thickness_cm || led_eff != input->led_efficiency_lm_per_w ||
        led_power_unit != input->led_power_per_unit_w || mist_density != input->mist_density_m2_per_nozzle) {
        clamped = true;
    }

    const float floor_area_cm2 = length_cm * width_cm;
    const float floor_area_m2 = floor_area_cm2 / 10000.0f;
    const float enclosure_volume_l = (length_cm * width_cm * height_cm) / 1000.0f;
    const float heater_target_area_cm2 = floor_area_cm2 / 3.0f;
    const float heater_side_cm = round_to_step(sqrtf(heater_target_area_cm2), 0.5f);
    const float coeff = material_coefficient(input->material);

    const float reference_volume_l = 300.0f; /* ~100×50×60 cm */
    float volume_factor = enclosure_volume_l / reference_volume_l;
    volume_factor = clampf(volume_factor, 0.7f, 1.4f);

    const float heater_power_raw = heater_target_area_cm2 * 0.040f * coeff * volume_factor;
    const float heater_power_catalog = round_up_catalog_power(heater_power_raw);
    const float heater_voltage = (heater_power_catalog <= 18.0f) ? 12.0f : 24.0f;
    const float heater_current = heater_power_catalog / heater_voltage;
    const float heater_resistance = (heater_voltage * heater_voltage) / heater_power_catalog;

    temp.status_flags |= TERRARIUM_CALC_STATUS_HEATING_VALID;
    temp.heating.valid = true;
    temp.heating.floor_area_cm2 = floor_area_cm2;
    temp.heating.floor_area_m2 = floor_area_m2;
    temp.heating.heater_target_area_cm2 = heater_target_area_cm2;
    temp.heating.heater_side_cm = heater_side_cm;
    temp.heating.heater_power_raw_w = heater_power_raw;
    temp.heating.heater_power_catalog_w = heater_power_catalog;
    temp.heating.heater_voltage_v = heater_voltage;
    temp.heating.heater_current_a = heater_current;
    temp.heating.heater_resistance_ohm = heater_resistance;
    temp.heating.enclosure_volume_l = enclosure_volume_l;

    if (input->target_lux > 0.0f && led_eff > 0.0f && led_power_unit > 0.0f) {
        const float luminous_flux = input->target_lux * floor_area_m2;
        const float led_power = luminous_flux / led_eff;
        const float led_units = led_power / led_power_unit;
        const uint32_t led_count = ceil_positive(led_units);
        temp.status_flags |= TERRARIUM_CALC_STATUS_LIGHTING_VALID;
        temp.lighting.valid = true;
        temp.lighting.luminous_flux_lm = luminous_flux;
        temp.lighting.power_w = led_power;
        temp.lighting.led_count = led_count;
        temp.lighting.power_per_led_w = led_power_unit;
    } else {
        temp.lighting.valid = false;
        temp.lighting.luminous_flux_lm = 0.0f;
        temp.lighting.power_w = 0.0f;
        temp.lighting.led_count = 0U;
        temp.lighting.power_per_led_w = led_power_unit;
    }

    if (input->uv_target_intensity > 0.0f && input->uv_module_intensity > 0.0f) {
        const float uv_units = input->uv_target_intensity / input->uv_module_intensity;
        temp.uv.module_count = ceil_positive(uv_units);
        temp.uv.valid = true;
        temp.status_flags |= TERRARIUM_CALC_STATUS_UV_VALID;
    } else {
        temp.uv.valid = false;
        temp.uv.module_count = 0U;
    }

    temp.substrate.volume_l = (floor_area_cm2 * substrate_thickness_cm) / 1000.0f;
    temp.substrate.valid = substrate_thickness_cm > 0.0f;
    if (temp.substrate.valid) {
        temp.status_flags |= TERRARIUM_CALC_STATUS_SUBSTRATE_VALID;
    }

    if (mist_density > 0.0f) {
        const float nozzle_units = floor_area_m2 / mist_density;
        temp.misting.nozzle_count = ceil_positive(nozzle_units);
        temp.misting.valid = true;
        temp.status_flags |= TERRARIUM_CALC_STATUS_MISTING_VALID;
    } else {
        temp.misting.valid = false;
        temp.misting.nozzle_count = 0U;
    }

    if (clamped) {
        temp.status_flags |= TERRARIUM_CALC_STATUS_INPUT_CLAMPED;
    }

    *result = temp;
    return true;
}

