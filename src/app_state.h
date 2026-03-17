#ifndef APP_STATE_H
#define APP_STATE_H

#include "app_core.h"

typedef enum {
    POWER_STATE_ACTIVE = 0,
    POWER_STATE_PREPARE_SLEEP,
    POWER_STATE_SLEEPING,
    POWER_STATE_RESTORE
} PowerState;

typedef enum {
    BUTTON_STATE_IDLE = 0,
    BUTTON_STATE_ONE_PRESSED,
    BUTTON_STATE_TWO_PRESSED,
    BUTTON_STATE_BOTH_PRESSED,
    BUTTON_STATE_WAIT_RELEASE
} ButtonState;

typedef enum {
    BUTTON_ACTION_NONE = 0,
    BUTTON_ACTION_IGNORE_RELEASE,
    BUTTON_ACTION_SLEEP_ON_RELEASE
} ButtonDeferredAction;

typedef struct {
    bool raw_level;
    bool stable_level;
    uint32_t transition_started_ms;
} DebouncedButton;

extern char PCComm[50];
extern char PCDebug[50];
extern char table_names[5][5];
extern uint16_t angle_raw;
extern RangeAngle table_data[TABLE_SIZE];
extern uint8_t pageBuffer[PAGE_BUFFER_SIZE];
extern uint8_t CommPointer;
extern uint8_t CommAv;
extern uint8_t MsgSize;
extern unsigned long CommStart;
extern unsigned long sleep_timer;
extern char Code_revision[];
extern char Date[];
extern char value[7];
extern char message[];
extern char SSD13003_PAGE[7];
extern char SSD13003_COL[7];
extern unsigned short comm;
extern uint8_t screen_buffer[64][6];
extern uint8_t bat_per_prev;
extern uint8_t table_num;
extern uint8_t roll_state;
extern uint8_t activate_sleep;
extern bool button_one_check;
extern bool button_two_check;
extern bool button_one_long_press;
extern bool button_both_check;
extern bool display_off;
extern int16_t lcd_brightness;
extern uint8_t test_Var;
extern uint8_t active_tables;
extern unsigned long roll_timer;
extern unsigned long timebase;
extern unsigned long printtime;
extern unsigned long sampletime;
extern unsigned long screen_dim;
extern unsigned long button_timer_one;
extern unsigned long button_timer_two;
extern unsigned long button_timer_both;
extern uint16_t raw_angle;
extern uint16_t adc_count;
extern float cur_ang;
extern float input_voltage;
extern float zero_angle;
extern uint8_t blinkflip;
extern uint8_t blink_roll;
extern int16_t roll_angle;
extern int16_t main_cycle;
extern bool show_clear_flag;
extern uint32_t system_time_ms;
extern uint32_t last_tick_count;
extern uint32_t tick_accumulator;
extern uint32_t last_sample_ms;
extern uint32_t last_status_ms;
extern uint32_t last_button_scan_ms;
extern uint32_t last_roll_animation_ms;
extern uint32_t power_state_started_ms;
extern uint32_t button_state_started_ms;
extern uint32_t loaded_table_rows;
extern bool ballistic_table_valid;
extern PowerState power_state;
extern bool power_state_initialized;
extern bool sleep_request_pending;
extern ButtonState button_state;
extern ButtonDeferredAction button_deferred_action;
extern bool button_long_action_fired;
extern DebouncedButton button_one_debounce;
extern DebouncedButton button_two_debounce;
extern uint8_t comm_queue[COMM_QUEUE_DEPTH][COMM_FRAME_MAX_LEN];
extern uint8_t comm_queue_sizes[COMM_QUEUE_DEPTH];
extern uint8_t comm_queue_head;
extern uint8_t comm_queue_tail;
extern uint8_t comm_queue_count;
extern bool comm_queue_overflow;

#endif
