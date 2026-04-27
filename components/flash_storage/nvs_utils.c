/*
 * nvs_utils.c
 *
 *  Created on: 12 lip 2025
 *      Author: majorBien
 */

#include "nvs_utils.h"
#include "nvs_utils.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "string.h"

#define TAG "NVS_UTILS"

#define NVS_NAMESPACE "device_storage"
#define CONST_DATA_KEY "device_config"
#define USER_DATA_KEY "wifi_config"

esp_err_t nvs_init_storage(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

esp_err_t nvs_save_network_data(const nvs_network_data_t *data) {
    if (data == NULL) {
        ESP_LOGE(TAG, "Invalid data pointer");
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(handle, USER_DATA_KEY, data, sizeof(nvs_network_data_t));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);
    return err;
}

esp_err_t nvs_load_network_data(nvs_network_data_t *data) {
    if (data == NULL) {
        ESP_LOGE(TAG, "Invalid data pointer");
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_size = sizeof(nvs_network_data_t);
    err = nvs_get_blob(handle, USER_DATA_KEY, data, &required_size);

    nvs_close(handle);
    return err;
}
