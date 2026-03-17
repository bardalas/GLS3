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
        app_process();
    }

    return EXIT_FAILURE;
}
