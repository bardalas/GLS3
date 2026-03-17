#ifndef APP_ACTIONS_H
#define APP_ACTIONS_H

#include "app_state.h"

void app_action_refresh_battery_status(void);
void app_action_refresh_measurements(void);
void app_action_restore_runtime_state(void);

void app_action_adjust_brightness(int16_t delta);
void app_action_select_next_table(void);
void app_action_calibrate_zero(void);
void app_action_set_sleep_enabled(bool enabled);
void app_action_update_active_tables(uint8_t active_table_mask);
void app_action_write_table_name(uint8_t table_index, const char *table_name);

#endif
