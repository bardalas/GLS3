/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    Application entry point and main control loop.
 *******************************************************************************/

#include "app.h"

static void sample_sensors_and_refresh_ui(void)
{
    update_battery_status();
    cur_ang = read_angle(50);
    update_range();
    roll_angle = calculate_roll();
    update_angle(roll_angle);
    buttons_isr();
    sampletime = 0;
    main_cycle++;
}

int main(void)
{
    /* Initialize all modules */
    SYS_Initialize(NULL);
    all_init();

#ifndef OPERATIONAL
    send_string_UART2("\t starting main\n<><><><><><><><><><><><><><><>\n");
#endif

    while (true)
    {
        IFS3bits.CNEIF = 0;
        SYS_Tasks();

#ifdef FORRONEN
        __delay_ms(40);
#else
        __delay_ms(1);
        increment_loop_timers();
        comm_isr();

        if (sampletime > 100U)
            sample_sensors_and_refresh_ui();

        check_sleep_cycle();

        if (printtime >= 500U)
        {
            print_sr();
            printtime = 0;
        }
#endif
    }

    return EXIT_FAILURE;
}
