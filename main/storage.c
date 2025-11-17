#include "storage.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "storage";
static const char *NAMESPACE = "calc";

static esp_err_t save_blob(const char *key, const void *data, size_t len)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_blob(h, key, data, len);
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return err;
}

static esp_err_t load_blob(const char *key, void *data, size_t len)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) {
        return err;
    }
    size_t out_len = len;
    err = nvs_get_blob(h, key, data, &out_len);
    nvs_close(h);
    if (err == ESP_OK && out_len != len) {
        return ESP_ERR_INVALID_SIZE;
    }
    return err;
}

esp_err_t storage_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t storage_reset_nvs(void)
{
    esp_err_t err = nvs_flash_erase();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS erase failed: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS re-init failed after erase: %s", esp_err_to_name(err));
    }
    return err;
}

typedef struct __attribute__((packed)) {
    float length_cm;
    float depth_cm;
    float height_cm;
    float ratio;
    uint8_t material;
} pad_blob_t;

esp_err_t storage_load_heating_pad(heating_pad_input_t *in)
{
    if (!in) {
        return ESP_ERR_INVALID_ARG;
    }
    pad_blob_t blob = {0};
    esp_err_t err = load_blob("pad", &blob, sizeof(blob));
    if (err != ESP_OK) {
        in->length_cm = 100.0f;
        in->depth_cm = 60.0f;
        in->height_cm = 60.0f;
        in->heated_ratio = 0.33f;
        in->material = TERRARIUM_MATERIAL_GLASS;
        return err;
    }
    in->length_cm = blob.length_cm;
    in->depth_cm = blob.depth_cm;
    in->height_cm = blob.height_cm;
    in->heated_ratio = blob.ratio;
    in->material = (terrarium_material_t)blob.material;
    return ESP_OK;
}

esp_err_t storage_save_heating_pad(const heating_pad_input_t *in)
{
    if (!in) {
        return ESP_ERR_INVALID_ARG;
    }
    pad_blob_t blob = {
        .length_cm = in->length_cm,
        .depth_cm = in->depth_cm,
        .height_cm = in->height_cm,
        .ratio = in->heated_ratio,
        .material = (uint8_t)in->material,
    };
    return save_blob("pad", &blob, sizeof(blob));
}

typedef struct __attribute__((packed)) {
    float length_cm;
    float depth_cm;
    float ratio;
    float power_linear_w_per_m;
    float supply_voltage_v;
    float target_density;
    float spacing_cm;
    uint8_t material;
} cable_blob_t;

esp_err_t storage_load_heating_cable(heating_cable_input_t *in)
{
    if (!in) {
        return ESP_ERR_INVALID_ARG;
    }
    cable_blob_t blob = {0};
    esp_err_t err = load_blob("cable", &blob, sizeof(blob));
    if (err != ESP_OK) {
        in->length_cm = 120.0f;
        in->depth_cm = 60.0f;
        in->heated_ratio = 0.33f;
        in->power_linear_w_per_m = 20.0f;
        in->supply_voltage_v = 24.0f;
        in->target_power_density_w_per_cm2 = 0.035f;
        in->spacing_cm = 4.0f;
        in->material = TERRARIUM_MATERIAL_GLASS;
        return err;
    }
    in->length_cm = blob.length_cm;
    in->depth_cm = blob.depth_cm;
    in->heated_ratio = blob.ratio;
    in->power_linear_w_per_m = blob.power_linear_w_per_m;
    in->supply_voltage_v = blob.supply_voltage_v;
    in->target_power_density_w_per_cm2 = blob.target_density;
    in->spacing_cm = blob.spacing_cm;
    in->material = (terrarium_material_t)blob.material;
    return ESP_OK;
}

esp_err_t storage_save_heating_cable(const heating_cable_input_t *in)
{
    if (!in) {
        return ESP_ERR_INVALID_ARG;
    }
    cable_blob_t blob = {
        .length_cm = in->length_cm,
        .depth_cm = in->depth_cm,
        .ratio = in->heated_ratio,
        .power_linear_w_per_m = in->power_linear_w_per_m,
        .supply_voltage_v = in->supply_voltage_v,
        .target_density = in->target_power_density_w_per_cm2,
        .spacing_cm = in->spacing_cm,
        .material = (uint8_t)in->material,
    };
    return save_blob("cable", &blob, sizeof(blob));
}

typedef struct __attribute__((packed)) {
    float length_cm;
    float depth_cm;
    float height_cm;
    uint8_t env;
    float led_flux;
    float led_power;
    float uva;
    float uvb;
    float ref_dist;
} light_blob_t;

esp_err_t storage_load_lighting(lighting_input_t *in)
{
    if (!in) {
        return ESP_ERR_INVALID_ARG;
    }
    light_blob_t blob = {0};
    esp_err_t err = load_blob("light", &blob, sizeof(blob));
    if (err != ESP_OK) {
        in->length_cm = 100.0f;
        in->depth_cm = 60.0f;
        in->height_cm = 60.0f;
        in->environment = TERRARIUM_ENV_DESERTIC;
        in->led_luminous_flux_lm = 150.0f;
        in->led_power_w = 1.0f;
        in->uva_irradiance_mw_cm2_at_distance = 3.0f;
        in->uvb_uvi_at_distance = 1.2f;
        in->reference_distance_cm = 30.0f;
        return err;
    }
    in->length_cm = blob.length_cm;
    in->depth_cm = blob.depth_cm;
    in->height_cm = blob.height_cm;
    in->environment = (terrarium_environment_t)blob.env;
    in->led_luminous_flux_lm = blob.led_flux;
    in->led_power_w = blob.led_power;
    in->uva_irradiance_mw_cm2_at_distance = blob.uva;
    in->uvb_uvi_at_distance = blob.uvb;
    in->reference_distance_cm = blob.ref_dist;
    return ESP_OK;
}

esp_err_t storage_save_lighting(const lighting_input_t *in)
{
    if (!in) {
        return ESP_ERR_INVALID_ARG;
    }
    light_blob_t blob = {
        .length_cm = in->length_cm,
        .depth_cm = in->depth_cm,
        .height_cm = in->height_cm,
        .env = (uint8_t)in->environment,
        .led_flux = in->led_luminous_flux_lm,
        .led_power = in->led_power_w,
        .uva = in->uva_irradiance_mw_cm2_at_distance,
        .uvb = in->uvb_uvi_at_distance,
        .ref_dist = in->reference_distance_cm,
    };
    return save_blob("light", &blob, sizeof(blob));
}

typedef struct __attribute__((packed)) {
    float length_cm;
    float depth_cm;
    float height_cm;
    float substrate_height_cm;
    uint8_t type;
} substrate_blob_t;

esp_err_t storage_load_substrate(substrate_input_t *in)
{
    if (!in) {
        return ESP_ERR_INVALID_ARG;
    }
    substrate_blob_t blob = {0};
    esp_err_t err = load_blob("substrate", &blob, sizeof(blob));
    if (err != ESP_OK) {
        in->length_cm = 100.0f;
        in->depth_cm = 60.0f;
        in->height_cm = 60.0f;
        in->substrate_height_cm = 8.0f;
        in->type = SUBSTRATE_FOREST_BLEND;
        return err;
    }
    in->length_cm = blob.length_cm;
    in->depth_cm = blob.depth_cm;
    in->height_cm = blob.height_cm;
    in->substrate_height_cm = blob.substrate_height_cm;
    in->type = (substrate_type_t)blob.type;
    return ESP_OK;
}

esp_err_t storage_save_substrate(const substrate_input_t *in)
{
    if (!in) {
        return ESP_ERR_INVALID_ARG;
    }
    substrate_blob_t blob = {
        .length_cm = in->length_cm,
        .depth_cm = in->depth_cm,
        .height_cm = in->height_cm,
        .substrate_height_cm = in->substrate_height_cm,
        .type = (uint8_t)in->type,
    };
    return save_blob("substrate", &blob, sizeof(blob));
}

typedef struct __attribute__((packed)) {
    float length_cm;
    float depth_cm;
    uint8_t env;
    float nozzle_flow;
    float cycle_duration;
    uint32_t cycles_per_day;
    uint32_t autonomy_days;
} misting_blob_t;

esp_err_t storage_load_misting(misting_input_t *in)
{
    if (!in) {
        return ESP_ERR_INVALID_ARG;
    }
    misting_blob_t blob = {0};
    esp_err_t err = load_blob("misting", &blob, sizeof(blob));
    if (err != ESP_OK) {
        in->length_cm = 120.0f;
        in->depth_cm = 60.0f;
        in->environment = MIST_ENV_TROPICAL;
        in->nozzle_flow_ml_per_min = 80.0f;
        in->cycle_duration_min = 2.0f;
        in->cycles_per_day = 4;
        in->autonomy_days = 3;
        return err;
    }
    in->length_cm = blob.length_cm;
    in->depth_cm = blob.depth_cm;
    in->environment = (mist_environment_t)blob.env;
    in->nozzle_flow_ml_per_min = blob.nozzle_flow;
    in->cycle_duration_min = blob.cycle_duration;
    in->cycles_per_day = blob.cycles_per_day;
    in->autonomy_days = blob.autonomy_days;
    return ESP_OK;
}

esp_err_t storage_save_misting(const misting_input_t *in)
{
    if (!in) {
        return ESP_ERR_INVALID_ARG;
    }
    misting_blob_t blob = {
        .length_cm = in->length_cm,
        .depth_cm = in->depth_cm,
        .env = (uint8_t)in->environment,
        .nozzle_flow = in->nozzle_flow_ml_per_min,
        .cycle_duration = in->cycle_duration_min,
        .cycles_per_day = in->cycles_per_day,
        .autonomy_days = in->autonomy_days,
    };
    return save_blob("misting", &blob, sizeof(blob));
}

