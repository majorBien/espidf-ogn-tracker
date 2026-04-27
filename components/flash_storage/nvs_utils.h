/*
 * nvs_utils.h
 *
 *  Created on: 12 lip 2025
 *      Author: majorBien
 */

#ifndef MAIN_SYSTEM_NVS_UTILS_H_
#define MAIN_SYSTEM_NVS_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"


// User WiFi configuration structure
typedef struct {
    char ssid[33];            // 32 chars + null
    char password[65];        // 64 chars + null
} nvs_network_data_t;


// Initialize NVS storage
esp_err_t nvs_init_storage(void);

// wifi data operations
esp_err_t nvs_save_network_data(const nvs_network_data_t *data);
esp_err_t nvs_load_network_data(nvs_network_data_t *data);

#ifdef __cplusplus
}
#endif
#endif /* MAIN_SYSTEM_NVS_UTILS_H_ */
