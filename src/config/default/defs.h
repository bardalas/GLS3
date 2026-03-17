/* GENERAL */
#define _XTAL_FREQ 50000000UL
#define TICK_PER_MS (_XTAL_FREQ/2/1000)
#define TICK_PER_uS (_XTAL_FREQ/2/1000000)
#define PRINT_INTERVAL 100UL

/* PINS */
// PORT B
#define TEST_PIN        LATBbits.LATB1 // was RB11 in EVB
#define BAT_AN_SAMP     PORTBbits.RB2 // new in JIG
#define ANG_nCS         LATBbits.LATB3 // was RB5
#define MEMS_nCS        LATBbits.LATB4
#define LCD_EN          LATBbits.LATB5 // new in JIG

// PORT D
#define TXD             PORTDbits.RD0
#define DCDC_nSHDN      LATDbits.LATD2
#define RXD             PORTDbits.RD9

// PORT E
#define RES_nCS         LATEbits.LATE0
#define BUTTON_ONE      PORTEbits.RE1  // was rf4
#define LIS_nINT        PORTEbits.RE3
#define EEPROM_CS       LATEbits.LATE4 // new in JIG
#define BUTTON_TWO      PORTEbits.RE5  // was rb13
#define SSD13003_Rst    LATEbits.LATE6
#define SSD13003_DC     LATEbits.LATE7

// PORT F


// PORT G
#define SSD13003_CS     LATGbits.LATG9
/* ADC */

#define ADC_VREF                (2.048f)
#define ADC_MAX_COUNT           (4095)

/* NumS */

#define OLED_WIDTH          64       // Width of the OLED display in pixels
#define OLED_HEIGHT         48       // Height of the OLED display in pixels
#define SSD13003_WIDTH      64
#define SSD13003_HEIGHT     48
#define FONT_WIDTH          8         // Width of each character in pixels
#define FONT_HEIGHT         16       // Height of each character in pixels

// drawings
#define BAT_START_W         3
#define BAT_START_H         12
#define BAT_WIDTH           8
#define BAT_HEIGHT          22
#define BAT_START_W_VER     38
#define BAT_START_H_VER     3
#define BAT_WIDTH_VER       22
#define BAT_HEIGHT_VER      7

#define ROLL_START_W        13
#define ROLL_START_H        40
#define ROLL_WIDTH          39
#define ROLL_HEIGHT         6

#define TABLE_START_W       5
#define TABLE_START_H       5

#define RANGE_START_W       10
#define RANGE_START_H       17

#define ROLL_M5_5   0
#define ROLL_6_10   1
#define ROLL_11_15  2
#define ROLL_16_20  3
#define ROLL_M16_M20    4
#define ROLL_M11_M15    5
#define ROLL_M6_M10 6
#define BLINK_RIGHT 7
#define BLINK_LEFT 8


#define LARGE_CHAR_W 10
#define LARGE_CHAR_H 14

// Define the _BV macro for PIC
#define _BV(bit) (1 << (bit))
// Define the XOR mode (you should assign it an appropriate value)
#define PAGE_BUFFER_SIZE (SSD13003_WIDTH * (SSD13003_HEIGHT / 8))


// Define your SPI commands and addresses here
#define OLED_COMMAND_MODE  0x00
#define OLED_DATA_MODE     0x40
#define OLED_SET_PAGE_ADDR 0xB0
#define OLED_SET_COL_ADDR  0x00


#define TABLE_SIZE  135
#define SHORT_BUTTON  500UL // 500ms
#define ZERO_TIME    15000UL // 15s
#define OFFTIME      500UL
#define MAX_NUM_TABLES 5
#define SLEEP_TIME 60000UL // 60 seconds 