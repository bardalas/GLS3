#include "app_actions.h"

#include "app_ballistics.h"
#include "app_display.h"
#include "app_input.h"
#include "app_runtime.h"
#include "app_system.h"

static uint8_t calculate_battery_percentage(float voltage)
{
    float battery_pct = (100.0f * (voltage - BATTERY_PERCENT_MIN_VOLTAGE)) / BATTERY_PERCENT_RANGE_VOLTAGE;

    if (battery_pct < 0.0f)
        battery_pct = 0.0f;
    else if (battery_pct > 100.0f)
        battery_pct = 100.0f;

    return (uint8_t)battery_pct;
}

static void write_active_table_label(void)
{
    write_str_LCD(TABLE_START_W, TABLE_START_H, table_names[table_num - 1U]);
}

void app_action_refresh_battery_status(void)
{
    ADCHS_ChannelConversionStart(ADCHS_CH2);
    while (!ADCHS_ChannelResultIsReady(ADCHS_CH2)) {}

    adc_count = ADCHS_ChannelResultGet(ADCHS_CH2);
    input_voltage = (float)2 * (float)adc_count * (float)ADC_VREF / (float)ADC_MAX_COUNT;
    set_battery_display_vertical(calculate_battery_percentage(input_voltage));
}

void app_action_refresh_measurements(void)
{
    app_action_refresh_battery_status();
    cur_ang = read_angle(50);
    update_range();
    roll_angle = calculate_roll();
    update_angle(roll_angle);
    main_cycle++;
}

void app_action_restore_runtime_state(void)
{
    app_input_reset();
    SSD13003_Init();
    SSD13003_ClearDisplay();
    set_display_drawings();
    SSD13003_SetBrightness((uint8_t)lcd_brightness);
    write_active_table_label();
    app_action_refresh_measurements();
}

void app_action_adjust_brightness(int16_t delta)
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

void app_action_select_next_table(void)
{
    load_table();
    update_range();
}

void app_action_calibrate_zero(void)
{
    write_str_LCD_large_font(RANGE_START_W, RANGE_START_H, "  O ");
    calibrate_angle();
}

void app_action_set_sleep_enabled(bool enabled)
{
    activate_sleep = enabled ? 1U : 0U;
    write_byte_eerpom(ACT_SLP, activate_sleep);
    app_register_activity();
}

void app_action_update_active_tables(uint8_t active_table_mask)
{
    bool current_table_still_active;

    active_tables = active_table_mask;
    write_byte_eerpom(ACTIVE_TAB, active_tables);

    current_table_still_active = (table_num >= 1U) &&
        (table_num <= MAX_NUM_TABLES) &&
        ((active_tables & (1U << (table_num - 1U))) != 0U);

    if (!current_table_still_active)
    {
        table_num = find_first_active_table();
        if ((table_num == 0U) || (table_num > MAX_NUM_TABLES))
            table_num = 1U;
    }

    write_byte_eerpom(LAST_TABLE, table_num);
    init_ballistic_table();
    write_active_table_label();
    update_range();
}

void app_action_write_table_name(uint8_t table_index, const char *table_name)
{
    uint8_t char_index;

    if ((table_index == 0U) || (table_index > MAX_NUM_TABLES) || (table_name == NULL))
        return;

    for (char_index = 0U; char_index < (TABLE_NAME_TEXT_LEN - 1U); char_index++)
    {
        char next_char = table_name[char_index];

        table_names[table_index - 1U][char_index] = next_char;
        write_byte_eerpom((uint16_t)TAB1N + (uint16_t)(4U * (table_index - 1U)) + char_index, (uint8_t)next_char);
    }

    table_names[table_index - 1U][TABLE_NAME_TEXT_LEN - 1U] = '\0';
    if (table_index == table_num)
        write_active_table_label();
}
