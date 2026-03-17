#ifndef APP_SYSTEM_H
#define APP_SYSTEM_H

#include "app_state.h"

void print_sr(void);
int16_t calculate_roll(void);
uint8_t find_first_active_table(void);
uint8_t find_next_active_table(void);
void prt_stt(void);
float calibrate_angle(void);

#endif
