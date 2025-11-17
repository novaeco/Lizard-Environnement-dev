#include "calc_substrate.h"

#include <stdio.h>

static float density_for_type(substrate_type_t type)
{
    switch (type) {
    case SUBSTRATE_SOIL:
        return 1.1f; // terreau humide compacté
    case SUBSTRATE_COCO:
        return 0.45f; // fibre coco réhydratée
    case SUBSTRATE_FOREST_BLEND:
        return 0.75f;
    case SUBSTRATE_SAND:
        return 1.6f;
    case SUBSTRATE_SAND_SOIL:
        return 1.3f;
    default:
        return 1.0f;
    }
}

bool substrate_calculate(const substrate_input_t *in, substrate_result_t *out)
{
    if (!in || !out) {
        return false;
    }
    if (in->length_cm <= 0.0f || in->depth_cm <= 0.0f || in->substrate_height_cm < 0.0f) {
        return false;
    }

    substrate_result_t r = {0};
    const float volume_l = (in->length_cm * in->depth_cm * in->substrate_height_cm) / 1000.0f;
    const float density = density_for_type(in->type);
    r.valid = volume_l > 0.0f;
    r.volume_l = volume_l;
    r.density_kg_per_l = density;
    r.mass_kg = volume_l * density;

    *out = r;
    return true;
}

void substrate_run_self_test(void)
{
    substrate_input_t in = {.length_cm = 100, .depth_cm = 60, .height_cm = 60, .substrate_height_cm = 8, .type = SUBSTRATE_FOREST_BLEND};
    substrate_result_t out = {0};
    if (substrate_calculate(&in, &out)) {
        printf("[TEST substrat] %.1f L, %.1f kg\n", out.volume_l, out.mass_kg);
    }
}

