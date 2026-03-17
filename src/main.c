/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#define OPERATIONAL                     // don't sent prints 
#define FW_VER                          2
//#define FORRONEN
//#define SLEEP_ACT

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes
#include "defs.h"  
char PCComm[50],PCDebug[50];
char CalcCRC(unsigned char TxMsgSize,char *MsgOut);
char table_names[5][5] = { "xxxx\0", "xxxx\0", "xxxx\0", "xxxx\0","xxxx\0" }; // first one is the tables index second one is the chars index
#include "drivers.h"
#include "config/default/M95128.h"
#include "SSD13003.c"
#include "LIS3DHTR.h"
#include "AS5048A.h"
#include <math.h>
#include "config/default/MAX5394.h"


/* VARS */
typedef struct {
    uint16_t range;
    float angle;
} RangeAngle;
uint16_t angle_raw;
RangeAngle table_data[TABLE_SIZE];
uint8_t pageBuffer[PAGE_BUFFER_SIZE];
uint8_t CommPointer=0,CommAv=0,MsgSize=0xFF;
unsigned long CommStart, sleep_timer=56000; // goto sleep 5 seconds after turn on 
char Code_revision[]="-1-";
char Date[]="07/11/2024";
char value[7];
char message[]="135.0";
char SSD13003_PAGE[7];
char SSD13003_COL[7];
unsigned short comm;
uint8_t screen_buffer[64][6],bat_per_prev=0;
uint8_t table_num=1,roll_state=10,activate_sleep=0; 
bool button_one_check=false,button_two_check=false,button_one_long_press=false,
        button_both_check=false,display_off=false;
int16_t lcd_brightness=127;
uint8_t test_Var=0,active_tables=0;
unsigned long roll_timer,timebase=0,printtime=0,sampletime=0,screen_dim=0, 
        button_timer_one=0, button_timer_two=0,button_timer_both=0;
uint16_t raw_angle;
uint16_t adc_count;
float cur_ang=0,input_voltage, zero_angle=0;
uint8_t blinkflip=0;
uint8_t blink_roll=0; // 0=no blink ; 1=blink left; 2=blink right;
int16_t roll_angle,main_cycle=0;
bool show_clear_flag=false; 
/* DECS */
void all_init(void);
void __delay_ms(uint32_t ms);
void __delay_us(uint32_t us);
void CB_ChipSelect(void);
void CB_ChipDeselect(void);
void SSD13003_WriteCommand(uint8_t comm);
void SSD13003_WriteData(uint8_t Data_Send);
void resetOLED(void);
void SSD13003_SPI_ON(void);
void SSD13003_Init(void);
void OLED13003_Position(uint8_t x, uint8_t page);
void SSD13003_SetCursor(uint8_t page, uint8_t column);
void setPageAddress(uint8_t add);
void setColumnAddress(uint8_t add);
void clear(void);
void drawPixel(uint8_t x, uint8_t y);
void clearPixel(uint8_t x, uint8_t y);
void SSD13003_SetBrightness(uint8_t brightness) ;
void OLED13003_DrawChar(uint8_t x, uint8_t y, unsigned char c);
void SSD13003_ClearDisplay(void);
void clearBuffer(void) ;
void draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void set_display_drawings(void);
void draw_vertical_line(void);
void set_battery_display(uint8_t bat_stat);
void fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void draw_roll(int16_t x);
void write_str_LCD(uint8_t x, uint8_t y, char *str);
void clear_bat(void);
void load_table(void);
void update_angle(int16_t new_angle);
void clear_roll(void);
uint8_t determine_roll_state(int16_t x);
void buttons_isr(void);
char handle_comm(char *Comm);
void comm_isr(void);
char CheckCRC(char OPCODE, unsigned char RxMsgSize,char *CheckComm);
void print_sr(void);
void clear_screen_buffer(void);
int16_t calculate_roll(void);
void set_battery_display_vertical(uint8_t bat_stat);
void write_str_LCD_large_font(uint8_t x, uint8_t y, char *str);
void OLED13003_DrawChar_revb(uint8_t x, uint8_t y, unsigned char c);
void load_screen_buffer_to_display(void);
void write_str_LCD_large_thick_font(uint8_t x, uint8_t y, char *str);
void send_ack_to_PC(uint8_t cur_opcode);
void send_info_back(void);
void init_ballistic_table(void);
uint16_t interpolateRange(float targetAngle);
void update_range(void);
uint16_t roundToNearest5(uint16_t value);
void increment_loop_timers(void);
void enterSleepMode(void);
uint8_t find_first_active_table(void);
uint8_t find_next_active_table(void);
void goto_sleep(void);
void OLED13003_DrawChar_revc(uint8_t x, uint8_t y, unsigned char c);
void prt_stt(void);
void update_battery_status(void);
void check_sleep_cycle(void);
float calibrate_angle(void);
void sw_to_sosc(void);
float load_cal_angle(void);
void sw_to_posc(void);

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

int main ( void )
{
    uint8_t bl_inten;
    uint16_t anghelp;
    float angle_deg_help;
    
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    
    
    all_init();
    
    
#ifndef OPERATIONAL
    send_string_UART2("\t starting main\n<><><><><><><><><><><><><><><>\n");
#endif
    //SSD13003_ClearDisplay();
    //drawPixel(30,16);
    ///while (true)
    //{}
    //if (activate_sleep)
    //    goto_sleep();

    while ( true )
    {
        IFS3bits.CNEIF = 0; // Clear CN interrupt flag
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
       
#ifdef FORRONEN
        //cur_ang=read_angle(50);
        //sprintf(PCDebug,"%.4f\n\r",cur_ang);send_string_UART2(PCDebug);
        roll_angle=calculate_roll(); //int16_t
        anghelp=read_one_angle_dis_error();//read_one_angle();
        angle_deg_help = (((((float)anghelp) * ((float)360.0)) / ((float)16383.0)));
        sprintf(PCDebug,"////\n\r%.2f\n\r////\n\r****\n\r%d\n\r****\n\r",angle_deg_help,roll_angle);send_string_UART2(PCDebug);
        //sprintf(PCDebug,"%.2f\n\r",angle_deg_help);send_string_UART2(PCDebug);
        __delay_ms(40);
#else
        __delay_ms(1); 
        increment_loop_timers();
        comm_isr();
        if (sampletime>100)
        {
            /* Wait till ADC conversion result is available */
            update_battery_status();
            //TEST_PIN=!TEST_PIN; 
            cur_ang=read_angle(50);
            //raw_angle=read_one_angle();
            update_range(); // uses cur_ang
            roll_angle=calculate_roll();
            update_angle(roll_angle);
            buttons_isr();
            sampletime=0;
            main_cycle++;
        }
        check_sleep_cycle();
        if (printtime>=500)
        {
            print_sr();
            printtime=0;
        }
#endif
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}
void check_sleep_cycle(void)
{
if (activate_sleep==0)
    return;

    // 10= 10mS
    // 1000 - 1s
    // 10000 - 10s
    if (sleep_timer>SLEEP_TIME) 
    {
        goto_sleep();
        sleep_timer=0;
    }
}
void update_battery_status(void)
{
    uint8_t bat_per;
    float battery_pct;
    
    ADCHS_ChannelConversionStart(ADCHS_CH2);
    while(!ADCHS_ChannelResultIsReady(ADCHS_CH2)) {};
    adc_count = ADCHS_ChannelResultGet(ADCHS_CH2);  /* Read the ADC result */
    input_voltage = (float)2*(float)adc_count * (float)ADC_VREF / (float)ADC_MAX_COUNT;
    battery_pct = (float)100*(input_voltage-2.4f)/0.6f;
    if (battery_pct < 0.0f)
        battery_pct = 0.0f;
    else if (battery_pct > 100.0f)
        battery_pct = 100.0f;
    bat_per=(uint8_t)battery_pct;
    set_battery_display_vertical(bat_per);
     
}
void increment_loop_timers(void)
{
    sampletime++;
    //screen_dim++;
    printtime++;
    button_timer_one++;  
    button_timer_two++;  
    button_timer_both++;
    sleep_timer++;
}
void update_range(void)
{
    char cur_text[5];
    uint16_t cur_range;
        
    cur_range=interpolateRange(cur_ang);
//#ifndef OPERATIONAL
//    sprintf(PCDebug,"range:%u\n",cur_range);send_string_UART2(PCDebug);
//#endif
    if ((cur_range!=0) && (cur_range!=0xFFFF))
        cur_range=roundToNearest5(cur_range);
    if (cur_range==0)
        sprintf(cur_text,"LOW ",cur_range);
    else if (cur_range==0xFFFF)
        sprintf(cur_text," HI ",cur_range);
    else if (cur_range<100)
    {
        sprintf(cur_text," %um",cur_range);
        
    }
    else
    {
        sprintf(cur_text,"%um",cur_range);
    }
    write_str_LCD_large_font(RANGE_START_W,RANGE_START_H,cur_text);
    
}
uint16_t roundToNearest5(uint16_t value) 
{
    return ((value + 2) / 5) * 5;
}
void all_init(void)
{
    //char cur_text[5];
    
    T2CONbits.ON=0b0;
    IEC4bits.U2RXIE = 0b1;
    IEC0bits.T2IE=0;
#ifndef OPERATIONAL
    send_string_UART2("\n\n********* GLS 3 started...\n");
#endif
    clear_screen_buffer();
    SSD13003_Init();
    SSD13003_ClearDisplay();
    set_display_drawings();
    
    lcd_brightness=(int16_t)read_byte_eerpom(BRIGHT_ADD);
    
    zero_angle=load_cal_angle(); // load sights angle calibration value
    clr_pot_sd();
    pot_set_resistenace(255);
    SSD13003_SetBrightness((uint8_t)lcd_brightness);
#ifndef OPERATIONAL
    send_string_UART2("\t\t display started\n");
#endif
    init_LIS();
    __builtin_enable_interrupts();
    IPC2bits.T2IP = 5;  // Set priority level (1-7, higher is more urgent)
    IPC2bits.T2IS = 1;  // Set sub-priority level
    while (IFS0bits.T2IF == 1){IFS0bits.T2IF=0;}
    IEC0bits.T2IE=0;
    IFS0bits.T2IF=0; // clear IF 
    T2CONbits.ON=1;
    //IFS3bits.CNEIF=0;
    //IEC3bits.CNCIE=1; 
    as5048a_spi_init_seq();
   
    init_ballistic_table();
    active_tables=read_byte_eerpom(ACTIVE_TAB); // load active tables from eeprom
    load_table_names();
     
    //sprintf(cur_text,"%s",table_names[table_num-1]); // get the string ready 
    write_str_LCD(TABLE_START_W,TABLE_START_H,table_names[table_num-1]);
    // configure button 1 to regular input. when going to sleep - switch back to interrupt 4 
    IEC0bits.INT4IE = 0;
    IFS0bits.INT4IF = 0; 
    TRISEbits.TRISE5=1; 
    //INTCONbits.INT4EP=0;
    // turn on screen
    DCDC_nSHDN=1; 
 //   SSD13003_SetBrightness(0xff); // maximum internal brightness
    activate_sleep=read_byte_eerpom(ACT_SLP);
}
int16_t calculate_roll(void)
{
       
    int16_t x,y;
    x=LIS_read_x();
    y=LIS_read_y();
    
   //sprintf(PCDebug,"x:%u\n",x);send_string_UART2(PCDebug);
    // Calculate angle in radians
    double angleRadians = atan2(y, x);
    // Convert to degrees
    double angleDegrees = angleRadians * (180.0 / M_PI);
    return ((int16_t)angleDegrees);
}
void print_sr(void)
{
    //sprintf(PCDebug,"\tLIS WHO AM I: %u\n\r",LIS_read_register(WHO_AM_I));send_string_UART2(PCDebug);
#ifdef OPERATIONAL
    return;
#endif
    sprintf(PCComm,"MEPRO: %u\n**********\n",main_cycle);send_string_UART2(PCComm);
    sprintf(PCComm,"\tENC ANG: %.4f\n",cur_ang);send_string_UART2(PCComm);
    sprintf(PCComm,"\tLIS WHO AM I: %u\n",LIS_read_register(WHO_AM_I));send_string_UART2(PCComm);
    sprintf(PCComm,"\tRoll angle: %d\n",roll_angle);send_string_UART2(PCComm);
    sprintf(PCComm,"\tEEPROM ID:%u\n",0x00FF&read_M95128_ID());send_string_UART2(PCComm);
    sprintf(PCComm,"\tAN2 RAW:%u\n",adc_count);send_string_UART2(PCComm);
    sprintf(PCComm,"\tBAT IN: %.4f\n",input_voltage);send_string_UART2(PCComm);

}
// U2STAbits. UTXBF, OERR, URXDA, U2RXIF
void comm_isr(void)
{
     if (U2STAbits.OERR==1)
        {
            //send_string_UART2("*>reseting UART2 OERR\n");
            U2STAbits.OERR=0b0;
        }
     if (CommAv) // poll PC communication
        {
            //send_string_UART2("*********PROCESSING MSG*******\n");
            sleep_timer=0;
            handle_comm(PCComm);
            
            CommAv=0;
            CommPointer=0;
            CommStart=0;
        }
}

/* Table xfr */
/* 0x17 <length=0x0C> <table><row - uint16> <range - uint16 > <angle - uint32 > < crc > 
 * angle can be 0-4,294,967,296 (degrees * 100000) 
 * range is in 5 meters spacing 
 */
char handle_comm(char *Comm)
{
  
    unsigned char RxMsgSize,OPCODE,help;
    uint8_t sel_table,table_row,MSB,LSB,secbyte,thirdbyte;
    uint16_t table_range,sel_add,anghelp; 
    uint32_t table_angle; 
    float  angle_deg_help;
    int helpint;
   
    OPCODE=*Comm++;
    RxMsgSize=(unsigned char)*Comm++ - 3;  // msgsize does not include the CRC
    
    if (CheckCRC(OPCODE, RxMsgSize, Comm)) // CRC error
    {
        //send_byte_UART2(0x55);
        //send_string_UART2("CRC ERROR");
        send_ack_to_PC(0x55); // send back error
        return 0; 
    }
    // we get to here after msg length so point is ok 
 
    switch (OPCODE)
    {
        case 0x15:   // comm ping
            send_ack_to_PC(OPCODE);
            break;
        case 0x16:   // debug PING
            send_string_UART2("PING\n");
            break;
        case 0x17: // debug send table value to keep in eeprom 
            write_table_row_debug(Comm);
            break;
        case 0x18: // debug test eeprom
            MSB=read_byte_eerpom(0x0102);
            sprintf(PCDebug,"\tRead from address 0x0102:%u\n",0x00FF&MSB);send_string_UART2(PCDebug);
            send_string_UART2("\tWriting previous val + 5...\n");
            write_byte_eerpom(0x0102,MSB+5);
            MSB=read_byte_eerpom(0x0102);
            sprintf(PCDebug,"\tRead from address 0x0102:%u\n",0x00FF&MSB);send_string_UART2(PCDebug);
            break;
        case 0x19:  // read table 
            sel_table=*Comm;
            //sprintf(PCDebug,"\n\tselected table to read -> %u\n",sel_table);send_string_UART2(PCDebug);
            send_table_to_pc(sel_table);
            break;
        case 0x20: // debug - read table to monitor
            sel_table=*Comm;
            send_table_to_pc_monitor(sel_table);
            break;
        case 0x21: // write table data to epprom
            write_table_to_row(Comm);
            send_ack_to_PC(OPCODE);
            break;
        case 0x22: // send back eeprom ID, accelerometer ID
            send_info_back();
            break;
        case 0x23: // table x rows count -> 0x23, 0x06 table count msb count lsb CRC ->  6 bytes 
            write_table_rows_count(Comm);
            send_ack_to_PC(OPCODE);
            break;
        case 0x24: // debug: table x rows count debug
            write_table_rows_count_debug(Comm);
            break;
        case 0x25 : // debug: get all tables rows count
            read_table_rows_count_debug();
            break;
        case 0x26: // get table rows count (send to PC)
            read_table_rows_count(*Comm);
            break;
        case 0x27: // debug - print current table 
            for (sel_add=0;sel_add<=134;sel_add++)
            {
                sprintf(PCDebug,"%u,%.6f\n", table_data[sel_add].range, table_data[sel_add].angle);
                send_string_UART2(PCDebug);
               
            }
            break;
        case 0x28: // debug - calculate range for current angle
            sprintf(PCDebug,"\nangle: %.6f -> range: %u\n",cur_ang,interpolateRange(cur_ang));
            send_string_UART2(PCDebug);
            break;
        case 0x29: // set active tables
            active_tables=*Comm;
            write_byte_eerpom(ACTIVE_TAB,active_tables);// save active tables to eeprom
            // set first active table
            table_num=find_first_active_table(); 
            write_byte_eerpom(LAST_TABLE,table_num);
            send_ack_to_PC(OPCODE);
            break;
        case 0x30: // debug - get active tables
            sprintf(PCDebug,"\n active tables : %u\n",active_tables); send_string_UART2(PCDebug);
            break;
        case 0x31: // get table name -> <0x31> <0x08> <table> <char1><char2><char3><char4> <crc>
            sel_table=*Comm++;
            for (help=0;help<=3;help++)
            {
                table_names[sel_table-1][help]=*Comm; 
                //send_byte_UART2(*Comm);
                write_byte_eerpom((uint16_t)TAB1N+(uint16_t)(4*(sel_table-1))+(uint16_t)help,*Comm++);
                
            }
            send_ack_to_PC(OPCODE);
            break;
        case 0x32: // debug - get table names 
            for (help=0;help<=4;help++) // 5 tables 
            {
               sprintf(PCDebug,"\ttable %u name:%s\n",help+1,table_names[help]);send_string_UART2(PCDebug);
            }
            break;
        case 0x33: // send to sleep, wakeup from button
            goto_sleep();
            break;
        case 0x34: // toggle LCD_EN
            send_string_UART2("\nToggling LCD enable\n");
            LCD_EN=!LCD_EN;
            break;
        case 0x35: // toggle DCDC nSHDN
            DCDC_nSHDN=!DCDC_nSHDN;
            sprintf(PCDebug,"\nnSHDN:%u\n",DCDC_nSHDN);
            send_string_UART2(PCDebug);
            
            break;
        case 0x36: // set potentiometer
            send_string_UART2("\nreseting and setting pot to mid range\n");
            pot_reset();
            pot_set_resistenace(0x7F);
            break;
        case 0x37: // get brightness
            sprintf(PCDebug,"\nbrightness:%u\n",lcd_brightness);send_string_UART2(PCDebug);
            break;
        case 0x38: // get status of all
            prt_stt();
            break;
        case 0x39: // get tables for gui
            for (help=0;help<=4;help++) // 5 tables 
            {
               sprintf(PCDebug,"%s\n",table_names[help]);send_string_UART2(PCDebug);
            }
            break;
        case 0x40: // zero sight
            // add zero value here
            calibrate_angle();
            send_ack_to_PC(OPCODE);
            break;
        case 0x41: // read calibrated angle
            sprintf(PCDebug,"\n zero angle: %.4f\n",zero_angle);send_string_UART2(PCDebug);
            break;
        case 0x42: // debug - send 100 samples
            send_string_UART2("\n");
            for (help=0;help<100;help++)
            {
                cur_ang=read_angle(50);
                sprintf(PCDebug,"%.4f\n",cur_ang);send_string_UART2(PCDebug);
                __delay_ms(10);
            }
            break;
        case 0x43: // activate sleep
            activate_sleep=1;
            write_byte_eerpom(ACT_SLP,1);
            sleep_timer=56000; 
            send_ack_to_PC(OPCODE);
            break;
        case 0x44: // deactivate sleep
            activate_sleep=0;
            write_byte_eerpom(ACT_SLP,0);
            send_ack_to_PC(OPCODE);
            break;
        case 0x45: // debug - spi comm 
            send_string_UART2("kick spi\n");
            SPI2_Initialize();
            as5048a_spi_init_seq();
            break;
        case 0x46: // debug - screen font
            send_string_UART2("sending text to display\n");
            write_str_LCD_large_thick_font(RANGE_START_W,RANGE_START_H,"RRR");
            __delay_ms(2000);
            break;
        case 0x47: // debug bat 
            send_string_UART2("bat debug\n");
            set_battery_display_vertical(45);
            __delay_ms(2000);
            break;
        case 0x48: // debug encoder 
            anghelp=read_one_angle();
            angle_deg_help = ((((float)anghelp) / ((float)16383.0)) * ((float)360.0));
            sprintf(PCDebug,"%u ; %.4f\n",anghelp,angle_deg_help);send_string_UART2(PCDebug);
            anghelp=read_one_angle_look_for_error();
            angle_deg_help = ((((float)anghelp) / ((float)16383.0)) * ((float)360.0));
            sprintf(PCDebug,"%u ; %.4f\n",anghelp,angle_deg_help);send_string_UART2(PCDebug);
            break;
      
     
    }
    return 0;
}
float calibrate_angle(void)
{
    uint32_t save_cal_angle;
    
    // read angle
    zero_angle=read_angle_for_zero(50);
    // store in eeprom - start from MSB
    save_cal_angle=(uint32_t)(zero_angle*(float)10000000); // its 1e8
    write_byte_eerpom(CAL_VALUE,(uint8_t)(save_cal_angle>>24));
    write_byte_eerpom(CAL_VALUE+1,(uint8_t)(save_cal_angle>>16));
    write_byte_eerpom(CAL_VALUE+2,(uint8_t)(save_cal_angle>>8));
    write_byte_eerpom(CAL_VALUE+3,(uint8_t)(save_cal_angle));
    return zero_angle; 
}
// live, full screen power: 48mA
// sleep with just DC DC off 19mA (still regular clock)
// sleep with 32khz clock, no pickit 4 connected: 17mA

void goto_sleep(void)
{
    configure_button_to_interrupt();
    LCD_EN=0;
    DCDC_nSHDN=0;
    open_l_terminal();
    __delay_ms(100);
    //sw_to_sosc();
    enterSleepMode();   // goto sleep     //send_string_UART2("\n>>>going to sleep<<<\n");
   //sw_to_posc();) //send_string_UART2("<<<<<WOKE UP>>>>\n");
   configure_button_to_io();
    LCD_EN=1; 
    DCDC_nSHDN=1;
    clr_pot_sd();
    SSD13003_SetBrightness((uint8_t)lcd_brightness);
//    pot_set_resistenace(lcd_brightness);
}
void configure_button_to_io(void)
{
    IEC0bits.INT4IE = 0;
    IFS0bits.INT4IF = 0; 
    TRISEbits.TRISE5=1; 
}
void configure_button_to_interrupt(void)
{
    TRISEbits.TRISE5=1; 
    IEC0bits.INT4IE = 1;
    IFS0bits.INT4IF = 0; 
}
void sw_to_posc(void)
{
     // switch back to regular clock
    // Unlock sequence
    SYSKEY = 0x00000000;    // Ensure OSCCON is locked
    SYSKEY = 0xAA996655;    // Write Key1 to SYSKEY
    SYSKEY = 0x556699AA;    // Write Key2 to SYSKEY

    // Select Primary Oscillator with PLL (e.g., POSC+PLL)
    OSCCONbits.NOSC = 0b011;  // NOSC = 0b011 for POSC with PLL
    OSCCONbits.OSWEN = 1;     // Initiate clock switch
    
    // Wait for clock switch to complete
    while (OSCCONbits.OSWEN == 1);
    while (OSCCONbits.CLKLOCK == 0);
    // Lock sequence
    SYSKEY = 0x00000000;    // Relock OSCCON
}
void sw_to_sosc(void)
{
      // switch to 32khz
    // Unlock sequence
    SYSKEY = 0x00000000;    // Ensure OSCCON is locked
    SYSKEY = 0xAA996655;    // Write Key1 to SYSKEY
    SYSKEY = 0x556699AA;    // Write Key2 to SYSKEY

    // Select Secondary Oscillator (SOSC)
    OSCCONbits.NOSC = 0b100;  // NOSC = 0b100 for SOSC
    OSCCONbits.OSWEN = 1;     // Initiate clock switch

    // Wait for clock switch to complete
    while (OSCCONbits.OSWEN == 1);

    // Lock sequence
    SYSKEY = 0x00000000;    // Relock OSCCON
}
void prt_stt(void)
{
    sprintf(PCDebug,"MEPRO: %u\n**********\n",main_cycle);send_string_UART2(PCDebug);
    sprintf(PCDebug,"\tENC ANG: %.4f ; raw:%u\n",cur_ang,angle_raw);send_string_UART2(PCDebug);
    sprintf(PCDebug,"\tLIS WHO AM I: %u\n",LIS_read_register(WHO_AM_I));send_string_UART2(PCDebug);
    sprintf(PCDebug,"\tRoll angle: %d\n",roll_angle);send_string_UART2(PCDebug);
    sprintf(PCDebug,"\tEEPROM ID:%u\n",0x00FF&read_M95128_ID());send_string_UART2(PCDebug);
    sprintf(PCDebug,"\tAN2 RAW:%u\n",adc_count);send_string_UART2(PCDebug);
    sprintf(PCDebug,"\tBAT IN: %.4f\n",input_voltage);send_string_UART2(PCDebug);
    sprintf(PCDebug,"\tBAT percentage:%u\n",(uint8_t)((float)100*(input_voltage-2.4)/((float)(0.6))));send_string_UART2(PCDebug);
}
uint8_t find_first_active_table(void)
{
    for (int i = 0; i < 8; i++) {
        if (active_tables & (1 << i)) {
            return (i+1);
        }
    }
    return 0; // Return 0 if no bits are set
}
uint8_t find_next_active_table(void)
{
    // check. let's say table_num is 1
    // so it will start from i=0 which is...1 so no good.
    if (table_num<5) // if we don't exceed max num of tables. 
        for (int i = table_num; i < 8; i++) 
        {
            if (active_tables & (1 << i)) {
                return (i+1);
            }
        }
    for (int i=0;i<table_num-1;i++) // cyclic 
    {
        if (active_tables & (1 << i)) {
            return (i+1);
        }
    }   
    return 0; // Return 0 if no bits are set
}
void enterSleepMode(void) 
{
    // Disable unnecessary peripherals here

    // Configure wake-up sources here

    // Unlock sequence to allow configuration changes
    SYSKEY = 0x0;            // Ensure SYSKEY is locked
    SYSKEY = 0xAA996655;     // Write Key1 to SYSKEY
    SYSKEY = 0x556699AA;     // Write Key2 to SYSKEY

    
    // Set the SLPEN bit to enable Sleep mode
    OSCCONbits.SLPEN = 1;

    // Lock the system
    SYSKEY = 0x0;

    // Enter Sleep mode
    asm volatile("wait");

    // Execution resumes here after wake-up
}
uint16_t interpolateRange(float targetAngle) 
{
    // check if smaller than first angle or larger than last angle
    // if low than show LOW (return 0)
    // if higher than last show HI (return FFFF)
    if (targetAngle<table_data[0].angle)
        return 0;
    // Iterate through the data to find the interval containing targetAngle
    for (uint8_t i = 0; i < 134; i++) {
        float angle1 = table_data[i].angle;
        float angle2 = table_data[i + 1].angle;
        
        // Check if targetAngle is between angle1 and angle2
        if ((targetAngle >= angle1 && targetAngle <= angle2) 
                && (angle2<=300))
                //|| (targetAngle >= angle2 && targetAngle <= angle1)) {
        {
            // Perform linear interpolation
            float range1 = (float)table_data[i].range;
            float range2 = (float)table_data[i + 1].range;
            // Calculate the interpolated range
            float interpolatedRange = range1 + (targetAngle - angle1) * (range2 - range1) / (angle2 - angle1);
            return ((uint16_t)interpolatedRange);
        }
    }

    // Handle case where targetAngle is out of the data range
    // You can choose to extrapolate or return an error indicator
    return 0xFFFF; // or some error indicator
}
void send_info_back(void)
{
    uint8_t msg_buffer[6],CRC;
    
    msg_buffer[0]=0x22;
    msg_buffer[1]=0x05;
    msg_buffer[2]=FW_VER;
    msg_buffer[3]=LIS_read_register(WHO_AM_I);
    msg_buffer[4]=read_M95128_ID();
    CRC=CalcCRC(5,msg_buffer); 
    msg_buffer[5]=CRC;
    for (CRC=0;CRC<=5;CRC++)
        send_byte_UART2(msg_buffer[CRC]);
}
void send_ack_to_PC(uint8_t cur_opcode)
{
    uint8_t msg_buffer[3],CRC;
    
    msg_buffer[0]=cur_opcode;
    msg_buffer[1]=0x03;
    CRC=CalcCRC(2,msg_buffer); 
    msg_buffer[2]=CRC;
    
    for (CRC=0;CRC<=2;CRC++)
        send_byte_UART2(msg_buffer[CRC]);
}
char CalcCRC(unsigned char TxMsgSize,char *MsgOut)
{
    char CSR=0;
    unsigned char helper;

    for (helper=1;helper<=TxMsgSize;helper++)
        CSR^=*MsgOut++;

    return CSR;
}
char CheckCRC(char OPCODE, unsigned char RxMsgSize,char *CheckComm)
{
    char CSR;
    unsigned char helper;

    CSR=OPCODE^(RxMsgSize+3);

    for (helper=1;helper<=RxMsgSize;helper++)
        CSR^=*CheckComm++;

    return(CSR!=*CheckComm); // 1 when CRC error
}
void buttons_isr(void)
{
        //<<<<<<<<<<<<<<<<<<< BOTH BUTTONS >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        if ((BUTTON_ONE)&&(BUTTON_TWO)&&(!button_both_check))
        {
            button_both_check=true;
            button_timer_both=0;
        }
        // long press on two buttons - send to sleep
        else if ((BUTTON_ONE)&&(BUTTON_TWO)&&(button_both_check)&&(button_timer_both>OFFTIME)) // if both pressed already
        {
           
            
            if (activate_sleep==0)
            {
                while ((BUTTON_ONE)||(BUTTON_TWO)) {}// wait for buttons release
                return;
            }
            else
            {
               DCDC_nSHDN=0; // turn off display 
               while ((BUTTON_ONE)||(BUTTON_TWO)) {}
               goto_sleep(); 
            }
            
            //display_off=true;
#ifndef OPERATIONAL
            send_string_UART2("turn off display\n");
#endif 
        }
        else if (((!BUTTON_ONE)||(!BUTTON_TWO))&&(button_both_check))  // one of the button released
        {
            button_both_check=false;
        }
         
        // <<<<<<<<<<<<<<<<<<< BUTTON ONE >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        if ((BUTTON_ONE)&&(!button_one_check))
        {
            button_one_check=true;
            sleep_timer=0;
#ifndef OPERATIONAL
            send_string_UART2("button 1 pushed\n");
#endif      
            
            button_timer_one=0;
            
        }
        // release button - short press
        else if ((!button_one_long_press)&&(!BUTTON_ONE)&&(button_one_check)
                &&(button_timer_one<=SHORT_BUTTON))
        {
            lcd_brightness+=50;         // 5 brightness levels 
            if (lcd_brightness>=250)
                    lcd_brightness=250;
            sprintf(PCDebug,"\nbrightness:%u\n",lcd_brightness);send_string_UART2(PCDebug);
            SSD13003_SetBrightness((uint8_t)lcd_brightness);
            write_byte_eerpom(BRIGHT_ADD,(uint8_t)lcd_brightness);
         //   pot_set_resistenace((uint8_t)lcd_brightness);
            button_one_check=false;
            
        }
        // long press - load table
        else if ((BUTTON_ONE)&&(button_one_check)&&(button_timer_one>SHORT_BUTTON))
        {
            //button_one_check=false;
            button_timer_one=0;
            button_one_long_press=true;
            sleep_timer=0;
            load_table();
        }
        else if ((!BUTTON_ONE)&&(button_one_check))
        {
            button_one_long_press=false; 
            button_one_check=false; 
        }
   
        // <<<<<<<<<<<<<<<<<<< BUTTON TWO >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        if ((BUTTON_TWO)&&(!button_two_check))
        {
            button_two_check=true;
            sleep_timer=0;
            button_timer_two=0;
            
#ifndef OPERATIONAL
         send_string_UART2("button 2 pushed\n");
#endif
     
        }
        // short press
        else if ((!BUTTON_TWO)&&(button_two_check)&&(button_timer_two<=SHORT_BUTTON))
        {
            lcd_brightness-=50;         // 5 brightness levels 
            if (lcd_brightness<0)
                    lcd_brightness=0;
           
            SSD13003_SetBrightness((uint8_t)lcd_brightness);
            sprintf(PCDebug,"\nbrightness:%u\n",lcd_brightness);send_string_UART2(PCDebug);
            write_byte_eerpom(BRIGHT_ADD,(uint8_t)lcd_brightness);
        //    pot_set_resistenace((uint8_t)lcd_brightness);
            button_two_check=false;
        }
        // long press - calibrate 
        else if ((BUTTON_TWO)&&(button_two_check)&&(button_timer_two>ZERO_TIME))
        {
            //send_string_UART2("\nZERO PRESS\n");
            write_str_LCD_large_font(RANGE_START_W,RANGE_START_H,"  O ");
            button_timer_two=0;
            button_two_check=false;
            calibrate_angle();
            while (BUTTON_TWO) {} // wait for button release
        }
        else if ((!BUTTON_TWO)&&(button_two_check))
        {
            //button_two_long_press=false; 
            button_two_check=false; 
        }
            
   
           
        
}
void turn_off_display(void)
{
    
}
//roll ranges
//  -5 to 5  
//  6 to 10    
//  11 to 15   
//  16 to 20
//  >20 blink
//  -6 to -10
//  -11 to -15
//  -16 to -20
//  <-20 blink
// determine_roll_state return 0-8 where 7,8 are blink states
void update_angle(int16_t new_angle)
{
    uint8_t rollnow;
    
    // if error ID
    if (LIS_read_register(WHO_AM_I)!=0x33)
    {
        clear_roll();
        return; 
    }
    rollnow=determine_roll_state((int16_t)new_angle);
    // activate only on change 
    if  (rollnow!=roll_state)
    {
#ifndef OPERATIONAL
        sprintf(PCComm,">>>>>>>>>>>>new roll state %u\n" ,roll_state);send_string_UART2(PCComm);
#endif
        roll_state=rollnow;
        if ((rollnow!=BLINK_RIGHT) && (rollnow!=BLINK_LEFT))
            clear_roll();
        draw_roll((int16_t)new_angle);
        if (roll_state>=7)
            clear_roll();
    }
    if (roll_state>=7)
        draw_roll((int16_t)new_angle);
}
void load_table(void)
{
    char cur_text[5];
    uint16_t sel_add,add_ct,row_ct=0,calc_range;
    uint32_t uint32_angle;
    float calc_angle;
    
    // old cyclic 1-5 tables 
    //table_num++;
    //if (table_num>=6)
    //         table_num=1;
    
    // new- cycle according to active tables
    // active_tables 0x13 (for example) -> tables 1,2,5
    //send_string_UART2("switching to next table...\n");
    table_num=find_next_active_table();
    //sprintf(PCDebug,"new table:%u\n",table_num);send_string_UART2(PCDebug);
    sprintf(cur_text,"%s",table_names[table_num-1]);
    write_str_LCD(TABLE_START_W,TABLE_START_H,cur_text);  
    write_byte_eerpom(LAST_TABLE,table_num);  // save to reload on next startup
    // load table from eeprom 
    // range,angle  -> uint16_t, float  (float should be divided by 10,000,000)
    sel_add=(uint16_t)256*((uint16_t)1+(uint16_t)5*((uint16_t)table_num-(uint16_t)1));
    for (add_ct=sel_add;add_ct<sel_add+800;add_ct+=6) // we load a lot each time
    {
        calc_range=read_byte_eerpom(add_ct)*256+read_byte_eerpom(add_ct+1);
        uint32_angle=(((uint32_t)read_byte_eerpom(add_ct+2))<<24)|
                     (((uint32_t)read_byte_eerpom(add_ct+3))<<16) |
                     (((uint32_t)read_byte_eerpom(add_ct+4))<<8)|
                     (((uint32_t)read_byte_eerpom(add_ct+5)));
        calc_angle=uint32_angle/((float)10000000);
        table_data[row_ct].range = calc_range;
        table_data[row_ct].angle = calc_angle;
        row_ct++;
    }
  
    
   
}

float load_cal_angle(void)
{
    uint32_t uint32_angle;
    uint32_t add_ct;
    float calc_angle;
    
    uint32_angle=(((uint32_t)read_byte_eerpom(CAL_VALUE))<<24)|
                     (((uint32_t)read_byte_eerpom(CAL_VALUE+1))<<16) |
                     (((uint32_t)read_byte_eerpom(CAL_VALUE+2))<<8)|
                     (((uint32_t)read_byte_eerpom(CAL_VALUE+3)));
    if (uint32_angle==0xFFFFFFFF) // if no calibraiton value is saved
        return 0; // no calibration value 
    else 
        calc_angle=uint32_angle/((float)10000000);
    
    return calc_angle;
}
void init_ballistic_table(void)
{
    uint16_t add_ct,sel_add,row_ct=0,calc_range;
    uint32_t uint32_angle;
    float calc_angle;
    //table_num=1; // <<<<<-=---save last table used and load on startup
    table_num=read_byte_eerpom(LAST_TABLE); // load last ballistic table used
    if (table_num>MAX_NUM_TABLES)
        table_num=1; // safe guard and return to default 
    sel_add=(uint16_t)256*((uint16_t)1+(uint16_t)5*((uint16_t)table_num-(uint16_t)1));
    for (add_ct=sel_add;add_ct<sel_add+800;add_ct+=6)
    {
        calc_range=read_byte_eerpom(add_ct)*256+read_byte_eerpom(add_ct+1);
        uint32_angle=(((uint32_t)read_byte_eerpom(add_ct+2))<<24)|
                     (((uint32_t)read_byte_eerpom(add_ct+3))<<16) |
                     (((uint32_t)read_byte_eerpom(add_ct+4))<<8)|
                     (((uint32_t)read_byte_eerpom(add_ct+5)));
        calc_angle=uint32_angle/((float)10000000);
        table_data[row_ct].range = calc_range;
        table_data[row_ct].angle = calc_angle;
        row_ct++; 
    }
}
    
void clear_screen_buffer(void)
{
    uint8_t j,i;
    
    for (i=0;i<=63;i++)
        for (j=0;j<=5;j++)
                screen_buffer[i][j]=0;
    //write_str_LCD(TABLE_START_W,TABLE_START_H,"T1");
}
void set_display_drawings(void)
{

    draw_rect(ROLL_START_W,ROLL_START_H,ROLL_WIDTH,ROLL_HEIGHT);
    draw_rect(ROLL_START_W+ROLL_WIDTH/2,ROLL_START_H-2,1,2);
    //set_battery_display(40);
    //set_battery_display_vertical(90);
    draw_roll(-7);
    //write_str_LCD(TABLE_START_W,TABLE_START_H,"T1");
    //write_str_LCD_large_font(RANGE_START_W,RANGE_START_H,"789m");
    //write_str_LCD(RANGE_START_W,RANGE_START_H,"185m");
}   
/*
 * #define BAT_START_W         3
 * #define BAT_START_H         12
 * #define BAT_WIDTH           8
 * #define BAT_HEIGHT          22
 */
void clear_bat(void)
{
    uint8_t i,j;
    for(i=BAT_START_W_VER+1;i<=BAT_START_W_VER+BAT_WIDTH_VER-1;i++)
        for(j=BAT_START_H_VER+1;j<=BAT_START_H_VER+BAT_HEIGHT_VER-1;j++)
            clearPixel(i,j);
}
void clear_roll(void)
{
    uint8_t i,j;
    for(i=ROLL_START_W+1;i<=ROLL_START_W+ROLL_WIDTH-1;i++)
        for(j=ROLL_START_H+1;j<=ROLL_START_H+ROLL_HEIGHT-1;j++)
            clearPixel(i,j);
}
//roll ranges
//  -5 to 5 -   
//  6 to 10    
//  11 to 15   
//  16 to 20
//  >20 blink
//  -6 to -10
//  -11 to -15
//  -16 to -20
//  <-20 blink
void draw_roll(int16_t x)
{
    //sprintf(PCComm,"\n***%d***\n",x);send_string_UART2(PCComm);
    if ((roll_state!=BLINK_RIGHT)&&(roll_state!=BLINK_LEFT))
    {
        if ((x>=-5) && (x<=5))
            fill_rect(ROLL_START_W+ROLL_WIDTH/2-1,ROLL_START_H+2,3,2);
        else if ((x>=6)&& (x<=10))
            fill_rect(ROLL_START_W+ROLL_WIDTH/2-5,ROLL_START_H+2,3,2);
            
        else if ((x>=11)&& (x<=15))
            fill_rect(ROLL_START_W+ROLL_WIDTH/2-11,ROLL_START_H+2,3,2);
        else if ((x>=16)&& (x<=20))
            fill_rect(ROLL_START_W+ROLL_WIDTH/2-16,ROLL_START_H+2,3,2);
        else if ((x>=-20)&& (x<=-16))
            fill_rect(ROLL_START_W+ROLL_WIDTH/2+15,ROLL_START_H+2,3,2);
        else if ((x>=-15)&& (x<=-11))
            fill_rect(ROLL_START_W+ROLL_WIDTH/2+9,ROLL_START_H+2,3,2);
            
        else if ((x>=-10)&& (x<=-6))
            fill_rect(ROLL_START_W+ROLL_WIDTH/2+4,ROLL_START_H+2,3,2);
    }
    if (roll_state==BLINK_RIGHT)
    {
       // send_string_UART2("blink right\n");
        blink_roll=2; // blink right; 
    }
    else if (roll_state==BLINK_LEFT)
    {
        blink_roll=1; // blink left; 
    }
    else
    {
        blink_roll=0;
        roll_timer=0;
    }
    
    if (blink_roll==1)
    {
        roll_timer++;
        if (roll_timer>3)
        {
            if (!blinkflip)
            {   
                blinkflip=1;
                fill_rect(ROLL_START_W+ROLL_WIDTH/2+15,ROLL_START_H+2,3,2);
            }
            else
            {
                blinkflip=0;
                clear_roll();
            }
            roll_timer=0;
        }
    }
    if (blink_roll==2)
    {
        roll_timer++;
        if (roll_timer>3)
        {
            if (!blinkflip)
            {   
                blinkflip=1;
                fill_rect(ROLL_START_W+ROLL_WIDTH/2-16,ROLL_START_H+2,3,2);
            }
            else
            {
                blinkflip=0;
                clear_roll();
            }
            roll_timer=0;
        }
    }
}
uint8_t determine_roll_state(int16_t x)
{
    if ((x>=-5) && (x<=5))
       return ROLL_M5_5; 
    else if ((x>=6)&& (x<=10))
       return ROLL_6_10; 
    else if ((x>=11)&& (x<=15))
       return ROLL_11_15; 
    else if ((x>=16)&& (x<=20))
       return ROLL_16_20; 
    else if ((x>=-20)&& (x<=-16))
       return ROLL_M16_M20; 
    else if ((x>=-15)&& (x<=-11))
       return ROLL_M11_M15; 
    else if ((x>=-10)&& (x<=-6))
       return ROLL_M6_M10; 
    else if (x>20)
       return BLINK_RIGHT;
    else if (x<-20)
       return BLINK_LEFT;
      
}
// bat stat in 0%-100%
void set_battery_display(uint8_t bat_stat)
{
    clear_bat();
    
    draw_rect(BAT_START_W,BAT_START_H,BAT_WIDTH,BAT_HEIGHT);
    draw_rect(BAT_START_W+BAT_WIDTH/2-1,BAT_START_H-3,2,3);
    // battery levels are:
    //      0-4 one cube
    //      5-24
    //      25-49
    //      50-74
    //      75-100
    if (bat_stat<=4)
    {
        // do nothing - empty 
    }
    else if (bat_stat<=24)
    {
        fill_rect(BAT_START_W+2,BAT_START_H+BAT_HEIGHT-5,BAT_WIDTH-4,3);
    }
    else if (bat_stat<=49) 
    {
        fill_rect(BAT_START_W+2,BAT_START_H+BAT_HEIGHT-5,BAT_WIDTH-4,3);
        fill_rect(BAT_START_W+2,BAT_START_H+BAT_HEIGHT-10,BAT_WIDTH-4,3);
    }
    else if (bat_stat<=74)   
    {
        fill_rect(BAT_START_W+2,BAT_START_H+BAT_HEIGHT-5,BAT_WIDTH-4,3);
        fill_rect(BAT_START_W+2,BAT_START_H+BAT_HEIGHT-10,BAT_WIDTH-4,3);
        fill_rect(BAT_START_W+2,BAT_START_H+BAT_HEIGHT-15,BAT_WIDTH-4,3);
    }
    else
    {
        fill_rect(BAT_START_W+2,BAT_START_H+BAT_HEIGHT-5,BAT_WIDTH-4,3);
        fill_rect(BAT_START_W+2,BAT_START_H+BAT_HEIGHT-10,BAT_WIDTH-4,3);
        fill_rect(BAT_START_W+2,BAT_START_H+BAT_HEIGHT-15,BAT_WIDTH-4,3);
        fill_rect(BAT_START_W+2,BAT_START_H+BAT_HEIGHT-20,BAT_WIDTH-4,3);
    }
        
}
void set_battery_display_vertical(uint8_t bat_stat)
{
    
    
    draw_rect(BAT_START_W_VER,BAT_START_H_VER,BAT_WIDTH_VER,BAT_HEIGHT_VER);
    draw_rect(BAT_START_W_VER-3,BAT_START_H_VER+2,2,2);
    // battery levels are:
    //      0-4 one cube
    //      5-24
    //      25-49
    //      50-74
    //      75-100
    if ((bat_stat<=4)&&(bat_per_prev!=1))
    {
        clear_bat();
        bat_per_prev=1;
        // do nothing - empty 
    }
    else if ((bat_stat<=24)&&(bat_per_prev!=2)&&(bat_stat>4))
    {
        clear_bat();
        bat_per_prev=2;
        fill_rect(BAT_START_W_VER+BAT_WIDTH_VER-5,BAT_START_H_VER+2,3,3);
    }
    else if ((bat_stat<=49)&&(bat_per_prev!=3)&&(bat_stat>24))
    {
        clear_bat();
        bat_per_prev=3;
        fill_rect(BAT_START_W_VER+BAT_WIDTH_VER-5,BAT_START_H_VER+2,3,3);
        fill_rect(BAT_START_W_VER+BAT_WIDTH_VER-10,BAT_START_H_VER+2,3,3);
    }
    else if ((bat_stat<=74)&&(bat_per_prev!=4)&&(bat_stat>49))   
    {
        clear_bat();
        bat_per_prev=4;
        fill_rect(BAT_START_W_VER+BAT_WIDTH_VER-5,BAT_START_H_VER+2,3,3);
        fill_rect(BAT_START_W_VER+BAT_WIDTH_VER-10,BAT_START_H_VER+2,3,3);
        fill_rect(BAT_START_W_VER+BAT_WIDTH_VER-15,BAT_START_H_VER+2,3,3);
     
    }
    else if ((bat_stat>74)&&(bat_per_prev!=5))
    {
        clear_bat();
        bat_per_prev=5;
        fill_rect(BAT_START_W_VER+BAT_WIDTH_VER-5,BAT_START_H_VER+2,3,3);
        fill_rect(BAT_START_W_VER+BAT_WIDTH_VER-10,BAT_START_H_VER+2,3,3);
        fill_rect(BAT_START_W_VER+BAT_WIDTH_VER-15,BAT_START_H_VER+2,3,3);
        fill_rect(BAT_START_W_VER+BAT_WIDTH_VER-20,BAT_START_H_VER+2,3,3);
    }
        
}
void __delay_ms(uint32_t ms)
{
uint32_t Start=_CP0_GET_COUNT();
uint32_t Duration=TICK_PER_MS*ms;
while((_CP0_GET_COUNT()-Start)<Duration);
}
void __delay_us(uint32_t us)
{
uint32_t Start=_CP0_GET_COUNT();
uint32_t Duration=TICK_PER_uS*us;
while((_CP0_GET_COUNT()-Start)<Duration);
}
// Callback function for SSD1306 select.
void CB_ChipSelect(void) {
                          SSD13003_CS = 0;
                         }
// Callback function for SSD1306 deselect.
void CB_ChipDeselect(void) {
                            SSD13003_CS = 1;
                           }
// Function to send a command to the SSD13003
void SSD13003_WriteCommand(uint8_t comm)
{
    SSD13003_DC = 0;         // Set D/C# low for command
    CB_ChipSelect();         // Select the OLED
    SPI2_Write_custom(comm); // Send the command byte over SPI
    CB_ChipDeselect();       // Deselect the OLED
}
// Function to send data to the SSD13003 (for pixels or text)
void SSD13003_WriteData(uint8_t Data_Send){
                                           CB_ChipSelect();       // Select the OLED display
                                           SSD13003_DC = 1;       // Set D/C# high for data
                                           SPI2_Write_custom(Data_Send); // Send data
                                           SSD13003_DC = 0;       // Optionally set D/C# low after sending data
                                           CB_ChipDeselect();     // Deselect the OLED display
                                           }
void resetOLED(void){ // OLED reset function (optional)
                  SSD13003_Rst = 0;              // Reset OLED
                  __delay_ms(10);                // Hold reset for 10ms
                  SSD13003_Rst = 1;              // Release reset
                  }
// Initialize SSD1306_SPI module
void SSD13003_SPI_ON(void){
                      // moved to init io SSD13003_Rst_TRIS = 0;   // Set Rst pin to be output
                      // moved to init io SSD13003_CS_TRIS = 0;    // Set CS pin to be output
                      // moved to init io SSD13003_DC_TRIS = 0;    // Set D/C pin to be output
                      CB_ChipDeselect();           // Deselect module
                      resetOLED(); // Proper reset sequence
                      // Initialize SPI settings
                      }
void SSD13003_Init(void)
{
                    SSD13003_WriteCommand(0xAE); // Display OFF
                    SSD13003_WriteCommand(0xD5); // Set Display Clock Divide Ratio/Oscillator Frequency
                    SSD13003_WriteCommand(0x80); // Set clock divide ratio and oscillator frequency
                    SSD13003_WriteCommand(0xA8); // Set Multiplex Ratio
                    //SSD13003_WriteCommand(0x3F); // For 128x64 OLED
                    SSD13003_WriteCommand(0x2F); // 47 for 64x48 OLED
                    SSD13003_WriteCommand(0xD3); // Set Display Offset
                    SSD13003_WriteCommand(0x00); // No offset
                    SSD13003_WriteCommand(0x40); // Set Display Start Line
                    SSD13003_WriteCommand(0x8D); // Charge Pump Setting
                    SSD13003_WriteCommand(0x14); // Enable charge pump
                    SSD13003_WriteCommand(0x20); // Memory Addressing Mode
                    SSD13003_WriteCommand(0x00); // Horizontal addressing mode
                    SSD13003_WriteCommand(0xA0 | 0x1); // Segment Re-map // right to left or left to right
                    SSD13003_WriteCommand(0xC8); // COM Output Scan Direction  //up down or down up
                    SSD13003_WriteCommand(0xDA); // COM Pins Hardware Configuration
                    SSD13003_WriteCommand(0x12); // Alternative COM pin configuration
                    SSD13003_WriteCommand(0x81); // Contrast Control
                    SSD13003_WriteCommand(0x7F); // Set contrast 50%
                    SSD13003_WriteCommand(0xD9); // Pre-charge Period
                    SSD13003_WriteCommand(0xF1); // Set pre-charge period
                    SSD13003_WriteCommand(0xDB); // VCOMH Deselect Level
                    SSD13003_WriteCommand(0x40); // Set VCOMH deselect level
                    SSD13003_WriteCommand(0xA4); // Entire Display ON
                    SSD13003_WriteCommand(0xA6); // Set Normal Display
                    //SSD13003_WriteCommand(0xFF);  // Maximum contrast
                    SSD13003_WriteCommand(0xAF);    // Display ON
}
void OLED13003_Position(uint8_t x, uint8_t page)
{
    // Ensure the cursor stays within bounds
    if (x >= SSD13003_WIDTH || page >= (SSD13003_HEIGHT/8)) return; //SSD13003_WIDTH=64 SSD13003_HEIGHT=48
    // Set column address (X)
    SSD13003_WriteCommand(0x00 + (x & 0x0F));  // Lower nibble
    SSD13003_WriteCommand(0x10 + ((x >> 4) & 0x0F));  // Upper nibble
    // Set page address (Y)   
    SSD13003_WriteCommand(0x22);
    SSD13003_WriteCommand(page);
}
// Example function to set the cursor position
void SSD13003_SetCursor(uint8_t page, uint8_t column)
{
    SSD13003_WriteCommand(0xB0 | page); // Set page address
    SSD13003_WriteCommand(0x00 | (column & 0x0F)); // Set lower column address
    SSD13003_WriteCommand(0x10 | (column >> 4));   // Set higher column address
}
void setPageAddress(uint8_t add){
                                  add=0xb0|add;
                                  SSD13003_WriteCommand(add);
                                  return;
                                  }
void setColumnAddress(uint8_t add){
                                   SSD13003_WriteCommand((0x10|(add>>4))+0x02);
                                   SSD13003_WriteCommand((0x0f&add));
                                   return;
                                   }
/*void clear(void){
    uint8_t All=1;
    uint16_t kk,jj;
    if (All){
        for (kk=0;kk<8; kk++){
            setPageAddress(kk);
            setColumnAddress(0);
            for (jj=0; jj<0x80; jj++){
                SSD13003_WriteData(0);
            }
        }
    }
    else{
        memset(screenmemory,0,384);                        // (64 x 48) / 8 = 384
        //display();
    }
}*/
void draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t t;
    
    for (t=x;t<=x+w;t++)
        drawPixel(t,y);
    for (t=x;t<=x+w;t++)
        drawPixel(t,y+h);
    for (t=y;t<=y+h;t++)
        drawPixel(x,t);
    for (t=y;t<=y+h;t++)
        drawPixel(x+w,t);
    
}
void fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t t,k;
    
    for (t=x;t<=x+w;t++)
        for (k=y;k<=y+h;k++)
            drawPixel(t,k);
}
void draw_vertical_line(void)
{
    uint8_t Send_Data;
    
    setPageAddress(1);
    setColumnAddress(24);
    Send_Data = 0b00000001;
    SSD13003_WriteData(Send_Data);
    Send_Data = 0b00000010;
    setColumnAddress(24);
    SSD13003_WriteData(Send_Data);
    Send_Data = 0b00000100;
    setColumnAddress(24);
    SSD13003_WriteData(Send_Data);
    Send_Data = 0b00001111;
    setColumnAddress(24);
    SSD13003_WriteData(Send_Data);
}
/* draw single pixel and save results to memory */
void drawPixel(uint8_t x, uint8_t y)
{
    uint8_t page,pixelInPage,Send_Data;
    uint8_t cur_page_data; 
    
    if (x >= 64 || y >= 48) return; // Out of bounds check
    page = y / 8  ; // Each page represents 8 pixels vertically
    pixelInPage = y % 8; // Position within the page (0-7)
    // Set the page address
    setPageAddress(page); // set page works 
    // Set the column address
    setColumnAddress(x);
    // Prepare data for the pixel: set the corresponding bit
    cur_page_data=screen_buffer[x][page];
    Send_Data =cur_page_data|( 1 << pixelInPage); // Set the bit for the pixel in the current page
    // Write the pixel data
    SSD13003_WriteData(Send_Data); // Send the data to light the pixel
    screen_buffer[x][page]=Send_Data;// update screen buffer
    //sprintf(PCComm,"y: %u;page:%u locinpage:%u;datasend:%u\n",y,page,pixelInPage,Send_Data);
    //send_string_UART2(PCComm);
    //__delay_ms(5);
}
void load_screen_buffer_to_display(void)
{
    uint8_t i,j;
    
     for (i=0;i<=63;i++)
        for (j=0;j<=5;j++)
        {
            if (screen_buffer[i][j]==1)
               drawPixel(i,j);
        }
   
     
}
void clearPixel(uint8_t x, uint8_t y)
{
    uint8_t page,pixelInPage,Send_Data;
    uint8_t cur_page_data; 
    
    if (x >= 64 || y >= 48) return; // Out of bounds check
    page = y / 8  ; // Each page represents 8 pixels vertically
    pixelInPage = y % 8; // Position within the page (0-7)
    // Set the page address
    setPageAddress(page); // set page works 
    // Set the column address
    setColumnAddress(x);
    // Prepare data for the pixel: set the corresponding bit
    cur_page_data=screen_buffer[x][page];
    Send_Data =cur_page_data&(~( 1 << pixelInPage)); // Set the bit for the pixel in the current page
    // Write the pixel data
    SSD13003_WriteData(Send_Data); // Send the data to light the pixel
    screen_buffer[x][page]=Send_Data;// update screen buffer
    //sprintf(PCComm,"y: %u;page:%u locinpage:%u;datasend:%u\n",y,page,pixelInPage,Send_Data);
    //send_string_UART2(PCComm);
    //__delay_ms(5);
}
void SSD13003_SetBrightness(uint8_t brightness) 
{
    sprintf(PCDebug,"brightness:%u\n",brightness);send_string_UART2(PCDebug);
    if (brightness > 0xFF) {
        brightness = 0xFF;  // Ensure brightness is within valid range
    }
    SSD13003_WriteCommand(0x81); // Command to set contrast
    SSD13003_WriteCommand(brightness); // Brightness value (0x00 to 0xFF)
}
void write_str_LCD(uint8_t x, uint8_t y, char *str)
{
    uint8_t str_ct=0;
    
     while ((*str != '\0'))
     {
         OLED13003_DrawChar(x+str_ct*6,y,*str++);
         str_ct++;
     }
}
void write_str_LCD_large_font(uint8_t x, uint8_t y, char *str)
{
    uint8_t str_ct=0;
    
     while ((*str != '\0'))
     {
         OLED13003_DrawChar_revb(x+str_ct*(LARGE_CHAR_W+1),y,*str++);
         str_ct++;
     }
}
void write_str_LCD_large_thick_font(uint8_t x, uint8_t y, char *str)
{
    uint8_t str_ct=0;
    
     while ((*str != '\0'))
     {
         OLED13003_DrawChar_revc(x+str_ct*(LARGE_CHAR_W+1),y,*str++);
         str_ct++;
     }
}
void OLED13003_DrawChar_revc(uint8_t x, uint8_t y, unsigned char c)
{
    uint8_t t,k,letter_index;
    
    if (c==109) // if 'm'
        letter_index=10;
    else if (c==72)  // if 'H'
        letter_index=11;
    else if (c==73)  // if 'I'
        letter_index=12;
    else if (c==69)  // if 'E' 
        letter_index=13;
    else if (c==82)  // if 'R' 
        letter_index=14;
    else if (c==32)  // if space
        letter_index=15;
    else if (c==76)   // 'L' 
        letter_index=16;
    else if (c==79)   // 'O'
        letter_index=17;
    else if (c==87)   // 'W'
        letter_index=18;
    else
        letter_index=c-48;

    //sprintf(PCComm,"letter index:%u\n",letter_index);send_string_UART2(PCComm);
    for (t=x;t<=(x+LARGE_CHAR_W-1);t++)   // from 0 to 9 
        for (k=y;k<=(y+LARGE_CHAR_H-1);k++) // from 0 to 13
            if (large_thick_font[letter_index][k-y][t-x])
                drawPixel(t,k);
            else
                clearPixel(t,k);
}
// char size ratio 10x14
void OLED13003_DrawChar_revb(uint8_t x, uint8_t y, unsigned char c)
{
    uint8_t t,k,letter_index;
    
    if (c==109) // if 'm'
        letter_index=10;
    else if (c==72)  // if 'H'
        letter_index=11;
    else if (c==73)  // if 'I'
        letter_index=12;
    else if (c==69)  // if 'E' 
        letter_index=13;
    else if (c==82)  // if 'R' 
        letter_index=14;
    else if (c==32)  // if space
        letter_index=15;
    else if (c==76)   // 'L' 
        letter_index=16;
    else if (c==79)   // 'O'
        letter_index=17;
    else if (c==87)   // 'W'
        letter_index=18;
    else
        letter_index=c-48;

    //sprintf(PCComm,"letter index:%u\n",letter_index);send_string_UART2(PCComm);
    for (t=x;t<=(x+LARGE_CHAR_W-1);t++)   // from 0 to 9 
        for (k=y;k<=(y+LARGE_CHAR_H-1);k++) // from 0 to 13
            if (large_font[letter_index][k-y][t-x])
                drawPixel(t,k);
            else
                clearPixel(t,k);
}
void OLED13003_DrawChar(uint8_t x, uint8_t y, unsigned char c)
{
    uint8_t ii, charIndex;
    uint8_t page = y / 8;  // Determine the page based on y coordinate
    // Set the page address and column address using your existing functions
    setPageAddress(page);       // Set the correct page address
    setColumnAddress(x);        // Set the correct column address
    // Calculate the index in MyFonts array based on character
    if (c >= 'A' && c <= 'Z'){
        charIndex = c - 'A';  // Capital letters start at index 0
    } 
    else if (c >= 'a' && c <= 'z'){
        charIndex = c - 'a' + 26;  // Lowercase letters start at index 26
    } 
    else if (c >= '0' && c <= '9'){
        charIndex = c - '0' + 52;  // Digits start at index 52
    } 
    else {// Handle symbols (space, punctuation, etc.)
        switch (c) {
            case ' ': charIndex = 62; break;  // Space character
            case '#': charIndex = 63; break;  // Example for '#'
            case '.': charIndex = 64; break;  // Example for '.'
            default: charIndex = 0; break;    // Default to space or undefined character
        }
    }
    // Draw the character from the MyFonts array
    for (ii = 0; ii < 5; ii++){
        SSD13003_WriteData(MyFonts[charIndex][ii]);  // Send font data to the display
    }
}
void SSD13003_ClearDisplay(void)
{
    uint8_t page = 0,x = 0;
    // Clear the buffer first
    clearBuffer();
    // Send the cleared buffer to the OLED display
    for (page = 0; page < (SSD13003_HEIGHT / 8); page++) {
        setPageAddress(page);
        setColumnAddress(0); // Start from column 0
        // Send a row of zeroes to the display to clear it
        for (x = 0; x < SSD13003_WIDTH; x++) {
            SSD13003_WriteData(0x00); // Write zeros to clear the display
        }
    }
}
// Change the total fonts included
void clearBuffer(void) 
{
    memset(pageBuffer, 0, PAGE_BUFFER_SIZE); // Set all bytes to 0 (clear buffer)
}
/*******************************************************************************
 End of File
*/

