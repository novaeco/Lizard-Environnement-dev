#pragma once

#include "calc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MIST_ENV_TROPICAL = 0,
    MIST_ENV_TEMPERATE_HUMID,
    MIST_ENV_SEMI_ARID,
    MIST_ENV_DESERTIC,
    MIST_ENV_COUNT
} mist_environment_t;

typedef struct {
    float length_cm;
    float depth_cm;
    mist_environment_t environment;
    float nozzle_flow_ml_per_min;
    float cycle_duration_min;
    uint32_t cycles_per_day;
    uint32_t autonomy_days;
} misting_input_t;

typedef struct {
    bool valid;
    uint32_t nozzle_count;
    float daily_consumption_l;
    float tank_volume_l;
    float tank_volume_autonomy3_l;
    float tank_volume_autonomy7_l;
    bool warning_dense_spray;
    bool warning_sparse_spray;
    bool warning_flow_out_of_range;
} misting_result_t;

bool misting_calculate(const misting_input_t *in, misting_result_t *out);
void misting_run_self_test(void);

#ifdef __cplusplus
}
#endif

