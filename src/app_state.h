#ifndef APP_STATE_H
#define APP_STATE_H

#include "app_core.h"

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

#endif
