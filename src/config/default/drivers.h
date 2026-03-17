/* DEFS */
char send_string_UART2(char *str);
uint8_t SPI2_ExchangeByte(uint8_t data);
char write_byte_spi2(char reg,char data);
char get_byte_spi2(char data);
void SPI2_Write_custom(uint8_t data);
uint8_t SPI2_Read_custom(void);
char send_byte_UART2(char data);
uint16_t SPI2_ExchangeWord(uint16_t data);
uint16_t SPI2_ExchangeWord_16bitmode(uint16_t data);



char send_string_UART2(char *str)
{
    while ((*str != '\0'))
	{
		//while (!TX1STAbits.TRMT);
        //while (!PIR1bits.TX1IF); 
        //while ((U2STA & _U2STA_UTXBF_MASK) != 0U){} //#define _U2STA_UTXBF_MASK 0x00000200 
            //->bit 9 -> UTXBF
        while ((U2STA & _U2STA_UTXBF_MASK) != 0U);
            //while (!IFS4bits.U2TXIF);
		U2TXREG=*str++;
	}
        return 0;
}

// working configuration for lcd: 
//           clock idle = high
//           MOSI changes from active to idle (low->high/ rising)
//           MISO sampled at middle of data 

// encoder:
//            MOSI sampled on falling
//            MISO changes on rising ; sampled on falling 
//
// lcd: MOSI on rising 
// mems: both mosi and miso on rising 

// mode 0: rising mode 1: falling 

// mode 2: MISO: Data is captured (read) on the falling edge of SCK.
//         MOSI: Data is shifted (changed) on the rising edge of SCK.
// 
// mode 1:
//

uint8_t SPI2_ExchangeByte(uint8_t data)
{
    while((SPI2STAT & _SPI2STAT_SPITBE_MASK) == 0U)
    {(void)SPI2BUF;/* Wait for transmit buffer to be empty */}
    SPI2BUF = data; // write to buffer tx 
    /* If data is read, wait for the Receiver Data the data to become available */
    while((SPI2STAT & _SPI2STAT_SPIRBE_MASK) == _SPI2STAT_SPIRBE_MASK)
    {/* Do Nothing */}
    return SPI2BUF;
}


uint16_t SPI2_ExchangeWord(uint16_t data)
{
    uint16_t MSB=0,LSB=0;
    
    while((SPI2STAT & _SPI2STAT_SPITBE_MASK) == 0U){}
    //{(void)SPI2BUF;/* Wait for transmit buffer to be empty */}
    SPI2BUF = (uint8_t)(data>>8); // write to buffer tx  - send MSB 
    /* If data is read, wait for the Receiver Data the data to become available */
    while((SPI2STAT & _SPI2STAT_SPIRBE_MASK) == _SPI2STAT_SPIRBE_MASK)   {/* Do Nothing */}
    MSB=SPI2BUF; // get data 
    while((SPI2STAT & _SPI2STAT_SPITBE_MASK) == 0U) {} //{(void)SPI2BUF;/* Wait for transmit buffer to be empty */}
    SPI2BUF = (uint8_t)(data); // send LSB
    while((SPI2STAT & _SPI2STAT_SPIRBE_MASK) == _SPI2STAT_SPIRBE_MASK){}
    LSB=SPI2BUF; // get data
    return ((MSB<<8)&0xFF00)|(0x00FF&LSB);
}
uint16_t SPI2_ExchangeWord_16bitmode(uint16_t data)
{
    uint16_t MSB=0,LSB=0;
    
   
    while((SPI2STAT & _SPI2STAT_SPITBE_MASK) == 0U){}
    //{(void)SPI2BUF;/* Wait for transmit buffer to be empty */}
    SPI2BUF = data;
    /* If data is read, wait for the Receiver Data the data to become available */
    while((SPI2STAT & _SPI2STAT_SPIRBE_MASK) == _SPI2STAT_SPIRBE_MASK)
    {/* Do Nothing */}
    MSB=(uint16_t)(0x0000FFFF&SPI2BUF); // get data 
    while((SPI2STAT & _SPI2STAT_SPITBE_MASK) == 0U) {} //{(void)SPI2BUF;/* Wait for transmit buffer to be empty */}
 
    return MSB;
}
char send_byte_UART2(char data)
{
   
    //while (!IFS4bits.U2TXIF);
    while ((U2STA & _U2STA_UTXBF_MASK) != 0U);
	U2TXREG=data;
    return 0;
}

void SPI2_Write_custom(uint8_t data)
{
    while((SPI2STAT & _SPI2STAT_SPITBE_MASK) == 0U)
    {(void)SPI2BUF;/* Wait for transmit buffer to be empty */}
    SPI2BUF = data; // write to buffer tx 
    /* If data is read, wait for the Receiver Data the data to become available */
    while((SPI2STAT & _SPI2STAT_SPIRBE_MASK) == _SPI2STAT_SPIRBE_MASK)
    {/* Do Nothing */}
    return SPI2BUF;
}


uint8_t SPI2_Read_custom(void)
{
    while((SPI2STAT & _SPI2STAT_SPITBE_MASK) == 0U)
    {(void)SPI2BUF;/* Wait for transmit buffer to be empty */}
    SPI2BUF = 0xAA; // write to buffer tx 
    /* If data is read, wait for the Receiver Data the data to become available */
    while((SPI2STAT & _SPI2STAT_SPIRBE_MASK) == _SPI2STAT_SPIRBE_MASK)
    {/* Do Nothing */}
    return SPI2BUF;
}