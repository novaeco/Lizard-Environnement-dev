#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TERRARIUM_MATERIAL_WOOD = 0,
    TERRARIUM_MATERIAL_GLASS,
    TERRARIUM_MATERIAL_PVC,
    TERRARIUM_MATERIAL_ACRYLIC,
    TERRARIUM_MATERIAL_COUNT
} terrarium_material_t;

typedef enum {
    TERRARIUM_ENV_TROPICAL = 0,
    TERRARIUM_ENV_DESERTIC,
    TERRARIUM_ENV_TEMPERATE_FOREST,
    TERRARIUM_ENV_NOCTURNAL,
    TERRARIUM_ENV_COUNT
} terrarium_environment_t;

#ifdef __cplusplus
}
#endif

