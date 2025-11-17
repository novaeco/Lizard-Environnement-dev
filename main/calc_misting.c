#include "calc_misting.h"

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

static float coverage_for_env(mist_environment_t env)
{
    switch (env) {
    case MIST_ENV_TROPICAL:
        return 0.08f; // m²/buse (busette fine ~0,1 L/h)
        return 0.08f; // m²/buse
    case MIST_ENV_TEMPERATE_HUMID:
        return 0.10f;
    case MIST_ENV_SEMI_ARID:
        return 0.12f;
    case MIST_ENV_DESERTIC:
        return 0.16f;
    default:
        return 0.10f;
    }
}

bool misting_calculate(const misting_input_t *in, misting_result_t *out)
{
    if (!in || !out) {
        return false;
    }
    if (in->length_cm <= 0.0f || in->depth_cm <= 0.0f || in->nozzle_flow_ml_per_min <= 0.0f ||
        in->cycle_duration_min <= 0.0f || in->cycles_per_day == 0 || in->autonomy_days == 0) {
        return false;
    }

    misting_result_t r = {0};
    const float area_m2 = (in->length_cm * in->depth_cm) / 10000.0f;
    const float coverage = coverage_for_env(in->environment);
    const float nozzle_count = area_m2 / coverage;
    r.nozzle_count = (uint32_t)ceilf(nozzle_count - 1e-3f);

    const float daily_volume_ml = in->nozzle_flow_ml_per_min * in->cycle_duration_min * in->cycles_per_day * r.nozzle_count;
    const float daily_volume_l = daily_volume_ml / 1000.0f;
    r.daily_consumption_l = daily_volume_l;
    r.tank_volume_l = daily_volume_l * (float)in->autonomy_days * 1.2f; // +20% marge
    r.tank_volume_autonomy3_l = daily_volume_l * 3.0f * 1.2f;
    r.tank_volume_autonomy7_l = daily_volume_l * 7.0f * 1.2f;
    r.warning_dense_spray = (r.nozzle_count / fmaxf(area_m2, 0.1f)) > 10.0f; // >10 buses/m² : risque saturation
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
        printf("[TEST brumisation] buses=%u, conso=%.2f L/j, cuve=%.2f L\n",
               out.nozzle_count,
               out.daily_consumption_l,
               out.tank_volume_l);
    }
}

