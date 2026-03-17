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

static void report_system_status(bool verbose)
{
    sprintf(PCDebug, "MEPRO: %u\n**********\n", main_cycle);
    send_string_UART2(PCDebug);

    if (verbose)
        sprintf(PCDebug, "\tENC ANG: %.4f ; raw:%u\n", cur_ang, angle_raw);
    else
        sprintf(PCDebug, "\tENC ANG: %.4f\n", cur_ang);
    send_string_UART2(PCDebug);

    sprintf(PCDebug, "\tLIS WHO AM I: %u\n", LIS_read_register(WHO_AM_I));
    send_string_UART2(PCDebug);
    sprintf(PCDebug, "\tRoll angle: %d\n", roll_angle);
    send_string_UART2(PCDebug);
    sprintf(PCDebug, "\tEEPROM ID:%u\n", 0x00FF & read_M95128_ID());
    send_string_UART2(PCDebug);
    sprintf(PCDebug, "\tAN2 RAW:%u\n", adc_count);
    send_string_UART2(PCDebug);
    sprintf(PCDebug, "\tBAT IN: %.4f\n", input_voltage);
    send_string_UART2(PCDebug);

    if (verbose)
    {
        sprintf(PCDebug, "\tBAT percentage:%u\n", calculate_battery_percentage(input_voltage));
        send_string_UART2(PCDebug);
    }
}

void print_sr(void)
{
#ifdef OPERATIONAL
    return;
#else
    report_system_status(false);
#endif
}

void prt_stt(void)
{
    report_system_status(true);
}

int16_t calculate_roll(void)
{
    int16_t x = LIS_read_x();
    int16_t y = LIS_read_y();

    return (int16_t)(atan2(y, x) * (180.0 / M_PI));
}

float calibrate_angle(void)
{
    uint32_t save_cal_angle;

    zero_angle = read_angle_for_zero(50);
    save_cal_angle = (uint32_t)(zero_angle * ANGLE_STORAGE_SCALE);
    write_byte_eerpom(CAL_VALUE, (uint8_t)(save_cal_angle >> 24));
    write_byte_eerpom((uint16_t)(CAL_VALUE + 1U), (uint8_t)(save_cal_angle >> 16));
    write_byte_eerpom((uint16_t)(CAL_VALUE + 2U), (uint8_t)(save_cal_angle >> 8));
    write_byte_eerpom((uint16_t)(CAL_VALUE + 3U), (uint8_t)save_cal_angle);
    return zero_angle;
}

uint8_t find_first_active_table(void)
{
    int table_index;

    for (table_index = 0; table_index < MAX_NUM_TABLES; table_index++)
    {
        if ((active_tables & (1U << table_index)) != 0U)
            return (uint8_t)(table_index + 1);
    }

    return 0U;
}

uint8_t find_next_active_table(void)
{
    int table_index;

    if (table_num < MAX_NUM_TABLES)
    {
        for (table_index = table_num; table_index < MAX_NUM_TABLES; table_index++)
        {
            if ((active_tables & (1U << table_index)) != 0U)
                return (uint8_t)(table_index + 1);
        }
    }

    for (table_index = 0; table_index < (table_num - 1); table_index++)
    {
        if ((active_tables & (1U << table_index)) != 0U)
            return (uint8_t)(table_index + 1);
    }

    return 0U;
}
