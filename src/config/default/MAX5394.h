
// first 8 bits - command
// then another 8 bits - data


/* DEFS */
// RES_nCS is RE0

#define WIPER_CMD       0x00
#define SD_CLR_CMD      0x80
#define SD_H_WREG_CMD   0x90
#define SD_H_ZERO_CMD   0x91
#define SD_H_MID_CMD    0x92
#define SD_H_FULL_CMD   0x93
#define SD_L_WREG_CMD   0x88
#define SD_L_ZERO_CMD   0x89
#define SD_L_MID_CMD    0x8A
#define SD_L_FULL_CMD   0x8B
#define SD_W_CMD        0x84
#define QP_OFF_CMD      0xA0
#define QP_ON_CMD       0xA1
#define QP_RST_CMD      0xC0



/* DECS */
// 0x00 -> L
// 0xFF -> H
void pot_set_resistenace(uint8_t val)
{
        
    RES_nCS=0;            // CS 
    SPI2_Write_custom(WIPER_CMD);
    SPI2_Write_custom(val);
    RES_nCS=1;    
}
void open_l_terminal(void)
{
    
    RES_nCS=0;            // CS 
    SPI2_Write_custom(SD_L_WREG_CMD);
    SPI2_Write_custom(0x00);
    RES_nCS=1; 
}
void pot_open_wiper(void)
{
    RES_nCS=0;            // CS 
    SPI2_Write_custom(SD_W_CMD);
    SPI2_Write_custom(0x00);
    RES_nCS=1; 
}
void clr_pot_sd(void)
{
    RES_nCS=0;            // CS 
    SPI2_Write_custom(SD_CLR_CMD);
    SPI2_Write_custom(0x00);
    RES_nCS=1; 
}
void pot_reset(void)
{
    RES_nCS=0;            // CS 
    SPI2_Write_custom(QP_RST_CMD);
    SPI2_Write_custom(0x00);
    RES_nCS=1; 
}