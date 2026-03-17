#include "app_power.h"

#include "app_actions.h"

void app_power_initialize(void)
{
    power_state_started_ms = 0U;
    power_state = POWER_STATE_ACTIVE;
    power_state_initialized = false;
    sleep_request_pending = false;
}

bool app_power_is_active(void)
{
    return power_state == POWER_STATE_ACTIVE;
}

void app_power_request_sleep(void)
{
    sleep_request_pending = true;
}

void app_power_process(void)
{
    switch (power_state)
    {
        case POWER_STATE_ACTIVE:
            if (sleep_request_pending || ((activate_sleep != 0U) && ((system_time_ms - sleep_timer) >= SLEEP_TIME)))
            {
                power_state_started_ms = system_time_ms;
                power_state_initialized = false;
                power_state = POWER_STATE_PREPARE_SLEEP;
            }
            break;

        case POWER_STATE_PREPARE_SLEEP:
            if (!power_state_initialized)
            {
                power_state_initialized = true;
                configure_button_to_interrupt();
                LCD_EN = 0;
                DCDC_nSHDN = 0;
                open_l_terminal();
            }

            if ((system_time_ms - power_state_started_ms) >= 100U)
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
            app_action_restore_runtime_state();
            sleep_request_pending = false;
            power_state_initialized = false;
            last_tick_count = _CP0_GET_COUNT();
            tick_accumulator = 0U;
            sleep_timer = system_time_ms;
            power_state = POWER_STATE_ACTIVE;
            break;

        default:
            power_state = POWER_STATE_ACTIVE;
            power_state_initialized = false;
            break;
    }
}

void goto_sleep(void)
{
    app_power_request_sleep();
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

void enterSleepMode(void)
{
    SYSKEY = 0x0;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    OSCCONbits.SLPEN = 1;
    SYSKEY = 0x0;
    asm volatile("wait");
}
