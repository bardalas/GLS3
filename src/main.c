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
#include <stdio.h>
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

#define TABLE_NAME_TEXT_LEN            5U
#define BATTERY_PERCENT_MIN_VOLTAGE    2.4f
#define BATTERY_PERCENT_RANGE_VOLTAGE  0.6f
#define BATTERY_SEGMENT_EMPTY_MAX      4U
#define BATTERY_SEGMENT_LOW_MAX        24U
#define BATTERY_SEGMENT_MID_MAX        49U
#define BATTERY_SEGMENT_HIGH_MAX       74U
#define BATTERY_LEVEL_EMPTY            1U
#define BATTERY_LEVEL_25               2U
#define BATTERY_LEVEL_50               3U
#define BATTERY_LEVEL_75               4U
#define BATTERY_LEVEL_FULL             5U
#define BRIGHTNESS_STEP                50
#define BRIGHTNESS_MAX                 250
#define ANGLE_STORAGE_SCALE            10000000.0f
#define BALLISTIC_TABLE_ENTRY_SIZE     6U
#define BALLISTIC_TABLE_BYTES          800U
#define BALLISTIC_TABLE_START_PAGE     1U
#define BALLISTIC_TABLE_PAGES_PER_TAB  5U
#define COMM_CRC_ERROR_OPCODE          0x55U
#define EEPROM_TEST_ADDRESS            0x0102U
#define RAW_ANGLE_MAX_COUNT            16383.0f
#define RAW_ANGLE_FULL_SCALE_DEG       360.0f
#define DEBUG_SAMPLE_COUNT             100U
#define DEBUG_SAMPLE_DELAY_MS          10U


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
void configure_button_to_io(void);
void configure_button_to_interrupt(void);

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
// bat stat in 0%-100%
static uint8_t get_battery_level(uint8_t battery_percentage)
{
    if (battery_percentage <= BATTERY_SEGMENT_EMPTY_MAX)
        return BATTERY_LEVEL_EMPTY;
    if (battery_percentage <= BATTERY_SEGMENT_LOW_MAX)
        return BATTERY_LEVEL_25;
    if (battery_percentage <= BATTERY_SEGMENT_MID_MAX)
        return BATTERY_LEVEL_50;
    if (battery_percentage <= BATTERY_SEGMENT_HIGH_MAX)
        return BATTERY_LEVEL_75;

    return BATTERY_LEVEL_FULL;
}

static void draw_vertical_battery_segments(uint8_t level)
{
    if (bat_per_prev == level)
        return;

    clear_bat();
    bat_per_prev = level;

    if (level >= BATTERY_LEVEL_25)
        fill_rect(BAT_START_W_VER+BAT_WIDTH_VER-5,BAT_START_H_VER+2,3,3);
    if (level >= BATTERY_LEVEL_50)
        fill_rect(BAT_START_W_VER+BAT_WIDTH_VER-10,BAT_START_H_VER+2,3,3);
    if (level >= BATTERY_LEVEL_75)
        fill_rect(BAT_START_W_VER+BAT_WIDTH_VER-15,BAT_START_H_VER+2,3,3);
    if (level >= BATTERY_LEVEL_FULL)
        fill_rect(BAT_START_W_VER+BAT_WIDTH_VER-20,BAT_START_H_VER+2,3,3);
}

static uint8_t map_large_font_character(unsigned char c)
{
    switch (c)
    {
        case 'm': return 10;
        case 'H': return 11;
        case 'I': return 12;
        case 'E': return 13;
        case 'R': return 14;
        case ' ': return 15;
        case 'L': return 16;
        case 'O': return 17;
        case 'W': return 18;
        default: return (uint8_t)(c - '0');
    }
}

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

    draw_vertical_battery_segments(get_battery_level(bat_stat));
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

    letter_index = map_large_font_character(c);
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

    letter_index = map_large_font_character(c);
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

