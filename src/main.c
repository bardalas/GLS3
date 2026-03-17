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
    SYS_Initialize(NULL);
    app_initialize();

#ifndef OPERATIONAL
    send_string_UART2("\t starting main\n<><><><><><><><><><><><><><><>\n");
#endif

    while (true)
    {
        IFS3bits.CNEIF = 0;
        SYS_Tasks();
        app_run_once();
    }

    return EXIT_FAILURE;
}
