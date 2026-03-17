#include "app_input.h"

static void update_brightness(int16_t delta)
{
    lcd_brightness += delta;
    if (lcd_brightness > BRIGHTNESS_MAX)
        lcd_brightness = BRIGHTNESS_MAX;
    else if (lcd_brightness < 0)
        lcd_brightness = 0;

    SSD13003_SetBrightness((uint8_t)lcd_brightness);
    sprintf(PCDebug, "\nbrightness:%u\n", lcd_brightness);
    send_string_UART2(PCDebug);
    write_byte_eerpom(BRIGHT_ADD, (uint8_t)lcd_brightness);
}

static void wait_for_buttons_release(void)
{
    while (BUTTON_ONE || BUTTON_TWO) {}
}

static bool handle_both_buttons(void)
{
    if (BUTTON_ONE && BUTTON_TWO && !button_both_check)
    {
        button_both_check = true;
        button_timer_both = 0;
        return false;
    }

    if (BUTTON_ONE && BUTTON_TWO && button_both_check && (button_timer_both > OFFTIME))
    {
        if (activate_sleep == 0U)
        {
            wait_for_buttons_release();
            return true;
        }

        DCDC_nSHDN = 0;
        wait_for_buttons_release();
        goto_sleep();
#ifndef OPERATIONAL
        send_string_UART2("turn off display\n");
#endif
        return true;
    }

    if ((!BUTTON_ONE || !BUTTON_TWO) && button_both_check)
        button_both_check = false;

    return false;
}

static void handle_button_one(void)
{
    if (BUTTON_ONE && !button_one_check)
    {
        button_one_check = true;
        sleep_timer = 0;
        button_timer_one = 0;
#ifndef OPERATIONAL
        send_string_UART2("button 1 pushed\n");
#endif
        return;
    }

    if (!button_one_long_press && !BUTTON_ONE && button_one_check && (button_timer_one <= SHORT_BUTTON))
    {
        update_brightness(BRIGHTNESS_STEP);
        button_one_check = false;
        return;
    }

    if (BUTTON_ONE && button_one_check && (button_timer_one > SHORT_BUTTON))
    {
        button_timer_one = 0;
        button_one_long_press = true;
        sleep_timer = 0;
        load_table();
        return;
    }

    if (!BUTTON_ONE && button_one_check)
    {
        button_one_long_press = false;
        button_one_check = false;
    }
}

static void handle_button_two(void)
{
    if (BUTTON_TWO && !button_two_check)
    {
        button_two_check = true;
        sleep_timer = 0;
        button_timer_two = 0;
#ifndef OPERATIONAL
        send_string_UART2("button 2 pushed\n");
#endif
        return;
    }

    if (!BUTTON_TWO && button_two_check && (button_timer_two <= SHORT_BUTTON))
    {
        update_brightness(-BRIGHTNESS_STEP);
        button_two_check = false;
        return;
    }

    if (BUTTON_TWO && button_two_check && (button_timer_two > ZERO_TIME))
    {
        write_str_LCD_large_font(RANGE_START_W, RANGE_START_H, "  O ");
        button_timer_two = 0;
        button_two_check = false;
        calibrate_angle();
        while (BUTTON_TWO) {}
        return;
    }

    if (!BUTTON_TWO && button_two_check)
        button_two_check = false;
}

void buttons_isr(void)
{
    if (handle_both_buttons())
        return;

    handle_button_one();
    handle_button_two();
}
