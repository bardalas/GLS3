#include "app_runtime.h"

#include "app_actions.h"
#include "app_ballistics.h"
#include "app_comm.h"
#include "app_display.h"
#include "app_input.h"
#include "app_power.h"
#include "app_system.h"

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

static bool task_due(uint32_t *last_run_ms, uint32_t period_ms, bool catch_up)
{
    if ((system_time_ms - *last_run_ms) < period_ms)
        return false;

    if (catch_up)
        *last_run_ms += period_ms;
    else
        *last_run_ms = system_time_ms;

    return true;
}

static void initialize_runtime_state(void)
{
    last_tick_count = _CP0_GET_COUNT();
    tick_accumulator = 0U;
    system_time_ms = 0U;
    last_sample_ms = 0U;
    last_status_ms = 0U;
    last_button_scan_ms = 0U;
    last_roll_animation_ms = 0U;
    sleep_timer = 0U;
    comm_queue_head = 0U;
    comm_queue_tail = 0U;
    comm_queue_count = 0U;
    comm_queue_overflow = false;
    bat_per_prev = 0U;
    app_power_initialize();
    app_input_reset();
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
    initialize_runtime_state();

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

    configure_button_to_io();
    DCDC_nSHDN = 1;
    activate_sleep = read_byte_eerpom(ACT_SLP);
    sleep_timer = system_time_ms;
}

void app_initialize(void)
{
    all_init();
}

uint32_t app_now_ms(void)
{
    return system_time_ms;
}

void app_register_activity(void)
{
    sleep_timer = system_time_ms;
}

void app_process(void)
{
    uint8_t sample_guard = 0U;

    update_system_time();
    app_power_process();
    if (!app_power_is_active())
        return;

    app_comm_process();

    if (task_due(&last_button_scan_ms, 10U, false))
        app_input_poll();

    while (task_due(&last_sample_ms, 100U, true))
    {
        app_action_refresh_measurements();
        sample_guard++;
        if (sample_guard >= 3U)
        {
            last_sample_ms = system_time_ms;
            break;
        }
    }

    if (task_due(&last_roll_animation_ms, 50U, false))
        process_display_animations();

    if (task_due(&last_status_ms, 500U, false))
        print_sr();
}

void app_run_once(void)
{
    app_process();
}
