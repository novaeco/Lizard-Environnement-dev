#include "calc.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define DEFAULT_HEATER_DENSITY_W_PER_CM2 (0.040f)
#define DEFAULT_LED_UNIT_POWER_W        (5.0f)
#define DEFAULT_LED_EFFICIENCY_LM_PER_W (110.0f)
#define DEFAULT_LUX_TARGET               (12000.0f)
#define DEFAULT_UV_TARGET                (150.0f)
#define DEFAULT_UV_MODULE_INTENSITY      (80.0f)
#define DEFAULT_MIST_DENSITY_M2          (0.10f)
#define DEFAULT_SUBSTRATE_THICKNESS_CM   (5.0f)

static const float s_material_coefficients[CALC_MATERIAL_COUNT] = {
    [CALC_MATERIAL_WOOD] = 0.85f,
    [CALC_MATERIAL_GLASS] = 1.00f,
    [CALC_MATERIAL_PVC] = 0.90f,
    [CALC_MATERIAL_ACRYLIC] = 0.95f,
};

static const char *s_material_names[CALC_MATERIAL_COUNT] = {
    [CALC_MATERIAL_WOOD] = "Bois",
    [CALC_MATERIAL_GLASS] = "Verre",
    [CALC_MATERIAL_PVC] = "PVC",
    [CALC_MATERIAL_ACRYLIC] = "Acrylique",
};

static const unsigned s_heater_catalog_values_w[] = {
    5, 8, 10, 13, 15, 20, 25, 30, 35, 40, 50, 60, 78, 100
};

static float round_to_step(float value, float step)
{
    if (step <= 0.0f) {
        return value;
    }
    float scaled = value / step;
    float rounded = floorf(scaled + 0.5f);
    return rounded * step;
}

static unsigned select_catalog_power(float requested_w)
{
    size_t catalog_count = sizeof(s_heater_catalog_values_w) / sizeof(s_heater_catalog_values_w[0]);
    for (size_t i = 0; i < catalog_count; ++i) {
        if (requested_w <= (float)s_heater_catalog_values_w[i]) {
            return s_heater_catalog_values_w[i];
        }
    }
    return s_heater_catalog_values_w[catalog_count - 1];
}

void calc_get_default_inputs(calc_inputs_t *inputs)
{
    if (!inputs) {
        return;
    }
    memset(inputs, 0, sizeof(*inputs));
    inputs->length_cm = 120.0f;
    inputs->width_cm = 60.0f;
    inputs->height_cm = 60.0f;
    inputs->substrate_thickness_cm = DEFAULT_SUBSTRATE_THICKNESS_CM;
    inputs->floor_heater_power_density_w_per_cm2 = DEFAULT_HEATER_DENSITY_W_PER_CM2;
    inputs->led_target_lux = DEFAULT_LUX_TARGET;
    inputs->led_efficiency_lm_per_w = DEFAULT_LED_EFFICIENCY_LM_PER_W;
    inputs->led_unit_power_w = DEFAULT_LED_UNIT_POWER_W;
    inputs->uv_target_intensity = DEFAULT_UV_TARGET;
    inputs->uv_module_intensity = DEFAULT_UV_MODULE_INTENSITY;
    inputs->mist_nozzle_density_m2 = DEFAULT_MIST_DENSITY_M2;
    inputs->floor_material = CALC_MATERIAL_GLASS;
}

static float calc_voltage_for_power(unsigned catalog_power_w)
{
    return (catalog_power_w <= 18U) ? 12.0f : 24.0f;
}

void calc_compute(const calc_inputs_t *inputs, calc_results_t *results)
{
    if (!inputs || !results) {
        return;
    }

    memset(results, 0, sizeof(*results));

    float length_cm = fmaxf(inputs->length_cm, 0.0f);
    float width_cm = fmaxf(inputs->width_cm, 0.0f);
    float substrate_thickness_cm = fmaxf(inputs->substrate_thickness_cm, 0.0f);
    float power_density = (inputs->floor_heater_power_density_w_per_cm2 > 0.0f) ?
                          inputs->floor_heater_power_density_w_per_cm2 : DEFAULT_HEATER_DENSITY_W_PER_CM2;
    float led_efficiency = (inputs->led_efficiency_lm_per_w > 0.0f) ?
                           inputs->led_efficiency_lm_per_w : DEFAULT_LED_EFFICIENCY_LM_PER_W;
    float led_unit_power = (inputs->led_unit_power_w > 0.0f) ?
                           inputs->led_unit_power_w : DEFAULT_LED_UNIT_POWER_W;
    float uv_module_intensity = (inputs->uv_module_intensity > 0.0f) ?
                                inputs->uv_module_intensity : DEFAULT_UV_MODULE_INTENSITY;
    float mist_density = (inputs->mist_nozzle_density_m2 > 0.0f) ?
                         inputs->mist_nozzle_density_m2 : DEFAULT_MIST_DENSITY_M2;

    float floor_area_cm2 = length_cm * width_cm;
    results->area_floor_cm2 = floor_area_cm2;

    float heater_area_cm2 = floor_area_cm2 / 3.0f;
    results->heater_target_area_cm2 = heater_area_cm2;

    float heater_side_cm = 0.0f;
    if (heater_area_cm2 > 0.0f) {
        heater_side_cm = round_to_step(sqrtf(heater_area_cm2), 0.5f);
    }
    results->heater_square_side_cm = heater_side_cm;

    float material_coeff = calc_material_coefficient(inputs->floor_material);
    float corrected_area = heater_area_cm2 * material_coeff;
    results->heater_corrected_area_cm2 = corrected_area;

    float requested_power = power_density * corrected_area;
    results->heater_power_w = requested_power;

    unsigned catalog_power = select_catalog_power(requested_power);
    results->heater_catalog_power_w = catalog_power;

    float heater_voltage = calc_voltage_for_power(catalog_power);
    results->heater_voltage_v = heater_voltage;

    if (catalog_power > 0) {
        results->heater_current_a = catalog_power / heater_voltage;
        results->heater_resistance_ohm = (heater_voltage * heater_voltage) / catalog_power;
    }

    float area_m2 = floor_area_cm2 / 10000.0f;
    results->led_area_m2 = area_m2;

    float luminous_flux = inputs->led_target_lux * area_m2;
    results->led_luminous_flux_lm = luminous_flux;

    float required_led_power = luminous_flux / led_efficiency;
    results->led_required_power_w = required_led_power;

    unsigned led_count = 0;
    if (led_unit_power > 0.0f) {
        led_count = (unsigned)ceilf(required_led_power / led_unit_power);
    }
    results->led_count = led_count;
    results->led_total_power_w = led_count * led_unit_power;

    unsigned uv_count = 0;
    if (uv_module_intensity > 0.0f) {
        uv_count = (unsigned)ceilf(inputs->uv_target_intensity / uv_module_intensity);
    }
    results->uv_module_count = uv_count;

    results->substrate_volume_l = (floor_area_cm2 * substrate_thickness_cm) / 1000.0f;

    unsigned mist_nozzles = 0;
    if (mist_density > 0.0f) {
        mist_nozzles = (unsigned)ceilf(area_m2 / mist_density);
    }
    results->mist_nozzle_count = mist_nozzles;
}

const char *calc_material_to_string(calc_material_t material)
{
    if (material < CALC_MATERIAL_COUNT) {
        return s_material_names[material];
    }
    return "Inconnu";
}

float calc_material_coefficient(calc_material_t material)
{
    if (material < CALC_MATERIAL_COUNT) {
        return s_material_coefficients[material];
    }
    return 1.0f;
}
