#include "calc_misting.h"

#include <math.h>
#include <stdio.h>

typedef struct {
    float coverage_min_m2_per_nozzle;
    float coverage_max_m2_per_nozzle;
} mist_coverage_t;

// Couverture issue datasheets MistKing/ExoTerra 0,08-0,16 m² par buse
static const mist_coverage_t coverage_table[MIST_ENV_COUNT] = {
    [MIST_ENV_TROPICAL] = {.coverage_min_m2_per_nozzle = 0.08f, .coverage_max_m2_per_nozzle = 0.12f},
    [MIST_ENV_TEMPERATE_HUMID] = {.coverage_min_m2_per_nozzle = 0.10f, .coverage_max_m2_per_nozzle = 0.14f},
    [MIST_ENV_SEMI_ARID] = {.coverage_min_m2_per_nozzle = 0.12f, .coverage_max_m2_per_nozzle = 0.16f},
    [MIST_ENV_DESERTIC] = {.coverage_min_m2_per_nozzle = 0.14f, .coverage_max_m2_per_nozzle = 0.18f},
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

bool misting_calculate(const misting_input_t *in, misting_result_t *out)
{
    if (!in || !out) {
        return false;
    }
    if (in->length_cm < 20.0f || in->depth_cm < 20.0f || in->nozzle_flow_ml_per_min <= 0.0f ||
        in->cycle_duration_min <= 0.0f || in->cycles_per_day == 0 || in->autonomy_days == 0) {
        return false;
    }

    const mist_coverage_t cov = coverage_table[in->environment];
    const float area_m2 = (in->length_cm * in->depth_cm) / 10000.0f;
    const float coverage_mid = (cov.coverage_min_m2_per_nozzle + cov.coverage_max_m2_per_nozzle) * 0.5f;

    misting_result_t r = {0};
    const float nozzle_count_exact = area_m2 / coverage_mid;
    r.nozzle_count = (uint32_t)ceilf(nozzle_count_exact - 1e-3f);

    const float daily_volume_ml = in->nozzle_flow_ml_per_min * in->cycle_duration_min * in->cycles_per_day * r.nozzle_count;
    const float daily_volume_l = daily_volume_ml / 1000.0f;
    r.daily_consumption_l = daily_volume_l;
    r.tank_volume_l = daily_volume_l * (float)in->autonomy_days * 1.2f; // +20% marge anti-désamorçage
    r.tank_volume_autonomy3_l = daily_volume_l * 3.0f * 1.2f;
    r.tank_volume_autonomy7_l = daily_volume_l * 7.0f * 1.2f;

    const float nozzle_density = r.nozzle_count / fmaxf(area_m2, 0.1f);
    r.warning_dense_spray = nozzle_density > (1.0f / cov.coverage_min_m2_per_nozzle);
    r.warning_sparse_spray = nozzle_density < (1.0f / cov.coverage_max_m2_per_nozzle) * 0.6f;
    r.warning_flow_out_of_range = (in->nozzle_flow_ml_per_min < 60.0f || in->nozzle_flow_ml_per_min > 120.0f);
    r.valid = r.nozzle_count > 0;

    *out = r;
    return true;
}

void misting_run_self_test(void)
{
    misting_input_t in = {
        .length_cm = 120,
        .depth_cm = 60,
        .environment = MIST_ENV_TROPICAL,
        .nozzle_flow_ml_per_min = 80.0f,
        .cycle_duration_min = 2.0f,
        .cycles_per_day = 4,
        .autonomy_days = 3,
    };
    misting_result_t out = {0};
    if (misting_calculate(&in, &out)) {
        printf("[TEST brumisation] buses=%u, conso=%.2f L/j, cuve %.2f L (3j=%.2f L, 7j=%.2f L)\n",
               out.nozzle_count,
               out.daily_consumption_l,
               out.tank_volume_l,
               out.tank_volume_autonomy3_l,
               out.tank_volume_autonomy7_l);
    }
}
