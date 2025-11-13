#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Matériau principal du plancher influençant la déperdition thermique.
 */
typedef enum {
    TERRARIUM_MATERIAL_WOOD = 0,   /*!< Bois / OSB scellé */
    TERRARIUM_MATERIAL_GLASS,      /*!< Verre float standard */
    TERRARIUM_MATERIAL_PVC,        /*!< PVC expansé */
    TERRARIUM_MATERIAL_ACRYLIC,    /*!< Acrylique (PMMA) */
    TERRARIUM_MATERIAL_COUNT
} terrarium_material_t;

/**
 * @brief Indicateurs sur la validité des sections de calcul.
 */
typedef enum {
    TERRARIUM_CALC_STATUS_HEATING_VALID   = 1u << 0,
    TERRARIUM_CALC_STATUS_LIGHTING_VALID  = 1u << 1,
    TERRARIUM_CALC_STATUS_UV_VALID        = 1u << 2,
    TERRARIUM_CALC_STATUS_SUBSTRATE_VALID = 1u << 3,
    TERRARIUM_CALC_STATUS_MISTING_VALID   = 1u << 4,
    TERRARIUM_CALC_STATUS_INPUT_CLAMPED   = 1u << 8,
    TERRARIUM_CALC_STATUS_INPUT_INVALID   = 1u << 9,
} terrarium_calc_status_t;

typedef struct {
    /**
     * Longueur intérieure utile en cm (> 0).
     */
    float length_cm;
    /**
     * Largeur intérieure utile en cm (> 0).
     */
    float width_cm;
    /**
     * Hauteur utile en cm (> 0). Utilisée pour ajuster le besoin thermique.
     */
    float height_cm;
    /**
     * Épaisseur moyenne du substrat en cm (>= 0).
     */
    float substrate_thickness_cm;
    terrarium_material_t material;
    /**
     * Éclairement cible sur le plancher (lux).
     */
    float target_lux;
    /**
     * Rendement lumineux des modules LED (lumens/Watt).
     */
    float led_efficiency_lm_per_w;
    /**
     * Puissance électrique d'un module LED (Watt).
     */
    float led_power_per_unit_w;
    /**
     * Intensité UV souhaitée au point chaud (valeur relative ou µW/cm²).
     */
    float uv_target_intensity;
    float uv_module_intensity;
    float mist_density_m2_per_nozzle;
} terrarium_calc_input_t;

typedef struct {
    bool valid;
    float floor_area_cm2;
    float floor_area_m2;
    float heater_target_area_cm2;
    float heater_side_cm;
    float heater_power_raw_w;
    float heater_power_catalog_w;
    float heater_voltage_v;
    float heater_current_a;
    float heater_resistance_ohm;
    float enclosure_volume_l;
} terrarium_heating_result_t;

typedef struct {
    bool valid;
    float luminous_flux_lm;
    float power_w;
    uint32_t led_count;
    float power_per_led_w;
} terrarium_led_result_t;

typedef struct {
    bool valid;
    uint32_t module_count;
} terrarium_uv_result_t;

typedef struct {
    bool valid;
    float volume_l;
} terrarium_substrate_result_t;

typedef struct {
    bool valid;
    uint32_t nozzle_count;
} terrarium_misting_result_t;

typedef struct {
    uint32_t status_flags;
    terrarium_heating_result_t heating;
    terrarium_led_result_t lighting;
    terrarium_uv_result_t uv;
    terrarium_substrate_result_t substrate;
    terrarium_misting_result_t misting;
} terrarium_calc_result_t;

/**
 * @brief Calcule les besoins matériels d'un terrarium.
 *
 * Les entrées sont nettoyées (clamp) dans des bornes sûres avant traitement.
 * La fonction retourne `false` uniquement lorsque les dimensions principales
 * sont hors bornes physiques (> 0). Les autres erreurs sont signalées dans
 * `result->status_flags`.
 */
bool terrarium_calc_compute(const terrarium_calc_input_t *input, terrarium_calc_result_t *result);

#ifdef __cplusplus
}
#endif

