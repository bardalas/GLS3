/*******************************************************************************
 System Interrupts File

  Company:
    Microchip Technology Inc.

  File Name:
    interrupt.c

  Summary:
    Interrupt vectors mapping

  Description:
    This file maps all the interrupt vectors to their corresponding
    implementations. If a particular module interrupt is used, then its ISR
    definition can be found in corresponding PLIB source file. If a module
    interrupt is not used, then its ISR implementation is mapped to dummy
    handler.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *******************************************************************************/
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include "interrupts.h"
#include "definitions.h"

extern uint8_t CommAv,CommPointer,MsgSize;
extern char PCComm[50];
extern unsigned long CommStart;
extern unsigned long timebase,printtime,sampletime;


// *****************************************************************************
// *****************************************************************************
// Section: System Interrupt Vector Functions
// *****************************************************************************
// *****************************************************************************


/* All the handlers are defined here.  Each will call its PLIB-specific function. */
// *****************************************************************************
// *****************************************************************************
// Section: System Interrupt Vector declarations
// *****************************************************************************
// *****************************************************************************
void TIMER_2_Handler (void);
void EXTERNAL_4_Handler (void);
void UART2_RX_Handler (void);


// *****************************************************************************
// *****************************************************************************
// Section: System Interrupt Vector definitions
// *****************************************************************************
// *****************************************************************************
void __ISR(_TIMER_2_VECTOR, ipl1SRS) TIMER_2_Handler (void)
{
    timebase++;
    printtime++;
    sampletime++;
    //TIMER_2_InterruptHandler();
    IFS0bits.T2IF=0; // clear IF 
}
void __ISR(_EXTERNAL_4_VECTOR, ipl1SRS) EXTERNAL_4_Handler (void)
{
    //send_byte_UART2('A');
    IFS0bits.INT4IF = 0;
    //IFS0CLR = _IFS0_INT4IF_MASK;
    //EXTERNAL_4_InterruptHandler();
}
void __ISR(_UART2_RX_VECTOR, ipl1SRS) UART2_RX_Handler (void)
{
   uint8_t Comm_data = 0;
   char debug_buffer[10];
   Comm_data=U2RXREG; // read buffer
   if (!CommAv)
            {
                if (CommPointer==0)  // if first char is accepted then start timer
                    CommStart=timebase;


                if (CommPointer<=MsgSize-1)
                {
                    if (CommPointer==1)
                        MsgSize=(unsigned char)Comm_data;         // Update MsgSize according to received byte #2
                                                                  // Msgsize includes all bytes - including opcode, msgsize and CRC
                    PCComm[CommPointer]=Comm_data;                // push new char into buffer
                    //sprintf(debug_buffer,"0x%02x\n",Comm_data);send_string_UART2(debug_buffer);
                    CommPointer++;
                }

                if (CommPointer==MsgSize)    // if received last char raise a flag
                {
                    CommAv=1;                // should be cleared after executing the command
                    MsgSize=0xFF;            // init MsgSize for next Rx message
                }
            }
   IFS4bits.U2RXIF=0;
}




/*******************************************************************************
 End of File
*/
