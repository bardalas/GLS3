#ifndef APP_H
#define APP_H

#define OPERATIONAL                     // don't sent prints
#define FW_VER                          2
//#define FORRONEN
//#define SLEEP_ACT

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "definitions.h"
#include "defs.h"
#include "drivers.h"
#include "config/default/M95128.h"
#include "LIS3DHTR.h"
#include "AS5048A.h"
#include "config/default/MAX5394.h"

#define TABLE_NAME_TEXT_LEN            5U
#define BATTERY_PERCENT_MIN_VOLTAGE    2.4f
#define BATTERY_PERCENT_RANGE_VOLTAGE  0.6f
#define BATTERY_SEGMENT_EMPTY_MAX      4U
#define BATTERY_SEGMENT_LOW_MAX        24U
#define BATTERY_SEGMENT_MID_MAX        49U
#define BATTERY_SEGMENT_HIGH_MAX       74U
#define BATTERY_LEVEL_EMPTY            1U
#define BATTERY_LEVEL_25               2U
#define BATTERY_LEVEL_50               3U
#define BATTERY_LEVEL_75               4U
#define BATTERY_LEVEL_FULL             5U
#define BRIGHTNESS_STEP                50
#define BRIGHTNESS_MAX                 250
#define ANGLE_STORAGE_SCALE            10000000.0f
#define BALLISTIC_TABLE_ENTRY_SIZE     6U
#define BALLISTIC_TABLE_BYTES          800U
#define BALLISTIC_TABLE_START_PAGE     1U
#define BALLISTIC_TABLE_PAGES_PER_TAB  5U
#define COMM_CRC_ERROR_OPCODE          0x55U
#define EEPROM_TEST_ADDRESS            0x0102U
#define RAW_ANGLE_MAX_COUNT            16383.0f
#define RAW_ANGLE_FULL_SCALE_DEG       360.0f
#define DEBUG_SAMPLE_COUNT             100U
#define DEBUG_SAMPLE_DELAY_MS          10U

typedef struct {
    uint16_t range;
    float angle;
} RangeAngle;

extern char PCComm[50], PCDebug[50];
extern char table_names[5][5];
extern uint16_t angle_raw;
extern RangeAngle table_data[TABLE_SIZE];
extern uint8_t pageBuffer[PAGE_BUFFER_SIZE];
extern uint8_t CommPointer, CommAv, MsgSize;
extern unsigned long CommStart, sleep_timer;
extern char Code_revision[];
extern char Date[];
extern char value[7];
extern char message[];
extern char SSD13003_PAGE[7];
extern char SSD13003_COL[7];
extern unsigned short comm;
extern uint8_t screen_buffer[64][6], bat_per_prev;
extern uint8_t table_num, roll_state, activate_sleep;
extern bool button_one_check, button_two_check, button_one_long_press, button_both_check, display_off;
extern int16_t lcd_brightness;
extern uint8_t test_Var, active_tables;
extern unsigned long roll_timer, timebase, printtime, sampletime, screen_dim, button_timer_one, button_timer_two, button_timer_both;
extern uint16_t raw_angle;
extern uint16_t adc_count;
extern float cur_ang, input_voltage, zero_angle;
extern uint8_t blinkflip;
extern uint8_t blink_roll;
extern int16_t roll_angle, main_cycle;
extern bool show_clear_flag;

void all_init(void);
void __delay_ms(uint32_t ms);
void __delay_us(uint32_t us);
void CB_ChipSelect(void);
void CB_ChipDeselect(void);
void SSD13003_WriteCommand(uint8_t comm);
void SSD13003_WriteData(uint8_t Data_Send);
void resetOLED(void);
void SSD13003_SPI_ON(void);
void SSD13003_Init(void);
void OLED13003_Position(uint8_t x, uint8_t page);
void SSD13003_SetCursor(uint8_t page, uint8_t column);
void setPageAddress(uint8_t add);
void setColumnAddress(uint8_t add);
void clear(void);
void drawPixel(uint8_t x, uint8_t y);
void clearPixel(uint8_t x, uint8_t y);
void SSD13003_SetBrightness(uint8_t brightness);
void OLED13003_DrawChar(uint8_t x, uint8_t y, unsigned char c);
void SSD13003_ClearDisplay(void);
void clearBuffer(void);
void draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void set_display_drawings(void);
void draw_vertical_line(void);
void set_battery_display(uint8_t bat_stat);
void fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void draw_roll(int16_t x);
void write_str_LCD(uint8_t x, uint8_t y, char *str);
void clear_bat(void);
void load_table(void);
void update_angle(int16_t new_angle);
void clear_roll(void);
uint8_t determine_roll_state(int16_t x);
void buttons_isr(void);
char handle_comm(char *Comm);
void comm_isr(void);
char CalcCRC(unsigned char TxMsgSize,char *MsgOut);
char CheckCRC(char OPCODE, unsigned char RxMsgSize,char *CheckComm);
void print_sr(void);
void clear_screen_buffer(void);
int16_t calculate_roll(void);
void set_battery_display_vertical(uint8_t bat_stat);
void write_str_LCD_large_font(uint8_t x, uint8_t y, char *str);
void OLED13003_DrawChar_revb(uint8_t x, uint8_t y, unsigned char c);
void load_screen_buffer_to_display(void);
void write_str_LCD_large_thick_font(uint8_t x, uint8_t y, char *str);
void send_ack_to_PC(uint8_t cur_opcode);
void send_info_back(void);
void init_ballistic_table(void);
uint16_t interpolateRange(float targetAngle);
void update_range(void);
uint16_t roundToNearest5(uint16_t value);
void increment_loop_timers(void);
void enterSleepMode(void);
uint8_t find_first_active_table(void);
uint8_t find_next_active_table(void);
void goto_sleep(void);
void OLED13003_DrawChar_revc(uint8_t x, uint8_t y, unsigned char c);
void prt_stt(void);
void update_battery_status(void);
void check_sleep_cycle(void);
float calibrate_angle(void);
void sw_to_sosc(void);
float load_cal_angle(void);
void sw_to_posc(void);
void configure_button_to_io(void);
void configure_button_to_interrupt(void);

#endif
