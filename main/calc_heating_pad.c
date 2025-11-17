#include "calc_heating_pad.h"

#include <math.h>
#include <stdio.h>

typedef struct {
    float heated_area_cm2;
    float power_w;
} calibration_point_t;

// Points relevés sur fiches Zoo Med ReptiTherm / Habistat 12-24 V (catalogue)
// et utilisés comme table de spline monotone (densité 0,030-0,045 W/cm²).
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
        return (material_limits_t){.min_density_w_cm2 = 0.032f, .max_density_w_cm2 = 0.065f, .material_coeff = 0.90f};
    case TERRARIUM_MATERIAL_GLASS:
        return (material_limits_t){.min_density_w_cm2 = 0.030f, .max_density_w_cm2 = 0.055f, .material_coeff = 1.00f};
    case TERRARIUM_MATERIAL_PVC:
        return (material_limits_t){.min_density_w_cm2 = 0.028f, .max_density_w_cm2 = 0.050f, .material_coeff = 0.93f};
    case TERRARIUM_MATERIAL_ACRYLIC:
        return (material_limits_t){.min_density_w_cm2 = 0.027f, .max_density_w_cm2 = 0.045f, .material_coeff = 0.96f};
    default:
        return (material_limits_t){.min_density_w_cm2 = 0.030f, .max_density_w_cm2 = 0.055f, .material_coeff = 1.0f};
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

// Spline Hermite monotone (Fritsch-Carlson) sur les 5 points catalogue
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

    const material_limits_t limits = limits_for_material(in->material);
    const float power_catalog = spline_power_for_area(heated_area);
    const float density_catalog = clampf(power_catalog / heated_area, limits.min_density_w_cm2, limits.max_density_w_cm2);

    const float height_factor = clampf(in->height_cm / 50.0f, 0.85f, 1.35f);
    const float density_raw = density_catalog * limits.material_coeff * height_factor;
    const float density_capped = clampf(density_raw, limits.min_density_w_cm2, limits.max_density_w_cm2);

    const float power_raw = heated_area * density_capped;
    const float power_final = round_catalog_power(power_raw);
    const float voltage = (power_final <= 18.0f) ? 12.0f : 24.0f;
    const float current = power_final / voltage;
    const float resistance = (voltage * voltage) / power_final;
    const float density_final = power_final / heated_area;

    r.valid = true;
    r.floor_area_cm2 = floor_area;
    r.heated_area_cm2 = heated_area;
    r.heater_side_cm = roundf(heater_side * 2.0f) / 2.0f;
    r.power_w = power_final;
    r.power_density_w_per_cm2 = density_final;
    r.density_limit_w_per_cm2 = limits.max_density_w_cm2;
    r.voltage_v = voltage;
    r.current_a = current;
    r.resistance_ohm = resistance;
    r.warning_density_high = density_final > limits.max_density_w_cm2 * 0.9f;
    r.warning_density_over = density_final > limits.max_density_w_cm2;
    r.warning_density_near_limit = (density_final > limits.max_density_w_cm2 * 0.95f) ||
                                   (density_final < limits.min_density_w_cm2 * 1.05f);

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
