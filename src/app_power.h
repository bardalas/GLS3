#ifndef APP_POWER_H
#define APP_POWER_H

#include "app_state.h"

void app_power_initialize(void);
void app_power_process(void);
void app_power_request_sleep(void);
bool app_power_is_active(void);

void goto_sleep(void);
void configure_button_to_io(void);
void configure_button_to_interrupt(void);
void enterSleepMode(void);
void sw_to_sosc(void);
void sw_to_posc(void);

#endif
