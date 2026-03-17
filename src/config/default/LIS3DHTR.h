
/* DEFS */

#define STATUS_REG_AUX 0x07
#define OUT_ADC3_L     0x0D
#define WHO_AM_I       0x0F  // = 0x33 = 51 dec
#define CTRL_REG0      0x1E
#define CTRL_REG1      0x20
#define CTRL_REG2      0x21
#define CTRL_REG3      0x22
#define CTRL_REG4      0x23
#define CTRL_REG5      0x24
#define CTRL_REG6      0x25
#define STATUS_REG     0x27
#define OUT_X_L        0x28
#define OUT_X_H        0x29
#define OUT_Y_L        0x2A
#define OUT_Y_H        0x2B
#define OUT_Z_L        0x2C
#define OUT_Z_H        0x2D



/* FUNCS */
void init_LIS(void);
void LIS_write_register(uint8_t add, uint8_t data);
uint8_t LIS_read_register(uint8_t add);
int16_t LIS_read_x(void);
int16_t LIS_read_y(void);
// first add/cmd byte:
//  bit 7: RnW bit -> 0=write 1=read
//  bit 6: MnS bit -> 1=address auto increment 
//  bit 5-0: address (AD 5:0)
//  bit 8-15: data to read/write ; MSbit first

uint8_t LIS_read_register(uint8_t add)
{
    uint8_t byte_to_send,reg_data;
    
    byte_to_send=add|0x80; // set to read
    byte_to_send&=0xBF;    // no auto increment 
    SSD13003_CS=1;         // debug- make sure it's disabled 
    MEMS_nCS=0;            // CS MEMS
    SPI2_Write_custom(byte_to_send); // send address with read command 
    reg_data=SPI2_Read_custom();
    MEMS_nCS=1;            // CDS MEMS
    
    return reg_data;
}
int16_t LIS_read_x(void) 
{
    uint8_t  MSB,LSB;
    int16_t res;
    
    MSB=LIS_read_register(OUT_X_H);
    LSB=LIS_read_register(OUT_X_L);
    
    res = ((((int16_t)MSB)<<8)&0xFF00)|(((int16_t)LSB)&0x00FF);
    return res;
}
int16_t LIS_read_y(void) 
{
    uint8_t  MSB,LSB;
    int16_t res;
    
    MSB=LIS_read_register(OUT_Y_H);
    LSB=LIS_read_register(OUT_Y_L);
    
    res = ((((int16_t)MSB)<<8)&0xFF00)|(((int16_t)LSB)&0x00FF);
    return res;
}
void LIS_write_register(uint8_t add, uint8_t data)
{
    uint8_t byte_to_send;
    
    byte_to_send=add&0x7F; // set to write 
    byte_to_send&=0xBF;    // no auto increment 
    MEMS_nCS=0;            // CS MEMS
    SPI2_Write_custom(byte_to_send);
    SPI2_Write_custom(data);
    MEMS_nCS=1;    
    
    return;
}
void init_LIS(void)
{
    LIS_write_register(CTRL_REG0,0x90); // disable pull up
    LIS_write_register(CTRL_REG1,0x57); // 100Hz (0b0101) 0b01011111
    
}