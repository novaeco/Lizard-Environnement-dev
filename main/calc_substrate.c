#include "calc_substrate.h"

#include <stdio.h>

typedef struct {
    float density_min_kg_per_l;
    float density_max_kg_per_l;
} substrate_density_t;

// Plages issues de fiches techniques :
// - Terreau horticole : 0,65-0,85 kg/L (NF U44-551)
// - Fibre de coco réhydratée : 0,45-0,65 kg/L (bloc 5 kg -> 70-80 L)
// - Mélange forestier (écorce/pin/humus) : 0,60-0,80 kg/L
// - Sable fin : 1,5-1,7 kg/L (EN 13139 granulats)
// - Sable/terre : 1,0-1,3 kg/L (mélange prudent pour drainage)
static const substrate_density_t density_table[SUBSTRATE_COUNT] = {
    [SUBSTRATE_SOIL] = {.density_min_kg_per_l = 0.65f, .density_max_kg_per_l = 0.85f},
    [SUBSTRATE_COCO] = {.density_min_kg_per_l = 0.45f, .density_max_kg_per_l = 0.65f},
    [SUBSTRATE_FOREST_BLEND] = {.density_min_kg_per_l = 0.60f, .density_max_kg_per_l = 0.80f},
    [SUBSTRATE_SAND] = {.density_min_kg_per_l = 1.50f, .density_max_kg_per_l = 1.70f},
    [SUBSTRATE_SAND_SOIL] = {.density_min_kg_per_l = 1.00f, .density_max_kg_per_l = 1.30f},
};

bool substrate_calculate(const substrate_input_t *in, substrate_result_t *out)
{
    if (!in || !out) {
        return false;
    }
    if (in->length_cm < 20.0f || in->depth_cm < 20.0f || in->substrate_height_cm <= 0.0f) {
        return false;
    }

    const substrate_density_t d = density_table[in->type];
    substrate_result_t r = {0};
    const float volume_l = (in->length_cm * in->depth_cm * in->substrate_height_cm) / 1000.0f;
    const float density_mid = (d.density_min_kg_per_l + d.density_max_kg_per_l) * 0.5f;

    r.valid = volume_l > 0.0f;
    r.volume_l = volume_l;
    r.density_min_kg_per_l = d.density_min_kg_per_l;
    r.density_max_kg_per_l = d.density_max_kg_per_l;
    r.density_kg_per_l = density_mid;
    r.mass_min_kg = volume_l * d.density_min_kg_per_l;
    r.mass_max_kg = volume_l * d.density_max_kg_per_l;
    r.mass_kg = volume_l * density_mid;
    r.warning_dimensions_small = (in->length_cm < 40.0f || in->depth_cm < 40.0f);
    r.warning_height_low = in->substrate_height_cm < 3.0f;

    *out = r;
    return true;
}

void substrate_run_self_test(void)
{
    substrate_input_t in = {.length_cm = 100, .depth_cm = 60, .height_cm = 60, .substrate_height_cm = 8, .type = SUBSTRATE_FOREST_BLEND};
    substrate_result_t out = {0};
    if (substrate_calculate(&in, &out)) {
        printf("[TEST substrat] %.1f L, %.1f-%.1f kg (densité %.2f-%.2f kg/L)\n",
               out.volume_l,
               out.mass_min_kg,
               out.mass_max_kg,
               out.density_min_kg_per_l,
               out.density_max_kg_per_l);
    }
}
