#pragma once

#include "calc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SUBSTRATE_SOIL = 0,
    SUBSTRATE_COCO,
    SUBSTRATE_FOREST_BLEND,
    SUBSTRATE_SAND,
    SUBSTRATE_SAND_SOIL,
    SUBSTRATE_COUNT
} substrate_type_t;

typedef struct {
    float length_cm;
    float depth_cm;
    float height_cm;
    float substrate_height_cm;
    substrate_type_t type;
} substrate_input_t;

typedef struct {
    bool valid;
    float volume_l;
    float mass_kg;
    float mass_min_kg;
    float mass_max_kg;
    float density_kg_per_l;
    float density_min_kg_per_l;
    float density_max_kg_per_l;
} substrate_result_t;

bool substrate_calculate(const substrate_input_t *in, substrate_result_t *out);
void substrate_run_self_test(void);

#ifdef __cplusplus
}
#endif

