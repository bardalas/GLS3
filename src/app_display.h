#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H

#include "app_state.h"

void CB_ChipSelect(void);
void CB_ChipDeselect(void);
void SSD13003_WriteCommand(uint8_t comm);
void SSD13003_WriteData(uint8_t data_send);
void resetOLED(void);
void SSD13003_SPI_ON(void);
void SSD13003_Init(void);
void OLED13003_Position(uint8_t x, uint8_t page);
void SSD13003_SetCursor(uint8_t page, uint8_t column);
void setPageAddress(uint8_t add);
void setColumnAddress(uint8_t add);
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
void update_angle(int16_t new_angle);
void clear_roll(void);
uint8_t determine_roll_state(int16_t x);
void clear_screen_buffer(void);
void set_battery_display_vertical(uint8_t bat_stat);
void write_str_LCD_large_font(uint8_t x, uint8_t y, char *str);
void OLED13003_DrawChar_revb(uint8_t x, uint8_t y, unsigned char c);
void load_screen_buffer_to_display(void);
void write_str_LCD_large_thick_font(uint8_t x, uint8_t y, char *str);
void OLED13003_DrawChar_revc(uint8_t x, uint8_t y, unsigned char c);

#endif
