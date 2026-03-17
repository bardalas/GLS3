
/* DEFS */

#define RDSR_INST   0x05
#define WREN_INST   0x06
#define READ_INST   0x03
#define WRITE_INST  0x02
#define WRSR        0x01
#define WR_DIS      0x04
#define RDID_INST   0x83
#define ST_MF_ADD   0x00 // ST MNF ID is 0x20

#define TABLE1_ADD 0x0100
#define TABLE2_ADD 0x0600
#define TABLE3_ADD 0x0B00
#define TABLE4_ADD 0x1100
#define TABLE5_ADD 0x1600

#define BRIGHT_ADD 0x000A
#define ACTIVE_TAB 0x000B
#define TAB1N      0x000C
#define TAB2N      0x0010
#define TAB3N      0x0014
#define TAB4N      0x0018
#define TAB5N      0x001C
#define LAST_TABLE 0x0020
#define CAL_VALUE  0x0030
#define ACT_SLP    0x0040
/* EEPROM MEMORY MAP*/
/*
 * 128Kbits total
 * Page size: 64 bytes 
 * 0x0000 - 0x3FFF
 * 
 */

/* ballistic table 
 * there are ~80 rows
 * in each row distance (2 bytes)
 * and angle (4 bytes will be enough) 
 * 
 * total: 6*84=840bytes 
 *          76 rows are for 400 meters (400-25)/5=375/5=75
 * 
 * we clear ~ 1000 bytes for each table, so no problem for more
 * 
 * so table 1 will start at address: 0x0100-0x0500 (256 dec-1280dec) 
 *                         table 2 : 0x0600-0x0A00
 *                         table 3 : 0x0B00-0x0F00
 *                         table 4 : 0x1100-0x1500
 *                         table 5:  0x1600-0x1A00
 *                  
 *                          table 1 rows count: 0x0000
 *                          table 2 rows count: 0x0002
 *                          table 3 rows count: 0x0004
 *                          table 4 rows count: 0x0006
 *                          table 5 rows count: 0x0008
 *          
 *                          brightness level:   0x000A
 *                          active tables:      0x000B
 *                          table 1 name:       0x000C
 *                          table 2 name:       0x0010
 *                          table 3 name:       0x0014
 *                          table 4 name:       0x0018
 *                          table 5 name:       0x001C
 *              
 *                          last table used:    0x0020
 * 
 *                          calibratino value:  0x0030  (calc_angle=uint32_angle/((float)10000000);)
 *                          
 *                          activate sleep:     0x0040
 */         

/* DECS */
uint8_t read_M95128_ID(void);
void write_inst_eeprom(char inst);
uint8_t read_byte_eerpom(uint16_t add);
void write_byte_eerpom(uint16_t add,uint8_t data);
void send_table_to_pc(uint16_t sel_table);
void send_table_to_pc_monitor(uint16_t sel_table);
void write_table_row_debug(char *Comm);
void write_table_to_row(char *Comm);
void  write_table_rows_count(char *Comm);
void  write_table_rows_count_debug(char *Comm);
void  read_table_rows_count_debug(void);
void  read_table_rows_count(uint8_t sel_table);
void load_table_names(void);
/* FUNCS */
void load_table_names(void)
{
    uint8_t sel_table,ct;
    for (sel_table=1;sel_table<=5;sel_table++)
        for (ct=0;ct<=3;ct++)
            table_names[sel_table-1][ct]=
                    read_byte_eerpom((uint16_t)ct+(uint16_t)TAB1N+(uint16_t)(4*(sel_table-1)));
  
}
void  read_table_rows_count(uint8_t sel_table)
{
   uint8_t MSB,LSB,ct;   
   uint8_t msg_buffer[20];
   
    MSB=read_byte_eerpom(2*(sel_table-1));LSB=read_byte_eerpom(2*(sel_table-1)+1);
    
    msg_buffer[0]=0x26; msg_buffer[1]=0x05;
    msg_buffer[2]=MSB; msg_buffer[3]=LSB;
    msg_buffer[4]=CalcCRC(4,msg_buffer);
            
   for (ct=0;ct<=4;ct++)
       send_byte_UART2(msg_buffer[ct]);
    
    //rows_count=(0x00FF&((uint16_t)LSB)) | (0xFF00&(((uint16_t)MSB)<<8));
    //sprintf(PCDebug,"rows count for table %u: %u\n",sel_table,rows_count);send_string_UART2(PCDebug);
   
   
}
void  read_table_rows_count_debug(void)
{
    uint8_t sel_table,MSB,LSB;   
    uint16_t rows_count,sel_add;
   
    for (sel_table=1;sel_table<=5;sel_table++)
    {
        MSB=read_byte_eerpom(2*(sel_table-1));LSB=read_byte_eerpom(2*(sel_table-1)+1);
        rows_count=(0x00FF&((uint16_t)LSB)) | (0xFF00&(((uint16_t)MSB)<<8));
        sprintf(PCDebug,"rows count for table %u: %u\n",sel_table,rows_count);send_string_UART2(PCDebug);
    }
   
}
void  write_table_rows_count_debug(char *Comm)
{
    uint8_t sel_table,MSB,LSB;   
    uint16_t rows_count,sel_add;
    
    sel_table=*Comm++;
    MSB=*Comm++; LSB=*Comm++;
    rows_count=(0x00FF&((uint16_t)LSB)) | (0xFF00&(((uint16_t)MSB)<<8));
    sel_add=2*(sel_table-1);
   
    sprintf(PCDebug,"writing count %u to add %u\n",rows_count,sel_add);send_string_UART2(PCDebug);
}
void  write_table_rows_count(char *Comm)
{
    uint8_t sel_table,MSB,LSB;   
    uint16_t rows_count,sel_add;
    
    sel_table=*Comm++;
    MSB=*Comm++; LSB=*Comm++;
    //rows_count=(0x00FF&((uint16_t)LSB)) | (0xFF00&(((uint16_t)MSB)<<8));
    sel_add=2*(sel_table-1);
    write_byte_eerpom(sel_add++,MSB);
    write_byte_eerpom(sel_add++,LSB);
}
void write_table_to_row(char *Comm)
{
    uint8_t sel_table,MSB,LSB,secbyte;//,thirdbyte;
    uint16_t table_row,sel_add; //,table_range;
    //uint32_t table_angle; 
    
    sel_table=*Comm++;
    MSB=*Comm++; LSB=*Comm++;
    table_row=(0x00FF&((uint16_t)LSB)) | (0xFF00&(((uint16_t)MSB)<<8));
    sel_add=(uint16_t)256*((uint16_t)1+(uint16_t)5*((uint16_t)sel_table-(uint16_t)1));
    sel_add=sel_add+(uint16_t)6*((uint16_t)table_row-1); // update to row address
    write_byte_eerpom(sel_add++,*Comm++); // range MSB
    write_byte_eerpom(sel_add++,*Comm++); // range LSB
    write_byte_eerpom(sel_add++,*Comm++); // ang 1
    write_byte_eerpom(sel_add++,*Comm++); // ang 2
    write_byte_eerpom(sel_add++,*Comm++); // ang 3
    write_byte_eerpom(sel_add++,*Comm); // ang 4
}
void write_table_row_debug(char *Comm)
{
    uint8_t sel_table,MSB,LSB,secbyte,thirdbyte;
    uint16_t table_row,sel_add,table_range;
    uint32_t table_angle; 
    
   
    sel_table=*Comm++;
    MSB=*Comm++; LSB=*Comm++;
    table_row=(0x00FF&((uint16_t)LSB)) | (0xFF00&(((uint16_t)MSB)<<8));
    sel_add=(uint16_t)256*((uint16_t)1+(uint16_t)5*((uint16_t)sel_table-(uint16_t)1));
    sprintf(PCDebug,"\nsel add start: 0x%04x\n",sel_add);send_string_UART2(PCDebug);
    sel_add=sel_add+(uint16_t)6*((uint16_t)table_row-1);
    sprintf(PCDebug,"sel add row: 0x%04x\n",sel_add);send_string_UART2(PCDebug);
    sprintf(PCDebug,"table row MSB,LSB: 0x%02x , 0x%02x\n",MSB,LSB);send_string_UART2(PCDebug);
    MSB=*Comm++;
    LSB=*Comm++;
    table_range=(0x00FF&((uint16_t)LSB)) | (0xFF00&(((uint16_t)MSB)<<8));
    sprintf(PCDebug,"range MSB,LSB: 0x%02x , 0x%02x\n",MSB,LSB);send_string_UART2(PCDebug);
   
    MSB=*Comm++;
    secbyte=*Comm++;
    thirdbyte=*Comm++;
    LSB=*Comm;
    table_angle=(0x000000FF&((uint32_t)LSB)) | 
      (0x0000FF00&(((uint32_t)thirdbyte)<<8))|
      (0x00FF0000&(((uint32_t)secbyte)<<16))|
      (0xFF000000&(((uint32_t)MSB)<<24));
            sprintf(PCComm,"Rx table %u -> row: %u ; range: %u ; angle: %.4f\n",
                   sel_table,table_row,table_range,((float)table_angle)/((float)10000000));
    send_string_UART2(PCComm);
}
void send_table_to_pc_monitor(uint16_t sel_table)
{
    char CRC;
    uint16_t sel_add,add_ct,start_from;
    uint8_t part_ct;
    uint8_t table_buffer[840]; // table size
    uint8_t msg_buffer[214];   // divide table to 4 msgs 
                               // opcode+length+table subpart:210 bytes+CRC
    
    // sel_add (sel_table=1)=256*(1+5*(sel_table-1))=256->0x0100 ; 
    // sel_add (sel_table=2)=256*(1+5*(sel_table-1))=1536->0x0600
    sel_add=(uint16_t)256*((uint16_t)1+(uint16_t)5*((uint16_t)sel_table-(uint16_t)1));
  
    for (add_ct=sel_add;add_ct<sel_add+840;add_ct++)
        table_buffer[add_ct-sel_add]=read_byte_eerpom(add_ct);
   
    sprintf(PCComm,"\nreading table %u\n",sel_table); send_string_UART2(PCComm);
    sprintf(PCComm,"\tstart address: 0x%04x\n",sel_add); send_string_UART2(PCComm);
    msg_buffer[0]=0x19;
    msg_buffer[1]=214;
    
    for (part_ct=1;part_ct<=4;part_ct++)
    {
        msg_buffer[2]=part_ct; // part x out of 4 
        start_from=210*(part_ct-1); // 0, 210,420, 640
        sprintf(PCComm,"\tpart %u\n",part_ct); send_string_UART2(PCComm);
        for (add_ct=start_from;add_ct<210*part_ct;add_ct++) // send 210 bytes each time
            msg_buffer[3+add_ct-start_from]=table_buffer[add_ct]; // start after opcode,length and part
        CRC=CalcCRC(213,msg_buffer); 
        msg_buffer[213]=CRC;
        send_string_UART2("\t\t");
        for (add_ct=0;add_ct<=213;add_ct++)
            {sprintf(PCComm,"%u,",msg_buffer[add_ct]); send_string_UART2(PCComm);} 
        send_string_UART2("\n");
    }
}
void send_table_to_pc(uint16_t sel_table)
{
    char CRC;
    uint16_t sel_add,add_ct,start_from;
    uint8_t part_ct;
    uint8_t table_buffer[840]; // table size
    uint8_t msg_buffer[214];   // divide table to 4 msgs 
                               // opcode+length+table subpart:210 bytes+CRC
    
    // sel_add (sel_table=1)=256*(1+5*(sel_table-1))=256->0x0100 ; 
    // sel_add (sel_table=2)=256*(1+5*(sel_table-1))=1536->0x0600
    sel_add=(uint16_t)256*((uint16_t)1+(uint16_t)5*((uint16_t)sel_table-(uint16_t)1));
  
    for (add_ct=sel_add;add_ct<sel_add+840;add_ct++)
        table_buffer[add_ct-sel_add]=read_byte_eerpom(add_ct);
   
    msg_buffer[0]=0x19;
    msg_buffer[1]=214;
    
    for (part_ct=1;part_ct<=4;part_ct++)
    {
        msg_buffer[2]=part_ct; // part x out of 4 
        start_from=210*(part_ct-1); // 0, 210,420, 640
        for (add_ct=start_from;add_ct<210*part_ct;add_ct++) // send 210 bytes each time
            msg_buffer[3+add_ct-start_from]=table_buffer[add_ct]; // start after opcode,length and part
        CRC=CalcCRC(213,msg_buffer); 
        msg_buffer[213]=CRC;
        for (add_ct=0;add_ct<=213;add_ct++)
               send_byte_UART2(msg_buffer[add_ct]); // send msg 
    }
}


// CS
// Instruction: RDID
// address 0x0000
// get ID byte 
uint8_t read_M95128_ID(void)
{
    uint8_t read_val;
    EEPROM_CS=0;
    SPI2_ExchangeByte(RDID_INST);
    SPI2_ExchangeWord(0x0000);
    read_val=SPI2_ExchangeByte(0xA5);
    EEPROM_CS=1; 
    return read_val; 
}
uint8_t read_byte_eerpom(uint16_t add)
{
    uint8_t read_val=0;
    
  
    EEPROM_CS=0; 
    __delay_us(2);
    read_val=SPI2_ExchangeByte(READ_INST);  
    read_val=SPI2_ExchangeByte((uint8_t)(add>>8));  // address msb
    read_val=SPI2_ExchangeByte((uint8_t)add);       // address lsb
    read_val=SPI2_ExchangeByte(0x55); // dummy to get back register data
   __delay_us(2);
    EEPROM_CS=1;

    return read_val;
}
void write_byte_eerpom(uint16_t add,uint8_t data)
{
    uint8_t read_val;
    
    
    write_inst_eeprom(WREN_INST);
 
    EEPROM_CS=0; 
    __delay_us(2);
    read_val=SPI2_ExchangeByte(WRITE_INST);  
    read_val=SPI2_ExchangeByte((uint8_t)(add>>8));  // address msb
    read_val=SPI2_ExchangeByte((uint8_t)add);       // address lsb
    read_val=SPI2_ExchangeByte(data); // send to memory
    __delay_us(2);
    EEPROM_CS=1;
  
    __delay_ms(5); // write time
}
void write_inst_eeprom(char inst)
{
    char read_val;
  
    EEPROM_CS=0; 
    __delay_us(2);
    read_val=SPI2_ExchangeByte(inst);  // 0b01110111
    read_val=SPI2BUF; // clear buffer
    __delay_us(2);
    EEPROM_CS=1;
}
