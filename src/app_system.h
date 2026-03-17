#ifndef APP_SYSTEM_H
#define APP_SYSTEM_H

#include "app_display.h"
#include "app_ballistics.h"

void all_init(void);
void app_process(void);
uint32_t app_now_ms(void);
void app_register_activity(void);
void request_sleep(void);
void restore_runtime_state(void);
void __delay_ms(uint32_t ms);
void __delay_us(uint32_t us);
void print_sr(void);
int16_t calculate_roll(void);
void increment_loop_timers(void);
void enterSleepMode(void);
uint8_t find_first_active_table(void);
uint8_t find_next_active_table(void);
void goto_sleep(void);
void prt_stt(void);
void update_battery_status(void);
void check_sleep_cycle(void);
float calibrate_angle(void);
void sw_to_sosc(void);
void sw_to_posc(void);
void configure_button_to_io(void);
void configure_button_to_interrupt(void);
void process_display_animations(void);

#endif
