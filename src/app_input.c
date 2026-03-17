#include "app_input.h"

static bool debounce_button(DebouncedButton *button, bool raw_level)
{
    uint32_t now_ms = app_now_ms();

    if (raw_level != button->raw_level)
    {
        button->raw_level = raw_level;
        button->transition_started_ms = now_ms;
    }
    else if ((button->stable_level != raw_level) && ((now_ms - button->transition_started_ms) >= BUTTON_DEBOUNCE_MS))
    {
        button->stable_level = raw_level;
    }

    return button->stable_level;
}

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

static void set_button_state(ButtonState new_state)
{
    button_state = new_state;
    button_state_started_ms = app_now_ms();
    button_long_action_fired = false;
}

static void execute_deferred_button_action(void)
{
    if (button_deferred_action == BUTTON_ACTION_SLEEP_ON_RELEASE)
        request_sleep();

    button_deferred_action = BUTTON_ACTION_NONE;
}

static void process_idle_state(bool button_one_pressed, bool button_two_pressed)
{
    if (button_one_pressed && button_two_pressed)
    {
        app_register_activity();
        set_button_state(BUTTON_STATE_BOTH_PRESSED);
    }
    else if (button_one_pressed)
    {
        app_register_activity();
        set_button_state(BUTTON_STATE_ONE_PRESSED);
    }
    else if (button_two_pressed)
    {
        app_register_activity();
        set_button_state(BUTTON_STATE_TWO_PRESSED);
    }
}

static void process_button_one_state(bool button_one_pressed, bool button_two_pressed)
{
    uint32_t held_ms = app_now_ms() - button_state_started_ms;

    if (button_one_pressed && button_two_pressed)
    {
        set_button_state(BUTTON_STATE_BOTH_PRESSED);
        return;
    }

    if (!button_one_pressed)
    {
        if (!button_long_action_fired && (held_ms <= SHORT_BUTTON))
            update_brightness(BRIGHTNESS_STEP);
        set_button_state(BUTTON_STATE_IDLE);
        return;
    }

    if (!button_long_action_fired && (held_ms >= SHORT_BUTTON))
    {
        button_long_action_fired = true;
        app_register_activity();
        load_table();
    }
}

static void process_button_two_state(bool button_two_pressed)
{
    uint32_t held_ms = app_now_ms() - button_state_started_ms;

    if (!button_two_pressed)
    {
        if (!button_long_action_fired && (held_ms <= SHORT_BUTTON))
            update_brightness(-BRIGHTNESS_STEP);
        set_button_state(BUTTON_STATE_IDLE);
        return;
    }

    if (!button_long_action_fired && (held_ms >= ZERO_TIME))
    {
        button_long_action_fired = true;
        app_register_activity();
        write_str_LCD_large_font(RANGE_START_W, RANGE_START_H, "  O ");
        calibrate_angle();
        button_deferred_action = BUTTON_ACTION_IGNORE_RELEASE;
        button_state = BUTTON_STATE_WAIT_RELEASE;
    }
}

static void process_both_buttons_state(bool button_one_pressed, bool button_two_pressed)
{
    uint32_t held_ms = app_now_ms() - button_state_started_ms;

    if (!(button_one_pressed && button_two_pressed))
    {
        set_button_state(BUTTON_STATE_IDLE);
        return;
    }

    if (held_ms >= OFFTIME)
    {
        button_deferred_action = (activate_sleep != 0U) ? BUTTON_ACTION_SLEEP_ON_RELEASE : BUTTON_ACTION_IGNORE_RELEASE;
        button_state = BUTTON_STATE_WAIT_RELEASE;
    }
}

static void process_wait_release_state(bool button_one_pressed, bool button_two_pressed)
{
    if (!button_one_pressed && !button_two_pressed)
    {
        execute_deferred_button_action();
        set_button_state(BUTTON_STATE_IDLE);
    }
}

void buttons_isr(void)
{
    bool button_one_pressed = debounce_button(&button_one_debounce, (BUTTON_ONE != 0));
    bool button_two_pressed = debounce_button(&button_two_debounce, (BUTTON_TWO != 0));

    switch (button_state)
    {
        case BUTTON_STATE_IDLE:
            process_idle_state(button_one_pressed, button_two_pressed);
            break;
        case BUTTON_STATE_ONE_PRESSED:
            process_button_one_state(button_one_pressed, button_two_pressed);
            break;
        case BUTTON_STATE_TWO_PRESSED:
            process_button_two_state(button_two_pressed);
            break;
        case BUTTON_STATE_BOTH_PRESSED:
            process_both_buttons_state(button_one_pressed, button_two_pressed);
            break;
        case BUTTON_STATE_WAIT_RELEASE:
            process_wait_release_state(button_one_pressed, button_two_pressed);
            break;
        default:
            set_button_state(BUTTON_STATE_IDLE);
            break;
    }
}
