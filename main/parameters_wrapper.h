#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ===================== GETTERS =====================
// Return pointers to internal FlashParameters fields (read-only access)

const char* params_get_pilot(void);
const char* params_get_crew(void);
const char* params_get_reg(void);
const char* params_get_base(void);
const char* params_get_manuf(void);
const char* params_get_model(void);
const char* params_get_type(void);
const char* params_get_sn(void);

// ===================== SETTERS =====================
// Copy input string into internal FlashParameters fields

void params_set_pilot(const char* value);
void params_set_crew(const char* value);
void params_set_reg(const char* value);
void params_set_base(const char* value);
void params_set_manuf(const char* value);
void params_set_model(const char* value);
void params_set_type(const char* value);
void params_set_sn(const char* value);

#ifdef __cplusplus
}
#endif