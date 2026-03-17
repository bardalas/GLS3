
/* DEFS */

//command: 
//         15  14  13 12 .... LSB 
//         PAR RWn    address <13:0>
// Read=1
// when reading bit 14 is error  bit (1=error)
// 
//data:
//         15 14 13 12 ..... LSB
//         PAR 0      data <13:0>


// angle range 0x0000-0x3FFF (16383->, corresponds to 0-360 

#define NOP_REG             0x0000
#define CLR_ERR_REG         0x0001
#define PRG_CTRL_REG        0x0003
#define OTP_ZP_H_REG        0x0016
#define OTP_ZP_L_REG        0x0017
#define MAG_REG             0x3FFE
#define ANGLE_REG           0x3FFF
//#define ANG_PER_BIT         0.021973F

/* DECS */

extern float zero_angle;
extern uint16_t angle_raw;

uint16_t read_one_angle(void);
float read_angle(uint16_t num_of_samps);
bool analyze_encoder_error(uint16_t payload);
uint8_t AS5048A_CalcEvenParity(uint16_t value);
void as5048a_spi_init_seq(void);
uint16_t read_one_angle_look_for_error(void);

/* FUNCS */
uint8_t AS5048A_CalcEvenParity(uint16_t value)
{
	uint8_t cnt = 0;
	uint8_t i;

	for (i = 0; i < 16; i++)
	{
		if (value & 0x1)
		{
			cnt++;
		}
		value >>= 1;
	}
	return cnt & 0x1;
}

void as5048a_spi_init_seq(void)
{
    read_one_angle();
    SPI2CONbits.ON=0b0;
    SPI2CONbits.CKP=0b0; // clock polarity 0=idle is low
    //SPI2CONbits.CKE=0b1;  // 1=data changes on falling edge; 0=data changes on rising edge 
    //SPI2CONbits.SMP=0b0;// sampled on rising edge of clock  (0b0=end)
    SPI2CONbits.ON=0b1;
    //read_one_angle();
    //SPI2CONbits.ON=0b0;
    //SPI2CONbits.CKP=0b1; // clock polarity 0=idle is low
    //SPI2CONbits.ON=0b1;
}
bool analyze_encoder_error(uint16_t payload)
{
    //CLR_ERR_REG with bit 14 set to 1 
    if ((payload&0x4000)==0x4000) // clear error  4000 = 0b0100 0000 0000 0000
    {
        ANG_nCS=0;
        __delay_us(1);
        SPI2_ExchangeWord_16bitmode(0xC001); // PAR=1 (even) // 0b1100 0001
        ANG_nCS=1;
        __delay_us(1);
        return true;
    }
        
    else 
        return false; 
}

/*
 * For a single READ command two transmission sequences are necessary.
 * The first package contains the READ command and the address. 
 * the second package can be any command the chip has to process next.
 * 1
// I found my error, I had the SPI mode wrong ! Be sure to be CPOL = 0 CPHA = 1
 * angle at 3fff
 * even parity. meaning it's set to 1 if the total 1s is even 
 */
uint16_t read_one_angle(void) // returns raw value 0-16383 (14bit))
{
    uint16_t ang_res;
    ANG_nCS=0;
    //SPI2_Write_custom(0x40|0x3F); // 0x7F read angle command  MSB ; even parity->PAR=0
    //SPI2_Write_custom(0xFF);      // LSB
    __delay_us(2);
    ang_res=SPI2_ExchangeWord(0x7FFF); // next command is read too
    ANG_nCS=1;
    __delay_us(2);
    analyze_encoder_error(ang_res); // check if error flag and clear
    ang_res=0x3FFF&ang_res;
    return ang_res; 
}

uint16_t read_one_angle_look_for_error(void) // returns raw value 0-16383 (14bit))
{
    uint16_t ang_res;
    ANG_nCS=0;
    //SPI2_Write_custom(0x40|0x3F); // 0x7F read angle command  MSB ; even parity->PAR=0
    //SPI2_Write_custom(0xFF);      // LSB
    __delay_us(2);
    ang_res=SPI2_ExchangeWord(0x7FFF); // next command is read too
    ANG_nCS=1;
    __delay_us(2);
    if (analyze_encoder_error(ang_res))        // check if error flag and clear
        return 0xFFFF;
    ang_res=0x3FFF&ang_res;
    return ang_res; 
}
uint16_t read_one_angle_dis_error(void) // returns raw value 0-16383 (14bit))
{
    uint16_t ang_res;
    uint8_t retries=50;
   
    while (retries>1)
    {
        __delay_us(10);
        ANG_nCS=0;
        __delay_us(1);
        SPI2_ExchangeWord(0xFFFF); // next command is read too
        ANG_nCS=1;
        __delay_us(10);
        ANG_nCS=0;
        ang_res=SPI2_ExchangeWord(0x0000); // next command is read too
        ANG_nCS=1;
        __delay_us(1);
        if (analyze_encoder_error(ang_res)) // check if error flag and clear
            retries--;
        else 
            break;
    }
    //if (retries==1)
    //    ang_res=0xFFFF;
    //else
    ang_res=0x3FFF&ang_res;
    return ang_res; 
}

float read_angle(uint16_t num_of_samps)
{
    uint32_t val=0;
    uint32_t sum=0;
    uint16_t i;
    
    for (i=0;i<num_of_samps;i++){
        val=read_one_angle_dis_error();
        sum+=val;
      //  if (print == 1)sprintf(PCDebug,"read_one_angle_dis_error: %u",val);send_string_UART2(PCDebug);
    }
    
    angle_raw=(uint16_t)(sum/(0x0000FFFF&(uint32_t)num_of_samps));
    
    float angle_deg = (( ((float)angle_raw) / 16383.0) * 360.0);
    
    // shift by zero_angle
    
    // zero_angle = 355 and angle increases with rotation: then 5 deg -> -350 deg  so add 360
    // if still increases and zero angle is 10 then 20-10 -> 10 ->  all good 
    //                  5-355+360 = 10 deg 
    
    // if decreases then zero_angle=5 ; read angle=355 -> we will get 350 deg but it needs to be
    //                  360-350 = 10 deg 
    
    
    // 23.1.2025: angle is increasing so: 
    // zero_angle stores the zero level angle 
    
    // example: zero_angle=5; 
    // read angle = 10 -> all good 
    
    // zero_angle=355;
    // read_angle =10 -> -345 -> not good 
    // solution: add 360 
    
    // sometimes it might skip under zero_angle-> so equal zero 
    
    // z=187 degrees
    // result=11 , so before: 
    
    // positive direction
    
        if ((angle_deg<zero_angle)&&(zero_angle<250.0))
            angle_deg=0;
        else 
            angle_deg=angle_deg-zero_angle; 

        if (angle_deg<0.0)
            angle_deg+=360; 
    
    // negative direction
    /*if ((angle_deg>zero_angle)&&((angle_deg-zero_angle)<5))
        angle_deg=0;
    else 
        angle_deg=zero_angle-angle_deg; 
    if (angle_deg<0.0)
        angle_deg+=360; 
    */
 //   if (print == 1) sprintf(PCDebug,"read_one_angle_dis_error: %u",angle_deg);send_string_UART2(PCDebug);
    
    return (angle_deg);
}

float read_angle_for_zero(uint16_t num_of_samps)
{
    uint32_t sum=0;
    uint16_t i,angle_raw;
    
    for (i=0;i<num_of_samps;i++)
        sum+=read_one_angle();
    
    angle_raw=(uint16_t)(sum/(0x0000FFFF&(uint32_t)num_of_samps));
    
    float angle_deg = (( ((float)angle_raw) / 16383.0) * 360.0);
    
    // shift by zero_angle
    
    // zero_angle = 355 and angle increases with rotation: then 5 deg -> -350 deg  so add 360
    // if still increases and zero angle is 10 then 20-10 -> 10 ->  all good 
    //                  5-355+360 = 10 deg 
    
    // if decreases then zero_angle=5 ; read angle=355 -> we will get 350 deg but it needs to be
    //                  360-350 = 10 deg 
    
    
    // 23.1.2025: angle is increasing so: 
    // zero_angle stores the zero level angle 
    
    // example: zero_angle=5; 
    // read angle = 10 -> all good 
    
    // zero_angle=355;
    // read_angle =10 -> -345 -> not good 
    // solution: add 360 
    
    // sometimes it might skip under zero_angle-> so equal zero 
    
    // z=187 degrees
    // result=11 , so before: 
    
    //if ((angle_deg<zero_angle)&&(zero_angle<180.0))
    //    angle_deg=0;
    //else 
    //    angle_deg=angle_deg-zero_angle; 
    //
    //if (angle_deg<0.0)
    //    angle_deg+=360; 
    
    
    return (angle_deg);
}