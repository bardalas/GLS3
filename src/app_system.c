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

static void update_system_time(void)
{
    uint32_t current_tick = _CP0_GET_COUNT();
    uint32_t elapsed_ticks = current_tick - last_tick_count;

    last_tick_count = current_tick;
    tick_accumulator += elapsed_ticks;
    while (tick_accumulator >= TICK_PER_MS)
    {
        tick_accumulator -= TICK_PER_MS;
        system_time_ms++;
    }
}

static bool task_due(uint32_t *last_run_ms, uint32_t period_ms)
{
    if ((system_time_ms - *last_run_ms) < period_ms)
        return false;

    *last_run_ms = system_time_ms;
    return true;
}

static void sample_sensors_and_refresh_ui(void)
{
    update_battery_status();
    cur_ang = read_angle(50);
    update_range();
    roll_angle = calculate_roll();
    update_angle(roll_angle);
    main_cycle++;
}

static void power_process(void)
{
    switch (power_state)
    {
        case POWER_STATE_ACTIVE:
            if (sleep_request_pending || ((activate_sleep != 0U) && ((system_time_ms - sleep_timer) >= SLEEP_TIME)))
                power_state = POWER_STATE_PREPARE_SLEEP;
            break;
        case POWER_STATE_PREPARE_SLEEP:
            configure_button_to_interrupt();
            LCD_EN = 0;
            DCDC_nSHDN = 0;
            open_l_terminal();
            __delay_ms(100);
            power_state = POWER_STATE_SLEEPING;
            break;
        case POWER_STATE_SLEEPING:
            enterSleepMode();
            power_state = POWER_STATE_RESTORE;
            break;
        case POWER_STATE_RESTORE:
            configure_button_to_io();
            LCD_EN = 1;
            DCDC_nSHDN = 1;
            clr_pot_sd();
            SSD13003_SetBrightness((uint8_t)lcd_brightness);
            sleep_request_pending = false;
            last_tick_count = _CP0_GET_COUNT();
            tick_accumulator = 0U;
            sleep_timer = system_time_ms;
            power_state = POWER_STATE_ACTIVE;
            break;
        default:
            power_state = POWER_STATE_ACTIVE;
            break;
    }
}

void __delay_ms(uint32_t ms)
{
    uint32_t start = _CP0_GET_COUNT();
    uint32_t duration = TICK_PER_MS * ms;

    while ((_CP0_GET_COUNT() - start) < duration) {}
}

void __delay_us(uint32_t us)
{
    uint32_t start = _CP0_GET_COUNT();
    uint32_t duration = TICK_PER_uS * us;

    while ((_CP0_GET_COUNT() - start) < duration) {}
}

void all_init(void)
{
    T2CONbits.ON = 0b0;
    IEC4bits.U2RXIE = 0b1;
    IEC0bits.T2IE = 0;
#ifndef OPERATIONAL
    send_string_UART2("\n\n********* GLS 3 started...\n");
#endif
    clear_screen_buffer();
    SSD13003_Init();
    SSD13003_ClearDisplay();
    set_display_drawings();
    last_tick_count = _CP0_GET_COUNT();
    tick_accumulator = 0U;
    system_time_ms = 0U;
    last_sample_ms = 0U;
    last_status_ms = 0U;
    last_button_scan_ms = 0U;
    last_roll_animation_ms = 0U;
    power_state = POWER_STATE_ACTIVE;
    sleep_request_pending = false;

    lcd_brightness = (int16_t)read_byte_eerpom(BRIGHT_ADD);
    zero_angle = load_cal_angle();

    clr_pot_sd();
    pot_set_resistenace(255);
    SSD13003_SetBrightness((uint8_t)lcd_brightness);
#ifndef OPERATIONAL
    send_string_UART2("\t\t display started\n");
#endif
    init_LIS();
    __builtin_enable_interrupts();
    IPC2bits.T2IP = 5;
    IPC2bits.T2IS = 1;
    while (IFS0bits.T2IF == 1)
        IFS0bits.T2IF = 0;
    IEC0bits.T2IE = 0;
    IFS0bits.T2IF = 0;
    T2CONbits.ON = 1;
    as5048a_spi_init_seq();

    active_tables = read_byte_eerpom(ACTIVE_TAB);
    load_table_names();
    init_ballistic_table();
    write_str_LCD(TABLE_START_W, TABLE_START_H, table_names[table_num - 1U]);
    IEC0bits.INT4IE = 0;
    IFS0bits.INT4IF = 0;
    TRISEbits.TRISE5 = 1;
    DCDC_nSHDN = 1;
    activate_sleep = read_byte_eerpom(ACT_SLP);
    sleep_timer = system_time_ms;
}

uint32_t app_now_ms(void)
{
    return system_time_ms;
}

void app_register_activity(void)
{
    sleep_timer = system_time_ms;
}

void request_sleep(void)
{
    sleep_request_pending = true;
}

void app_process(void)
{
    update_system_time();
    comm_isr();

    if (task_due(&last_button_scan_ms, 10U))
        buttons_isr();

    if (task_due(&last_sample_ms, 100U))
        sample_sensors_and_refresh_ui();

    if (task_due(&last_roll_animation_ms, 50U))
        process_display_animations();

    power_process();

    if (task_due(&last_status_ms, 500U))
        print_sr();
}

void check_sleep_cycle(void)
{
    power_process();
}

void update_battery_status(void)
{
    ADCHS_ChannelConversionStart(ADCHS_CH2);
    while (!ADCHS_ChannelResultIsReady(ADCHS_CH2)) {}

    adc_count = ADCHS_ChannelResultGet(ADCHS_CH2);
    input_voltage = (float)2 * (float)adc_count * (float)ADC_VREF / (float)ADC_MAX_COUNT;
    set_battery_display_vertical(calculate_battery_percentage(input_voltage));
}

void increment_loop_timers(void)
{
    update_system_time();
}

int16_t calculate_roll(void)
{
    int16_t x = LIS_read_x();
    int16_t y = LIS_read_y();

    return (int16_t)(atan2(y, x) * (180.0 / M_PI));
}

void print_sr(void)
{
#ifdef OPERATIONAL
    return;
#else
    report_system_status(false);
#endif
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

void goto_sleep(void)
{
    request_sleep();
    while (sleep_request_pending || (power_state != POWER_STATE_ACTIVE))
        power_process();
}

void configure_button_to_io(void)
{
    IEC0bits.INT4IE = 0;
    IFS0bits.INT4IF = 0;
    TRISEbits.TRISE5 = 1;
}

void configure_button_to_interrupt(void)
{
    TRISEbits.TRISE5 = 1;
    IEC0bits.INT4IE = 1;
    IFS0bits.INT4IF = 0;
}

void sw_to_posc(void)
{
    SYSKEY = 0x00000000;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    OSCCONbits.NOSC = 0b011;
    OSCCONbits.OSWEN = 1;
    while (OSCCONbits.OSWEN == 1) {}
    while (OSCCONbits.CLKLOCK == 0) {}
    SYSKEY = 0x00000000;
}

void sw_to_sosc(void)
{
    SYSKEY = 0x00000000;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    OSCCONbits.NOSC = 0b100;
    OSCCONbits.OSWEN = 1;
    while (OSCCONbits.OSWEN == 1) {}
    SYSKEY = 0x00000000;
}

void prt_stt(void)
{
    report_system_status(true);
}

uint8_t find_first_active_table(void)
{
    int i;

    for (i = 0; i < MAX_NUM_TABLES; i++)
        if ((active_tables & (1U << i)) != 0U)
            return (uint8_t)(i + 1);

    return 0U;
}

uint8_t find_next_active_table(void)
{
    int i;

    if (table_num < MAX_NUM_TABLES)
        for (i = table_num; i < MAX_NUM_TABLES; i++)
            if ((active_tables & (1U << i)) != 0U)
                return (uint8_t)(i + 1);

    for (i = 0; i < (table_num - 1); i++)
        if ((active_tables & (1U << i)) != 0U)
            return (uint8_t)(i + 1);

    return 0U;
}

void enterSleepMode(void)
{
    SYSKEY = 0x0;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    OSCCONbits.SLPEN = 1;
    SYSKEY = 0x0;
    asm volatile("wait");
}
