#include "parameters_wrapper.h"

#include <string.h>
#include <stdio.h>

#include "hal.h"
#include "parameters.h"

// IMPORTANT:
// We do NOT create FlashParameters here.
// We use the existing global instance defined elsewhere:
extern FlashParameters Parameters;

// =====================================================
// Helper: safe copy with null termination
// =====================================================
static void safe_copy(char* dst, const char* src, size_t size)
{
    if (!dst || !src || size == 0) return;
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}

// =====================================================
// Initialization and flash management
// =====================================================

bool params_init(void)
{
    if (params_load_from_flash()) {
        return true;
    }
    params_reset_to_defaults();
    return false;
}

bool params_save_to_flash(void)
{
    Parameters.setCheckSum();
#ifdef WITH_ESP32
    esp_err_t err = Parameters.WriteToNVS();
    return (err == ESP_OK);
#elif defined(WITH_STM32) || defined(WITH_SAMD21)
    return (Parameters.WriteToFlash() == 0);
#else
    // No flash implementation available
    return false;
#endif
}

bool params_load_from_flash(void)
{
#ifdef WITH_ESP32
    esp_err_t err = Parameters.ReadFromNVS();
    if (err == ESP_OK && Parameters.goodCheckSum()) {
        return true;
    }
#elif defined(WITH_STM32) || defined(WITH_SAMD21)
    if (Parameters.ReadFromFlash() == 1 && Parameters.goodCheckSum()) {
        return true;
    }
#endif
    // If flash read fails, check current RAM checksum
    return Parameters.goodCheckSum();
}

void params_reset_to_defaults(void)
{
    Parameters.setDefault();
    Parameters.SaveToFlash = 0;  // Clear save flag
}

bool params_is_valid(void)
{
    return Parameters.goodCheckSum();
}

// =====================================================
// GETTERS
// Return internal pointers (read-only usage in HTTP etc.)
// =====================================================

const char* params_get_pilot(void)  { return Parameters.Pilot; }
const char* params_get_crew(void)   { return Parameters.Crew; }
const char* params_get_reg(void)    { return Parameters.Reg; }
const char* params_get_base(void)   { return Parameters.Base; }
const char* params_get_manuf(void)  { return Parameters.Manuf; }
const char* params_get_model(void)  { return Parameters.Model; }
const char* params_get_type(void)   { return Parameters.Type; }
const char* params_get_sn(void)     { return Parameters.SN; }
const char* params_get_id(void)     { return Parameters.ID; }
const char* params_get_class(void)  { return Parameters.Class; }
const char* params_get_task(void)   { return Parameters.Task; }
const char* params_get_ice(void)    { return Parameters.ICE; }
const char* params_get_pilotid(void){ return Parameters.PilotID; }
const char* params_get_hard(void)   { return Parameters.Hard; }
const char* params_get_soft(void)   { return Parameters.Soft; }
#if defined(WITH_BT_SPP) || defined(WITH_BLE_SPP)
const char* params_get_btname(void) { return Parameters.BTname; }
#else
const char* params_get_btname(void) { return ""; }
#endif

uint32_t params_get_address(void)   { return Parameters.Address; }
uint8_t  params_get_addr_type(void) { return Parameters.AddrType; }
uint8_t  params_get_acft_type(void) { return Parameters.AcftType; }
int8_t   params_get_tx_power(void)  { return Parameters.TxPower; }
uint32_t params_get_console_baud(void) { return Parameters.CONbaud; }
uint8_t  params_get_console_protocol(void) { return Parameters.CONprot; }
uint16_t params_get_tx_prot_mask(void) { return Parameters.TxProtMask; }
uint16_t params_get_rx_prot_mask(void) { return Parameters.RxProtMask; }
int16_t  params_get_freq_corr(void)  { return Parameters.RFchipFreqCorr; }
int16_t  params_get_press_corr(void) { return Parameters.PressCorr; }
int16_t  params_get_geoid_separ(void){ return Parameters.GeoidSepar; }
uint8_t  params_get_nav_mode(void)   { return Parameters.NavMode; }
uint8_t  params_get_nav_rate(void)   { return Parameters.NavRate; }
uint8_t  params_get_gnss(void)       { return Parameters.GNSS; }
uint8_t  params_get_verbose(void)    { return Parameters.Verbose; }
uint8_t  params_get_pps_delay(void)  { return Parameters.PPSdelay; }
uint8_t  params_get_initial_page(void) { return Parameters.InitialPage; }
uint32_t params_get_page_mask(void)  { return Parameters.PageMask; }
uint8_t  params_get_altitude_unit(void) { return Parameters.AltitudeUnit; }
uint8_t  params_get_speed_unit(void) { return Parameters.SpeedUnit; }
uint8_t  params_get_vario_unit(void) { return Parameters.VarioUnit; }
bool     params_get_encrypt(void)    { return Parameters.Encrypt; }
bool     params_get_stealth(void)    { return Parameters.Stealth; }
bool     params_get_notrack(void)    { return Parameters.NoTrack; }
uint8_t  params_get_freq_plan(void)  { return Parameters.FreqPlan; }
int8_t   params_get_temp_corr(void)  { return Parameters.RFchipTempCorr; }
int8_t   params_get_time_corr(void)  { return Parameters.TimeCorr; }

// =====================================================
// SETTERS
// Safe string copy into struct fields
// =====================================================

void params_set_pilot(const char* value) { safe_copy(Parameters.Pilot, value, sizeof(Parameters.Pilot)); }
void params_set_crew(const char* value)  { safe_copy(Parameters.Crew, value, sizeof(Parameters.Crew)); }
void params_set_reg(const char* value)   { safe_copy(Parameters.Reg, value, sizeof(Parameters.Reg)); }
void params_set_base(const char* value)  { safe_copy(Parameters.Base, value, sizeof(Parameters.Base)); }
void params_set_manuf(const char* value) { safe_copy(Parameters.Manuf, value, sizeof(Parameters.Manuf)); }
void params_set_model(const char* value) { safe_copy(Parameters.Model, value, sizeof(Parameters.Model)); }
void params_set_type(const char* value)  { safe_copy(Parameters.Type, value, sizeof(Parameters.Type)); }
void params_set_sn(const char* value)    { safe_copy(Parameters.SN, value, sizeof(Parameters.SN)); }
void params_set_id(const char* value)    { safe_copy(Parameters.ID, value, sizeof(Parameters.ID)); }
void params_set_class(const char* value) { safe_copy(Parameters.Class, value, sizeof(Parameters.Class)); }
void params_set_task(const char* value)  { safe_copy(Parameters.Task, value, sizeof(Parameters.Task)); }
void params_set_ice(const char* value)   { safe_copy(Parameters.ICE, value, sizeof(Parameters.ICE)); }
void params_set_pilotid(const char* value){ safe_copy(Parameters.PilotID, value, sizeof(Parameters.PilotID)); }
void params_set_hard(const char* value)  { safe_copy(Parameters.Hard, value, sizeof(Parameters.Hard)); }
void params_set_soft(const char* value)  { safe_copy(Parameters.Soft, value, sizeof(Parameters.Soft)); }
#if defined(WITH_BT_SPP) || defined(WITH_BLE_SPP)
void params_set_btname(const char* value){ safe_copy(Parameters.BTname, value, sizeof(Parameters.BTname)); }
#else
void params_set_btname(const char* value) { (void)value; }
#endif

void params_set_address(uint32_t addr)   { Parameters.Address = addr; }
void params_set_addr_type(uint8_t type)  { Parameters.AddrType = type; }
void params_set_acft_type(uint8_t type)  { Parameters.AcftType = type; }
void params_set_tx_power(int8_t power)   { Parameters.TxPower = power; }
void params_set_console_baud(uint32_t baud) { Parameters.CONbaud = baud; }
void params_set_console_protocol(uint8_t prot) { Parameters.CONprot = prot; }
void params_set_tx_prot_mask(uint16_t mask) { Parameters.TxProtMask = mask; }
void params_set_rx_prot_mask(uint16_t mask) { Parameters.RxProtMask = mask; }
void params_set_freq_corr(int16_t corr)  { Parameters.RFchipFreqCorr = corr; }
void params_set_press_corr(int16_t corr) { Parameters.PressCorr = corr; }
void params_set_geoid_separ(int16_t separ){ Parameters.GeoidSepar = separ; }
void params_set_nav_mode(uint8_t mode)   { Parameters.NavMode = mode; }
void params_set_nav_rate(uint8_t rate)   { Parameters.NavRate = rate; }
void params_set_gnss(uint8_t gnss)       { Parameters.GNSS = gnss; }
void params_set_verbose(uint8_t level)   { Parameters.Verbose = level; }
void params_set_pps_delay(uint8_t delay) { Parameters.PPSdelay = delay; }
void params_set_initial_page(uint8_t page) { Parameters.InitialPage = page; }
void params_set_page_mask(uint32_t mask) { Parameters.PageMask = mask; }
void params_set_altitude_unit(uint8_t unit) { Parameters.AltitudeUnit = unit; }
void params_set_speed_unit(uint8_t unit) { Parameters.SpeedUnit = unit; }
void params_set_vario_unit(uint8_t unit) { Parameters.VarioUnit = unit; }
void params_set_encrypt(bool enable)     { Parameters.Encrypt = enable ? 1 : 0; }
void params_set_stealth(bool enable)     { Parameters.Stealth = enable ? 1 : 0; }
void params_set_notrack(bool enable)     { Parameters.NoTrack = enable ? 1 : 0; }
void params_set_freq_plan(uint8_t plan)  { Parameters.FreqPlan = plan; }
void params_set_temp_corr(int8_t corr)   { Parameters.RFchipTempCorr = corr; }
void params_set_time_corr(int8_t corr)   { Parameters.TimeCorr = corr; }

// =====================================================
// Set by name (using the existing FlashParameters::ReadParam)
// =====================================================

bool params_set_by_name(const char* name, const char* value)
{
    // Build a line in the format "name = value" and use the existing parser
    char line[128];
    snprintf(line, sizeof(line), "%s = %s", name, value);
    return Parameters.ReadLine(line);
}

// =====================================================
// File I/O using existing FlashParameters methods
// =====================================================

int params_write_to_file(const char* filename)
{
    if (!filename) filename = "/spiffs/TRACKER.CFG";
    return Parameters.WriteToFile(filename);
}

int params_read_from_file(const char* filename)
{
    if (!filename) filename = "/spiffs/TRACKER.CFG";
    return Parameters.ReadFromFile(filename);
}