#include "parameters_wrapper.h"

#include <string.h>

#include "hal.h"
#include "parameters.h"
// IMPORTANT:
// We do NOT create FlashParameters here.
// We use the existing global instance defined elsewhere:
extern FlashParameters Parameters;

// =====================================================
// GETTERS
// Return internal pointers (read-only usage in HTTP etc.)
// =====================================================

const char* params_get_pilot(void) { return Parameters.Pilot; }
const char* params_get_crew(void)  { return Parameters.Crew; }
const char* params_get_reg(void)   { return Parameters.Reg; }
const char* params_get_base(void)  { return Parameters.Base; }
const char* params_get_manuf(void){ return Parameters.Manuf; }
const char* params_get_model(void){ return Parameters.Model; }
const char* params_get_type(void)  { return Parameters.Type; }
const char* params_get_sn(void)    { return Parameters.SN; }

// =====================================================
// SETTERS
// Safe string copy into struct fields
// =====================================================

static void safe_copy(char* dst, const char* src, size_t size)
{
    if (!dst || !src || size == 0) return;
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}

void params_set_pilot(const char* value) { safe_copy(Parameters.Pilot, value, sizeof(Parameters.Pilot)); }
void params_set_crew(const char* value)  { safe_copy(Parameters.Crew, value, sizeof(Parameters.Crew)); }
void params_set_reg(const char* value)   { safe_copy(Parameters.Reg, value, sizeof(Parameters.Reg)); }
void params_set_base(const char* value)  { safe_copy(Parameters.Base, value, sizeof(Parameters.Base)); }
void params_set_manuf(const char* value) { safe_copy(Parameters.Manuf, value, sizeof(Parameters.Manuf)); }
void params_set_model(const char* value) { safe_copy(Parameters.Model, value, sizeof(Parameters.Model)); }
void params_set_type(const char* value)  { safe_copy(Parameters.Type, value, sizeof(Parameters.Type)); }
void params_set_sn(const char* value)    { safe_copy(Parameters.SN, value, sizeof(Parameters.SN)); }