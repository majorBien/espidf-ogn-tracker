#ifndef __PARAMETERS_WRAPPER_H__
#define __PARAMETERS_WRAPPER_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =====================================================
// Initialization and flash management
// =====================================================

/**
 * Initialize the wrapper. Tries to load parameters from flash.
 * If checksum fails, loads defaults.
 * @return true if parameters loaded from flash, false if defaults used
 */
bool params_init(void);

/**
 * Save current parameters to persistent storage (flash/NVS).
 * @return true on success, false on error
 */
bool params_save_to_flash(void);

/**
 * Load parameters from flash (overwrites current RAM values).
 * @return true if valid parameters loaded, false on checksum error
 */
bool params_load_from_flash(void);

/**
 * Reset all parameters to default values (does not save to flash).
 */
void params_reset_to_defaults(void);

/**
 * Check checksum of current parameters in RAM.
 * @return true if valid, false otherwise
 */
bool params_is_valid(void);

// =====================================================
// GETTERS
// Return internal pointers (read-only usage in HTTP etc.)
// =====================================================

const char* params_get_pilot(void);
const char* params_get_crew(void);
const char* params_get_reg(void);
const char* params_get_base(void);
const char* params_get_manuf(void);
const char* params_get_model(void);
const char* params_get_type(void);
const char* params_get_sn(void);
const char* params_get_id(void);          // Competition ID
const char* params_get_class(void);       // Competition class
const char* params_get_task(void);        // Competition task
const char* params_get_ice(void);         // In Case of Emergency
const char* params_get_pilotid(void);     // Pilot ID (MAC)
const char* params_get_hard(void);        // Hardware version
const char* params_get_soft(void);        // Software version
const char* params_get_btname(void);      // Bluetooth name (if enabled)

// Numeric getters
uint32_t params_get_address(void);
uint8_t  params_get_addr_type(void);
uint8_t  params_get_acft_type(void);
int8_t   params_get_tx_power(void);
uint32_t params_get_console_baud(void);
uint8_t  params_get_console_protocol(void);
uint16_t params_get_tx_prot_mask(void);
uint16_t params_get_rx_prot_mask(void);
int16_t  params_get_freq_corr(void);      // [0.1ppm]
int16_t  params_get_press_corr(void);     // [0.25Pa]
int16_t  params_get_geoid_separ(void);    // [0.1m]
uint8_t  params_get_nav_mode(void);
uint8_t  params_get_nav_rate(void);
uint8_t  params_get_gnss(void);
uint8_t  params_get_verbose(void);
uint8_t  params_get_pps_delay(void);
uint8_t  params_get_initial_page(void);
uint32_t params_get_page_mask(void);
uint8_t  params_get_altitude_unit(void);
uint8_t  params_get_speed_unit(void);
uint8_t  params_get_vario_unit(void);
bool     params_get_encrypt(void);
bool     params_get_stealth(void);
bool     params_get_notrack(void);
uint8_t  params_get_freq_plan(void);
int8_t   params_get_temp_corr(void);
int8_t   params_get_time_corr(void);

// =====================================================
// SETTERS
// Safe string copy into struct fields (RAM only, not saved to flash)
// =====================================================

void params_set_pilot(const char* value);
void params_set_crew(const char* value);
void params_set_reg(const char* value);
void params_set_base(const char* value);
void params_set_manuf(const char* value);
void params_set_model(const char* value);
void params_set_type(const char* value);
void params_set_sn(const char* value);
void params_set_id(const char* value);
void params_set_class(const char* value);
void params_set_task(const char* value);
void params_set_ice(const char* value);
void params_set_pilotid(const char* value);
void params_set_hard(const char* value);
void params_set_soft(const char* value);
void params_set_btname(const char* value);

void params_set_address(uint32_t addr);
void params_set_addr_type(uint8_t type);
void params_set_acft_type(uint8_t type);
void params_set_tx_power(int8_t power);
void params_set_console_baud(uint32_t baud);
void params_set_console_protocol(uint8_t prot);
void params_set_tx_prot_mask(uint16_t mask);
void params_set_rx_prot_mask(uint16_t mask);
void params_set_freq_corr(int16_t corr);
void params_set_press_corr(int16_t corr);
void params_set_geoid_separ(int16_t separ);
void params_set_nav_mode(uint8_t mode);
void params_set_nav_rate(uint8_t rate);
void params_set_gnss(uint8_t gnss);
void params_set_verbose(uint8_t level);
void params_set_pps_delay(uint8_t delay);
void params_set_initial_page(uint8_t page);
void params_set_page_mask(uint32_t mask);
void params_set_altitude_unit(uint8_t unit);
void params_set_speed_unit(uint8_t unit);
void params_set_vario_unit(uint8_t unit);
void params_set_encrypt(bool enable);
void params_set_stealth(bool enable);
void params_set_notrack(bool enable);
void params_set_freq_plan(uint8_t plan);
void params_set_temp_corr(int8_t corr);
void params_set_time_corr(int8_t corr);

// =====================================================
// Advanced: set any parameter by name (like config file)
// =====================================================

/**
 * Set a parameter by name and string value.
 * @param name  parameter name (e.g., "Address", "Pilot", "TxPower")
 * @param value textual value (e.g., "123456", "Jan Kowalski")
 * @return true if parameter recognized and set, false otherwise
 */
bool params_set_by_name(const char* name, const char* value);

/**
 * Write all parameters to a file (if filesystem available).
 * @param filename path (default "/spiffs/TRACKER.CFG")
 * @return number of lines written, 0 on error
 */
int params_write_to_file(const char* filename);

/**
 * Read parameters from a file.
 * @param filename path (default "/spiffs/TRACKER.CFG")
 * @return number of lines read, 0 on error
 */
int params_read_from_file(const char* filename);

#ifdef __cplusplus
}
#endif

#endif // __PARAMETERS_WRAPPER_H__