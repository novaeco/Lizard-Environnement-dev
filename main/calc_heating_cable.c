#include "calc_heating_cable.h"

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

typedef struct {
    float min_density_w_cm2;
    float max_density_w_cm2;
    float material_coeff;
} material_limits_t;

static material_limits_t limits_for_material(terrarium_material_t m)
{
    switch (m) {
    case TERRARIUM_MATERIAL_WOOD:
        return (material_limits_t){.min_density_w_cm2 = 0.030f, .max_density_w_cm2 = 0.065f, .material_coeff = 0.92f};
    case TERRARIUM_MATERIAL_GLASS:
        return (material_limits_t){.min_density_w_cm2 = 0.028f, .max_density_w_cm2 = 0.055f, .material_coeff = 1.0f};
    case TERRARIUM_MATERIAL_PVC:
        return (material_limits_t){.min_density_w_cm2 = 0.025f, .max_density_w_cm2 = 0.050f, .material_coeff = 0.90f};
    case TERRARIUM_MATERIAL_ACRYLIC:
        return (material_limits_t){.min_density_w_cm2 = 0.024f, .max_density_w_cm2 = 0.045f, .material_coeff = 0.95f};
    default:
        return (material_limits_t){.min_density_w_cm2 = 0.028f, .max_density_w_cm2 = 0.055f, .material_coeff = 1.0f};
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

static float spline_power_for_area(float area_cm2)
{
    const size_t n = sizeof(k_calibration) / sizeof(k_calibration[0]);
    float x[n];
    float y[n];
    for (size_t i = 0; i < n; ++i) {
        x[i] = k_calibration[i].heated_area_cm2;
        y[i] = k_calibration[i].power_w;
    }

    float m[n - 1];
    for (size_t i = 0; i < n - 1; ++i) {
        m[i] = (y[i + 1] - y[i]) / (x[i + 1] - x[i]);
    }

    float t[n];
    t[0] = m[0];
    for (size_t i = 1; i < n - 1; ++i) {
        t[i] = 0.5f * (m[i - 1] + m[i]);
    }
    t[n - 1] = m[n - 2];

    for (size_t i = 0; i < n - 1; ++i) {
        if (fabsf(m[i]) < 1e-6f) {
            t[i] = 0.0f;
            t[i + 1] = 0.0f;
        } else {
            const float a = t[i] / m[i];
            const float b = t[i + 1] / m[i];
            const float s = a * a + b * b;
            if (s > 9.0f) {
                const float tau = 3.0f / sqrtf(s);
                t[i] = tau * a * m[i];
                t[i + 1] = tau * b * m[i];
            }
        }
    }

    if (area_cm2 <= x[0]) {
        return y[0] + t[0] * (area_cm2 - x[0]);
    }
    if (area_cm2 >= x[n - 1]) {
        return y[n - 1] + t[n - 1] * (area_cm2 - x[n - 1]);
    }

    size_t idx = 0;
    for (size_t i = 0; i < n - 1; ++i) {
        if (area_cm2 <= x[i + 1]) {
            idx = i;
            break;
        }
    }

    const float h = x[idx + 1] - x[idx];
    const float s = (area_cm2 - x[idx]) / h;
    const float h00 = (2.0f * s * s * s) - (3.0f * s * s) + 1.0f;
    const float h10 = (s * s * s) - (2.0f * s * s) + s;
    const float h01 = (-2.0f * s * s * s) + (3.0f * s * s);
    const float h11 = (s * s * s) - (s * s);

    return (h00 * y[idx]) + (h10 * h * t[idx]) + (h01 * y[idx + 1]) + (h11 * h * t[idx + 1]);
}

bool heating_cable_calculate(const heating_cable_input_t *in, heating_cable_result_t *out)
{
    if (!in || !out) {
        return false;
    }
    if (in->length_cm < 10.0f || in->depth_cm < 10.0f || in->power_linear_w_per_m <= 0.0f) {
        return false;
    }

    heating_cable_result_t r = {0};
    const float ratio = clampf(in->heated_ratio, 0.25f, 0.6f);
    const float heated_area = in->length_cm * in->depth_cm * ratio;
    const material_limits_t limits = limits_for_material(in->material);

    const float power_catalog = spline_power_for_area(heated_area);
    const float density_catalog = clampf(power_catalog / heated_area, limits.min_density_w_cm2, limits.max_density_w_cm2);

    const float spacing = clampf(in->spacing_cm > 0.0f ? in->spacing_cm : 4.0f, 2.0f, 12.0f);

    const float target_density_requested = (in->target_power_density_w_per_cm2 > 0.0f)
                                               ? in->target_power_density_w_per_cm2
                                               : density_catalog * limits.material_coeff;
    const float target_density = clampf(target_density_requested, limits.min_density_w_cm2, limits.max_density_w_cm2);

    const float target_power = heated_area * target_density;
    const float length_from_power_m = target_power / in->power_linear_w_per_m;
    const float length_from_geometry_m = heated_area / (spacing * 100.0f);
    const float length_m = fmaxf(length_from_power_m, length_from_geometry_m);

    const float resulting_density = (in->power_linear_w_per_m * length_m) / heated_area;

    const float r_per_m = (in->supply_voltage_v > 0.0f)
                              ? ((in->supply_voltage_v * in->supply_voltage_v) / in->power_linear_w_per_m)
                              : 0.0f;
    const float resistance = (r_per_m > 0.0f) ? r_per_m * length_m : 0.0f;
    const float current = (resistance > 0.0f && in->supply_voltage_v > 0.0f) ? in->supply_voltage_v / resistance : 0.0f;

    r.valid = true;
    r.heated_area_cm2 = heated_area;
    r.target_power_w = target_power;
    r.recommended_length_m = length_m;
    r.resulting_density_w_per_cm2 = resulting_density;
    r.spacing_cm = spacing;
    r.estimated_resistance_ohm = resistance;
    r.estimated_current_a = current;
    r.warning_density_high = resulting_density > limits.max_density_w_cm2 * 0.9f;
    r.warning_density_over = resulting_density > limits.max_density_w_cm2;
    r.warning_spacing_too_tight = spacing < 3.0f;
    r.warning_high_voltage = in->supply_voltage_v >= 220.0f;

    *out = r;
    return true;
}

static void log_case(const heating_cable_input_t *in)
{
    heating_cable_result_t out = {0};
    if (heating_cable_calculate(in, &out)) {
        printf("[TEST câble] %.0fx%.0f (ratio %.2f) -> %.2f m, %.1f cm esp., %.3f W/cm²%s%s\n",
               in->length_cm,
               in->depth_cm,
               in->heated_ratio,
               out.recommended_length_m,
               out.spacing_cm,
               out.resulting_density_w_per_cm2,
               out.warning_density_over ? " DENSITÉ>max" : (out.warning_density_high ? " densité haute" : ""),
               out.warning_spacing_too_tight ? " espacement<3cm" : "");
    }
}

void heating_cable_run_self_test(void)
{
    const heating_cable_input_t nominal = {
        .length_cm = 120,
        .depth_cm = 60,
        .material = TERRARIUM_MATERIAL_GLASS,
        .heated_ratio = 0.33f,
        .power_linear_w_per_m = 20.0f,
        .supply_voltage_v = 24.0f,
        .target_power_density_w_per_cm2 = 0.035f,
        .spacing_cm = 4.0f,
    };
    const heating_cable_input_t minimal = {
        .length_cm = 10,
        .depth_cm = 10,
        .material = TERRARIUM_MATERIAL_PVC,
        .heated_ratio = 0.25f,
        .power_linear_w_per_m = 15.0f,
        .supply_voltage_v = 12.0f,
        .target_power_density_w_per_cm2 = 0.030f,
        .spacing_cm = 12.0f,
    };
    const heating_cable_input_t dense = {
        .length_cm = 30,
        .depth_cm = 20,
        .material = TERRARIUM_MATERIAL_WOOD,
        .heated_ratio = 0.60f,
        .power_linear_w_per_m = 50.0f,
        .supply_voltage_v = 230.0f,
        .target_power_density_w_per_cm2 = 0.080f, // dépassement volontaire pour tester clamp + avertissements
        .spacing_cm = 2.0f,
    };

    log_case(&nominal);
    log_case(&minimal);
    log_case(&dense);
}
