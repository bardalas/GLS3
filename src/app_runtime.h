#ifndef APP_RUNTIME_H
#define APP_RUNTIME_H

#include "app_state.h"

void app_initialize(void);
void app_run_once(void);

void all_init(void);
void app_process(void);

uint32_t app_now_ms(void);
void app_register_activity(void);

void __delay_ms(uint32_t ms);
void __delay_us(uint32_t us);

#endif
