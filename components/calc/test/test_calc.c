#include "calc.h"

#include "unity.h"

#include <stdbool.h>

static void expect_flag(uint32_t flags, uint32_t flag, bool state)
{
    if (state) {
        TEST_ASSERT_MESSAGE(flags & flag, "expected flag set");
    } else {
        TEST_ASSERT_MESSAGE(!(flags & flag), "expected flag cleared");
    }
}

TEST_CASE("basic heating computation", "[calc]")
{
    terrarium_calc_input_t input = {
        .length_cm = 120.0f,
        .width_cm = 60.0f,
        .height_cm = 60.0f,
        .substrate_thickness_cm = 8.0f,
        .material = TERRARIUM_MATERIAL_GLASS,
        .target_lux = 12000.0f,
        .led_efficiency_lm_per_w = 160.0f,
        .led_power_per_unit_w = 5.0f,
        .uv_target_intensity = 150.0f,
        .uv_module_intensity = 75.0f,
        .mist_density_m2_per_nozzle = 0.1f,
    };

    terrarium_calc_result_t result = {0};
    TEST_ASSERT_TRUE(terrarium_calc_compute(&input, &result));

    expect_flag(result.status_flags, TERRARIUM_CALC_STATUS_HEATING_VALID, true);
    expect_flag(result.status_flags, TERRARIUM_CALC_STATUS_LIGHTING_VALID, true);
    expect_flag(result.status_flags, TERRARIUM_CALC_STATUS_UV_VALID, true);
    expect_flag(result.status_flags, TERRARIUM_CALC_STATUS_SUBSTRATE_VALID, true);
    expect_flag(result.status_flags, TERRARIUM_CALC_STATUS_MISTING_VALID, true);

    TEST_ASSERT_TRUE(result.heating.valid);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.0f, result.heating.heater_power_catalog_w);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.0f, result.heating.enclosure_volume_l);
    TEST_ASSERT_TRUE(result.lighting.valid);
    TEST_ASSERT_GREATER_THAN_UINT32(0u, result.lighting.led_count);
}

TEST_CASE("input clamping", "[calc]")
{
    terrarium_calc_input_t input = {
        .length_cm = 2.0f,
        .width_cm = 500.0f,
        .height_cm = -5.0f,
        .substrate_thickness_cm = 80.0f,
        .material = TERRARIUM_MATERIAL_WOOD,
        .target_lux = 0.0f,
        .led_efficiency_lm_per_w = 400.0f,
        .led_power_per_unit_w = 120.0f,
        .uv_target_intensity = 0.0f,
        .uv_module_intensity = 0.0f,
        .mist_density_m2_per_nozzle = 5.0f,
    };

    terrarium_calc_result_t result = {0};
    TEST_ASSERT_FALSE(terrarium_calc_compute(&input, &result));

    /* Corriger la hauteur pour permettre le calcul et vérifier le clamp. */
    input.height_cm = 50.0f;
    TEST_ASSERT_TRUE(terrarium_calc_compute(&input, &result));
    expect_flag(result.status_flags, TERRARIUM_CALC_STATUS_INPUT_CLAMPED, true);
    TEST_ASSERT_TRUE(result.heating.valid);
    TEST_ASSERT_FALSE(result.lighting.valid);
    TEST_ASSERT_FALSE(result.uv.valid);
    TEST_ASSERT_TRUE(result.substrate.valid);
    TEST_ASSERT_TRUE(result.misting.valid);
    expect_flag(result.status_flags, TERRARIUM_CALC_STATUS_INPUT_INVALID, true);
}

TEST_CASE("invalid optional sections flagged", "[calc]")
{
    terrarium_calc_input_t input = {
        .length_cm = 150.0f,
        .width_cm = 70.0f,
        .height_cm = 60.0f,
        .substrate_thickness_cm = 0.0f,
        .material = TERRARIUM_MATERIAL_GLASS,
        .target_lux = 0.0f,
        .led_efficiency_lm_per_w = 180.0f,
        .led_power_per_unit_w = 10.0f,
        .uv_target_intensity = -10.0f,
        .uv_module_intensity = 50.0f,
        .mist_density_m2_per_nozzle = 0.0f,
    };

    terrarium_calc_result_t result = {0};
    TEST_ASSERT_TRUE(terrarium_calc_compute(&input, &result));

    expect_flag(result.status_flags, TERRARIUM_CALC_STATUS_LIGHTING_VALID, false);
    expect_flag(result.status_flags, TERRARIUM_CALC_STATUS_UV_VALID, false);
    expect_flag(result.status_flags, TERRARIUM_CALC_STATUS_SUBSTRATE_VALID, false);
    expect_flag(result.status_flags, TERRARIUM_CALC_STATUS_MISTING_VALID, false);
    expect_flag(result.status_flags, TERRARIUM_CALC_STATUS_INPUT_INVALID, true);
    expect_flag(result.status_flags, TERRARIUM_CALC_STATUS_INPUT_CLAMPED, true);
}
