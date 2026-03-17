#include "app.h"
#include "config/default/SSD13003.c"

static uint8_t get_battery_level(uint8_t battery_percentage)
{
    if (battery_percentage <= BATTERY_SEGMENT_EMPTY_MAX)
        return BATTERY_LEVEL_EMPTY;
    if (battery_percentage <= BATTERY_SEGMENT_LOW_MAX)
        return BATTERY_LEVEL_25;
    if (battery_percentage <= BATTERY_SEGMENT_MID_MAX)
        return BATTERY_LEVEL_50;
    if (battery_percentage <= BATTERY_SEGMENT_HIGH_MAX)
        return BATTERY_LEVEL_75;

    return BATTERY_LEVEL_FULL;
}

static void draw_vertical_battery_segments(uint8_t level)
{
    if (bat_per_prev == level)
        return;

    clear_bat();
    bat_per_prev = level;

    if (level >= BATTERY_LEVEL_25)
        fill_rect(BAT_START_W_VER + BAT_WIDTH_VER - 5U, BAT_START_H_VER + 2U, 3U, 3U);
    if (level >= BATTERY_LEVEL_50)
        fill_rect(BAT_START_W_VER + BAT_WIDTH_VER - 10U, BAT_START_H_VER + 2U, 3U, 3U);
    if (level >= BATTERY_LEVEL_75)
        fill_rect(BAT_START_W_VER + BAT_WIDTH_VER - 15U, BAT_START_H_VER + 2U, 3U, 3U);
    if (level >= BATTERY_LEVEL_FULL)
        fill_rect(BAT_START_W_VER + BAT_WIDTH_VER - 20U, BAT_START_H_VER + 2U, 3U, 3U);
}

static uint8_t map_large_font_character(unsigned char c)
{
    switch (c)
    {
        case 'm': return 10U;
        case 'H': return 11U;
        case 'I': return 12U;
        case 'E': return 13U;
        case 'R': return 14U;
        case ' ': return 15U;
        case 'L': return 16U;
        case 'O': return 17U;
        case 'W': return 18U;
        default: return (uint8_t)(c - '0');
    }
}

void CB_ChipSelect(void)
{
    SSD13003_CS = 0;
}

void CB_ChipDeselect(void)
{
    SSD13003_CS = 1;
}

void SSD13003_WriteCommand(uint8_t comm_value)
{
    SSD13003_DC = 0;
    CB_ChipSelect();
    SPI2_Write_custom(comm_value);
    CB_ChipDeselect();
}

void SSD13003_WriteData(uint8_t data_send)
{
    CB_ChipSelect();
    SSD13003_DC = 1;
    SPI2_Write_custom(data_send);
    SSD13003_DC = 0;
    CB_ChipDeselect();
}

void resetOLED(void)
{
    SSD13003_Rst = 0;
    __delay_ms(10);
    SSD13003_Rst = 1;
}

void SSD13003_SPI_ON(void)
{
    CB_ChipDeselect();
    resetOLED();
}

void SSD13003_Init(void)
{
    SSD13003_WriteCommand(0xAE);
    SSD13003_WriteCommand(0xD5);
    SSD13003_WriteCommand(0x80);
    SSD13003_WriteCommand(0xA8);
    SSD13003_WriteCommand(0x2F);
    SSD13003_WriteCommand(0xD3);
    SSD13003_WriteCommand(0x00);
    SSD13003_WriteCommand(0x40);
    SSD13003_WriteCommand(0x8D);
    SSD13003_WriteCommand(0x14);
    SSD13003_WriteCommand(0x20);
    SSD13003_WriteCommand(0x00);
    SSD13003_WriteCommand(0xA0 | 0x1);
    SSD13003_WriteCommand(0xC8);
    SSD13003_WriteCommand(0xDA);
    SSD13003_WriteCommand(0x12);
    SSD13003_WriteCommand(0x81);
    SSD13003_WriteCommand(0x7F);
    SSD13003_WriteCommand(0xD9);
    SSD13003_WriteCommand(0xF1);
    SSD13003_WriteCommand(0xDB);
    SSD13003_WriteCommand(0x40);
    SSD13003_WriteCommand(0xA4);
    SSD13003_WriteCommand(0xA6);
    SSD13003_WriteCommand(0xAF);
}

void OLED13003_Position(uint8_t x, uint8_t page)
{
    if ((x >= SSD13003_WIDTH) || (page >= (SSD13003_HEIGHT / 8U)))
        return;

    SSD13003_WriteCommand(0x00 + (x & 0x0F));
    SSD13003_WriteCommand(0x10 + ((x >> 4) & 0x0F));
    SSD13003_WriteCommand(0x22);
    SSD13003_WriteCommand(page);
}

void SSD13003_SetCursor(uint8_t page, uint8_t column)
{
    SSD13003_WriteCommand(0xB0 | page);
    SSD13003_WriteCommand(0x00 | (column & 0x0F));
    SSD13003_WriteCommand(0x10 | (column >> 4));
}

void setPageAddress(uint8_t add)
{
    SSD13003_WriteCommand(0xB0 | add);
}

void setColumnAddress(uint8_t add)
{
    SSD13003_WriteCommand((uint8_t)((0x10 | (add >> 4)) + 0x02));
    SSD13003_WriteCommand((uint8_t)(0x0F & add));
}

void draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t t;

    for (t = x; t <= (uint8_t)(x + w); t++)
        drawPixel(t, y);
    for (t = x; t <= (uint8_t)(x + w); t++)
        drawPixel(t, (uint8_t)(y + h));
    for (t = y; t <= (uint8_t)(y + h); t++)
        drawPixel(x, t);
    for (t = y; t <= (uint8_t)(y + h); t++)
        drawPixel((uint8_t)(x + w), t);
}

void fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t t;
    uint8_t k;

    for (t = x; t <= (uint8_t)(x + w); t++)
        for (k = y; k <= (uint8_t)(y + h); k++)
            drawPixel(t, k);
}

void draw_vertical_line(void)
{
    uint8_t send_data;

    setPageAddress(1);
    setColumnAddress(24);
    send_data = 0b00000001;
    SSD13003_WriteData(send_data);
    send_data = 0b00000010;
    setColumnAddress(24);
    SSD13003_WriteData(send_data);
    send_data = 0b00000100;
    setColumnAddress(24);
    SSD13003_WriteData(send_data);
    send_data = 0b00001111;
    setColumnAddress(24);
    SSD13003_WriteData(send_data);
}

void drawPixel(uint8_t x, uint8_t y)
{
    uint8_t page;
    uint8_t pixel_in_page;
    uint8_t send_data;
    uint8_t cur_page_data;

    if ((x >= 64U) || (y >= 48U))
        return;

    page = y / 8U;
    pixel_in_page = y % 8U;
    setPageAddress(page);
    setColumnAddress(x);
    cur_page_data = screen_buffer[x][page];
    send_data = (uint8_t)(cur_page_data | (1U << pixel_in_page));
    SSD13003_WriteData(send_data);
    screen_buffer[x][page] = send_data;
}

void load_screen_buffer_to_display(void)
{
    uint8_t i;
    uint8_t j;

    for (i = 0; i <= 63U; i++)
        for (j = 0; j <= 5U; j++)
            if (screen_buffer[i][j] == 1U)
                drawPixel(i, j);
}

void clearPixel(uint8_t x, uint8_t y)
{
    uint8_t page;
    uint8_t pixel_in_page;
    uint8_t send_data;
    uint8_t cur_page_data;

    if ((x >= 64U) || (y >= 48U))
        return;

    page = y / 8U;
    pixel_in_page = y % 8U;
    setPageAddress(page);
    setColumnAddress(x);
    cur_page_data = screen_buffer[x][page];
    send_data = (uint8_t)(cur_page_data & (~(1U << pixel_in_page)));
    SSD13003_WriteData(send_data);
    screen_buffer[x][page] = send_data;
}

void SSD13003_SetBrightness(uint8_t brightness)
{
    sprintf(PCDebug, "brightness:%u\n", brightness);
    send_string_UART2(PCDebug);

    SSD13003_WriteCommand(0x81);
    SSD13003_WriteCommand(brightness);
}

void write_str_LCD(uint8_t x, uint8_t y, char *str)
{
    uint8_t str_ct = 0;

    while (*str != '\0')
    {
        OLED13003_DrawChar((uint8_t)(x + str_ct * 6U), y, *str++);
        str_ct++;
    }
}

void write_str_LCD_large_font(uint8_t x, uint8_t y, char *str)
{
    uint8_t str_ct = 0;

    while (*str != '\0')
    {
        OLED13003_DrawChar_revb((uint8_t)(x + str_ct * (LARGE_CHAR_W + 1U)), y, *str++);
        str_ct++;
    }
}

void write_str_LCD_large_thick_font(uint8_t x, uint8_t y, char *str)
{
    uint8_t str_ct = 0;

    while (*str != '\0')
    {
        OLED13003_DrawChar_revc((uint8_t)(x + str_ct * (LARGE_CHAR_W + 1U)), y, *str++);
        str_ct++;
    }
}

void OLED13003_DrawChar_revc(uint8_t x, uint8_t y, unsigned char c)
{
    uint8_t t;
    uint8_t k;
    uint8_t letter_index = map_large_font_character(c);

    for (t = x; t <= (uint8_t)(x + LARGE_CHAR_W - 1U); t++)
        for (k = y; k <= (uint8_t)(y + LARGE_CHAR_H - 1U); k++)
            if (large_thick_font[letter_index][k - y][t - x])
                drawPixel(t, k);
            else
                clearPixel(t, k);
}

void OLED13003_DrawChar_revb(uint8_t x, uint8_t y, unsigned char c)
{
    uint8_t t;
    uint8_t k;
    uint8_t letter_index = map_large_font_character(c);

    for (t = x; t <= (uint8_t)(x + LARGE_CHAR_W - 1U); t++)
        for (k = y; k <= (uint8_t)(y + LARGE_CHAR_H - 1U); k++)
            if (large_font[letter_index][k - y][t - x])
                drawPixel(t, k);
            else
                clearPixel(t, k);
}

void OLED13003_DrawChar(uint8_t x, uint8_t y, unsigned char c)
{
    uint8_t ii;
    uint8_t char_index;
    uint8_t page = y / 8U;

    setPageAddress(page);
    setColumnAddress(x);

    if ((c >= 'A') && (c <= 'Z'))
        char_index = (uint8_t)(c - 'A');
    else if ((c >= 'a') && (c <= 'z'))
        char_index = (uint8_t)(c - 'a' + 26);
    else if ((c >= '0') && (c <= '9'))
        char_index = (uint8_t)(c - '0' + 52);
    else
    {
        switch (c)
        {
            case ' ': char_index = 62U; break;
            case '#': char_index = 63U; break;
            case '.': char_index = 64U; break;
            default: char_index = 0U; break;
        }
    }

    for (ii = 0; ii < 5U; ii++)
        SSD13003_WriteData(MyFonts[char_index][ii]);
}

void SSD13003_ClearDisplay(void)
{
    uint8_t page;
    uint8_t x;

    clearBuffer();
    for (page = 0; page < (SSD13003_HEIGHT / 8U); page++)
    {
        setPageAddress(page);
        setColumnAddress(0);
        for (x = 0; x < SSD13003_WIDTH; x++)
            SSD13003_WriteData(0x00);
    }
}

void clearBuffer(void)
{
    memset(pageBuffer, 0, PAGE_BUFFER_SIZE);
}

void clear_screen_buffer(void)
{
    memset(screen_buffer, 0, sizeof(screen_buffer));
}

void set_battery_display(uint8_t bat_stat)
{
    clear_bat();
    draw_rect(BAT_START_W, BAT_START_H, BAT_WIDTH, BAT_HEIGHT);
    draw_rect((uint8_t)(BAT_START_W + BAT_WIDTH / 2U - 1U), (uint8_t)(BAT_START_H - 3U), 2U, 3U);

    if (bat_stat > BATTERY_SEGMENT_EMPTY_MAX)
        fill_rect((uint8_t)(BAT_START_W + 2U), (uint8_t)(BAT_START_H + BAT_HEIGHT - 5U), (uint8_t)(BAT_WIDTH - 4U), 3U);
    if (bat_stat > BATTERY_SEGMENT_LOW_MAX)
        fill_rect((uint8_t)(BAT_START_W + 2U), (uint8_t)(BAT_START_H + BAT_HEIGHT - 10U), (uint8_t)(BAT_WIDTH - 4U), 3U);
    if (bat_stat > BATTERY_SEGMENT_MID_MAX)
        fill_rect((uint8_t)(BAT_START_W + 2U), (uint8_t)(BAT_START_H + BAT_HEIGHT - 15U), (uint8_t)(BAT_WIDTH - 4U), 3U);
    if (bat_stat > BATTERY_SEGMENT_HIGH_MAX)
        fill_rect((uint8_t)(BAT_START_W + 2U), (uint8_t)(BAT_START_H + BAT_HEIGHT - 20U), (uint8_t)(BAT_WIDTH - 4U), 3U);
}

void set_battery_display_vertical(uint8_t bat_stat)
{
    draw_rect(BAT_START_W_VER, BAT_START_H_VER, BAT_WIDTH_VER, BAT_HEIGHT_VER);
    draw_rect((uint8_t)(BAT_START_W_VER - 3U), (uint8_t)(BAT_START_H_VER + 2U), 2U, 2U);
    draw_vertical_battery_segments(get_battery_level(bat_stat));
}

void set_display_drawings(void)
{
    draw_rect(ROLL_START_W, ROLL_START_H, ROLL_WIDTH, ROLL_HEIGHT);
    draw_rect((uint8_t)(ROLL_START_W + ROLL_WIDTH / 2U), (uint8_t)(ROLL_START_H - 2U), 1U, 2U);
    draw_roll(-7);
}

void clear_bat(void)
{
    uint8_t i;
    uint8_t j;

    for (i = BAT_START_W_VER + 1U; i <= (uint8_t)(BAT_START_W_VER + BAT_WIDTH_VER - 1U); i++)
        for (j = BAT_START_H_VER + 1U; j <= (uint8_t)(BAT_START_H_VER + BAT_HEIGHT_VER - 1U); j++)
            clearPixel(i, j);
}

void clear_roll(void)
{
    uint8_t i;
    uint8_t j;

    for (i = ROLL_START_W + 1U; i <= (uint8_t)(ROLL_START_W + ROLL_WIDTH - 1U); i++)
        for (j = ROLL_START_H + 1U; j <= (uint8_t)(ROLL_START_H + ROLL_HEIGHT - 1U); j++)
            clearPixel(i, j);
}

void draw_roll(int16_t x)
{
    if ((roll_state != BLINK_RIGHT) && (roll_state != BLINK_LEFT))
    {
        if ((x >= -5) && (x <= 5))
            fill_rect((uint8_t)(ROLL_START_W + ROLL_WIDTH / 2U - 1U), (uint8_t)(ROLL_START_H + 2U), 3U, 2U);
        else if ((x >= 6) && (x <= 10))
            fill_rect((uint8_t)(ROLL_START_W + ROLL_WIDTH / 2U - 5U), (uint8_t)(ROLL_START_H + 2U), 3U, 2U);
        else if ((x >= 11) && (x <= 15))
            fill_rect((uint8_t)(ROLL_START_W + ROLL_WIDTH / 2U - 11U), (uint8_t)(ROLL_START_H + 2U), 3U, 2U);
        else if ((x >= 16) && (x <= 20))
            fill_rect((uint8_t)(ROLL_START_W + ROLL_WIDTH / 2U - 16U), (uint8_t)(ROLL_START_H + 2U), 3U, 2U);
        else if ((x >= -20) && (x <= -16))
            fill_rect((uint8_t)(ROLL_START_W + ROLL_WIDTH / 2U + 15U), (uint8_t)(ROLL_START_H + 2U), 3U, 2U);
        else if ((x >= -15) && (x <= -11))
            fill_rect((uint8_t)(ROLL_START_W + ROLL_WIDTH / 2U + 9U), (uint8_t)(ROLL_START_H + 2U), 3U, 2U);
        else if ((x >= -10) && (x <= -6))
            fill_rect((uint8_t)(ROLL_START_W + ROLL_WIDTH / 2U + 4U), (uint8_t)(ROLL_START_H + 2U), 3U, 2U);
    }

    if (roll_state == BLINK_RIGHT)
        blink_roll = 2;
    else if (roll_state == BLINK_LEFT)
        blink_roll = 1;
    else
    {
        blink_roll = 0;
        roll_timer = 0;
    }

    if (blink_roll == 1U)
    {
        roll_timer++;
        if (roll_timer > 3U)
        {
            if (!blinkflip)
            {
                blinkflip = 1;
                fill_rect((uint8_t)(ROLL_START_W + ROLL_WIDTH / 2U + 15U), (uint8_t)(ROLL_START_H + 2U), 3U, 2U);
            }
            else
            {
                blinkflip = 0;
                clear_roll();
            }
            roll_timer = 0;
        }
    }

    if (blink_roll == 2U)
    {
        roll_timer++;
        if (roll_timer > 3U)
        {
            if (!blinkflip)
            {
                blinkflip = 1;
                fill_rect((uint8_t)(ROLL_START_W + ROLL_WIDTH / 2U - 16U), (uint8_t)(ROLL_START_H + 2U), 3U, 2U);
            }
            else
            {
                blinkflip = 0;
                clear_roll();
            }
            roll_timer = 0;
        }
    }
}

uint8_t determine_roll_state(int16_t x)
{
    if ((x >= -5) && (x <= 5))
        return ROLL_M5_5;
    if ((x >= 6) && (x <= 10))
        return ROLL_6_10;
    if ((x >= 11) && (x <= 15))
        return ROLL_11_15;
    if ((x >= 16) && (x <= 20))
        return ROLL_16_20;
    if ((x >= -20) && (x <= -16))
        return ROLL_M16_M20;
    if ((x >= -15) && (x <= -11))
        return ROLL_M11_M15;
    if ((x >= -10) && (x <= -6))
        return ROLL_M6_M10;
    if (x > 20)
        return BLINK_RIGHT;
    if (x < -20)
        return BLINK_LEFT;

    return ROLL_M5_5;
}

void update_angle(int16_t new_angle)
{
    uint8_t rollnow;

    if (LIS_read_register(WHO_AM_I) != 0x33U)
    {
        clear_roll();
        return;
    }

    rollnow = determine_roll_state(new_angle);
    if (rollnow != roll_state)
    {
#ifndef OPERATIONAL
        sprintf(PCComm, ">>>>>>>>>>>>new roll state %u\n", roll_state);
        send_string_UART2(PCComm);
#endif
        roll_state = rollnow;
        if ((rollnow != BLINK_RIGHT) && (rollnow != BLINK_LEFT))
            clear_roll();
        draw_roll(new_angle);
        if (roll_state >= 7U)
            clear_roll();
    }

    if (roll_state >= 7U)
        draw_roll(new_angle);
}
