static uint8_t calculate_battery_percentage(float voltage)
{
    float battery_pct = (100.0f * (voltage - BATTERY_PERCENT_MIN_VOLTAGE)) / BATTERY_PERCENT_RANGE_VOLTAGE;

    if (battery_pct < 0.0f)
        battery_pct = 0.0f;
    else if (battery_pct > 100.0f)
        battery_pct = 100.0f;

    return (uint8_t)battery_pct;
}

static void report_system_status(bool verbose)
{
    sprintf(PCDebug,"MEPRO: %u\n**********\n",main_cycle);send_string_UART2(PCDebug);
    if (verbose)
        sprintf(PCDebug,"\tENC ANG: %.4f ; raw:%u\n",cur_ang,angle_raw);
    else
        sprintf(PCDebug,"\tENC ANG: %.4f\n",cur_ang);
    send_string_UART2(PCDebug);
    sprintf(PCDebug,"\tLIS WHO AM I: %u\n",LIS_read_register(WHO_AM_I));send_string_UART2(PCDebug);
    sprintf(PCDebug,"\tRoll angle: %d\n",roll_angle);send_string_UART2(PCDebug);
    sprintf(PCDebug,"\tEEPROM ID:%u\n",0x00FF&read_M95128_ID());send_string_UART2(PCDebug);
    sprintf(PCDebug,"\tAN2 RAW:%u\n",adc_count);send_string_UART2(PCDebug);
    sprintf(PCDebug,"\tBAT IN: %.4f\n",input_voltage);send_string_UART2(PCDebug);
    if (verbose)
    {
        sprintf(PCDebug,"\tBAT percentage:%u\n",calculate_battery_percentage(input_voltage));
        send_string_UART2(PCDebug);
    }
}

static void update_brightness(int16_t delta)
{
    lcd_brightness += delta;
    if (lcd_brightness > BRIGHTNESS_MAX)
        lcd_brightness = BRIGHTNESS_MAX;
    else if (lcd_brightness < 0)
        lcd_brightness = 0;

    SSD13003_SetBrightness((uint8_t)lcd_brightness);
    sprintf(PCDebug,"\nbrightness:%u\n",lcd_brightness);send_string_UART2(PCDebug);
    write_byte_eerpom(BRIGHT_ADD,(uint8_t)lcd_brightness);
}

static uint32_t read_u32_from_eeprom(uint16_t address)
{
    return (((uint32_t)read_byte_eerpom(address)) << 24) |
           (((uint32_t)read_byte_eerpom(address + 1U)) << 16) |
           (((uint32_t)read_byte_eerpom(address + 2U)) << 8) |
           ((uint32_t)read_byte_eerpom(address + 3U));
}

static float decode_stored_angle(uint32_t stored_angle)
{
    return ((float)stored_angle) / ANGLE_STORAGE_SCALE;
}

static uint16_t get_ballistic_table_base_address(uint8_t selected_table)
{
    return (uint16_t)256U * (BALLISTIC_TABLE_START_PAGE + (BALLISTIC_TABLE_PAGES_PER_TAB * ((uint16_t)selected_table - 1U)));
}

static void load_ballistic_table_data(uint8_t selected_table)
{
    uint16_t table_address = get_ballistic_table_base_address(selected_table);
    uint16_t row_ct = 0;
    uint16_t add_ct;

    for (add_ct = table_address; add_ct < (table_address + BALLISTIC_TABLE_BYTES); add_ct += BALLISTIC_TABLE_ENTRY_SIZE)
    {
        table_data[row_ct].range = (uint16_t)read_byte_eerpom(add_ct) * 256U + read_byte_eerpom(add_ct + 1U);
        table_data[row_ct].angle = decode_stored_angle(read_u32_from_eeprom(add_ct + 2U));
        row_ct++;
    }
}

static void wait_for_buttons_release(void)
{
    while ((BUTTON_ONE) || (BUTTON_TWO)) {}
}

static bool handle_both_buttons(void)
{
    if ((BUTTON_ONE)&&(BUTTON_TWO)&&(!button_both_check))
    {
        button_both_check=true;
        button_timer_both=0;
        return false;
    }
    else if ((BUTTON_ONE)&&(BUTTON_TWO)&&(button_both_check)&&(button_timer_both>OFFTIME))
    {
        if (activate_sleep==0)
        {
            wait_for_buttons_release();
            return true;
        }

        DCDC_nSHDN=0;
        wait_for_buttons_release();
        goto_sleep();
#ifndef OPERATIONAL
        send_string_UART2("turn off display\n");
#endif
        return true;
    }
    else if (((!BUTTON_ONE)||(!BUTTON_TWO))&&(button_both_check))
    {
        button_both_check=false;
        return false;
    }

    return false;
}

static void handle_button_one(void)
{
    if ((BUTTON_ONE)&&(!button_one_check))
    {
        button_one_check=true;
        sleep_timer=0;
#ifndef OPERATIONAL
        send_string_UART2("button 1 pushed\n");
#endif
        button_timer_one=0;
    }
    else if ((!button_one_long_press)&&(!BUTTON_ONE)&&(button_one_check)&&(button_timer_one<=SHORT_BUTTON))
    {
        update_brightness(BRIGHTNESS_STEP);
        button_one_check=false;
    }
    else if ((BUTTON_ONE)&&(button_one_check)&&(button_timer_one>SHORT_BUTTON))
    {
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
}

static void handle_button_two(void)
{
    if ((BUTTON_TWO)&&(!button_two_check))
    {
        button_two_check=true;
        sleep_timer=0;
        button_timer_two=0;
#ifndef OPERATIONAL
        send_string_UART2("button 2 pushed\n");
#endif
    }
    else if ((!BUTTON_TWO)&&(button_two_check)&&(button_timer_two<=SHORT_BUTTON))
    {
        update_brightness(-BRIGHTNESS_STEP);
        button_two_check=false;
    }
    else if ((BUTTON_TWO)&&(button_two_check)&&(button_timer_two>ZERO_TIME))
    {
        write_str_LCD_large_font(RANGE_START_W,RANGE_START_H,"  O ");
        button_timer_two=0;
        button_two_check=false;
        calibrate_angle();
        while (BUTTON_TWO) {}
    }
    else if ((!BUTTON_TWO)&&(button_two_check))
    {
        button_two_check=false;
    }
}

void check_sleep_cycle(void)
{
    if (activate_sleep==0)
        return;

    if (sleep_timer>SLEEP_TIME)
    {
        goto_sleep();
        sleep_timer=0;
    }
}

void update_battery_status(void)
{
    uint8_t bat_per;

    ADCHS_ChannelConversionStart(ADCHS_CH2);
    while(!ADCHS_ChannelResultIsReady(ADCHS_CH2)) {};
    adc_count = ADCHS_ChannelResultGet(ADCHS_CH2);
    input_voltage = (float)2*(float)adc_count * (float)ADC_VREF / (float)ADC_MAX_COUNT;
    bat_per = calculate_battery_percentage(input_voltage);
    set_battery_display_vertical(bat_per);
}

void increment_loop_timers(void)
{
    sampletime++;
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
    if ((cur_range!=0) && (cur_range!=0xFFFF))
        cur_range=roundToNearest5(cur_range);
    if (cur_range==0)
        sprintf(cur_text,"LOW ",cur_range);
    else if (cur_range==0xFFFF)
        sprintf(cur_text," HI ",cur_range);
    else if (cur_range<100)
        sprintf(cur_text," %um",cur_range);
    else
        sprintf(cur_text,"%um",cur_range);
    write_str_LCD_large_font(RANGE_START_W,RANGE_START_H,cur_text);
}

uint16_t roundToNearest5(uint16_t value)
{
    return ((value + 2) / 5) * 5;
}

void all_init(void)
{
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

    zero_angle=load_cal_angle();
    clr_pot_sd();
    pot_set_resistenace(255);
    SSD13003_SetBrightness((uint8_t)lcd_brightness);
#ifndef OPERATIONAL
    send_string_UART2("\t\t display started\n");
#endif
    init_LIS();
    __builtin_enable_interrupts();
    IPC2bits.T2IP = 5;
    IPC2bits.T2IS = 1;
    while (IFS0bits.T2IF == 1){IFS0bits.T2IF=0;}
    IEC0bits.T2IE=0;
    IFS0bits.T2IF=0;
    T2CONbits.ON=1;
    as5048a_spi_init_seq();

    init_ballistic_table();
    active_tables=read_byte_eerpom(ACTIVE_TAB);
    load_table_names();
    write_str_LCD(TABLE_START_W,TABLE_START_H,table_names[table_num-1]);
    IEC0bits.INT4IE = 0;
    IFS0bits.INT4IF = 0;
    TRISEbits.TRISE5=1;
    DCDC_nSHDN=1;
    activate_sleep=read_byte_eerpom(ACT_SLP);
}

int16_t calculate_roll(void)
{
    int16_t x,y;
    x=LIS_read_x();
    y=LIS_read_y();
    return ((int16_t)(atan2(y, x) * (180.0 / M_PI)));
}

void print_sr(void)
{
#ifdef OPERATIONAL
    return;
#endif
    report_system_status(false);
}

float calibrate_angle(void)
{
    uint32_t save_cal_angle;

    zero_angle=read_angle_for_zero(50);
    save_cal_angle=(uint32_t)(zero_angle * ANGLE_STORAGE_SCALE);
    write_byte_eerpom(CAL_VALUE,(uint8_t)(save_cal_angle>>24));
    write_byte_eerpom(CAL_VALUE+1,(uint8_t)(save_cal_angle>>16));
    write_byte_eerpom(CAL_VALUE+2,(uint8_t)(save_cal_angle>>8));
    write_byte_eerpom(CAL_VALUE+3,(uint8_t)(save_cal_angle));
    return zero_angle;
}

void goto_sleep(void)
{
    configure_button_to_interrupt();
    LCD_EN=0;
    DCDC_nSHDN=0;
    open_l_terminal();
    __delay_ms(100);
    enterSleepMode();
    configure_button_to_io();
    LCD_EN=1;
    DCDC_nSHDN=1;
    clr_pot_sd();
    SSD13003_SetBrightness((uint8_t)lcd_brightness);
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
    SYSKEY = 0x00000000;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    OSCCONbits.NOSC = 0b011;
    OSCCONbits.OSWEN = 1;
    while (OSCCONbits.OSWEN == 1);
    while (OSCCONbits.CLKLOCK == 0);
    SYSKEY = 0x00000000;
}

void sw_to_sosc(void)
{
    SYSKEY = 0x00000000;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    OSCCONbits.NOSC = 0b100;
    OSCCONbits.OSWEN = 1;
    while (OSCCONbits.OSWEN == 1);
    SYSKEY = 0x00000000;
}

void prt_stt(void)
{
    report_system_status(true);
}

uint8_t find_first_active_table(void)
{
    for (int i = 0; i < MAX_NUM_TABLES; i++) {
        if (active_tables & (1 << i)) {
            return (i+1);
        }
    }
    return 0;
}

uint8_t find_next_active_table(void)
{
    if (table_num<MAX_NUM_TABLES)
        for (int i = table_num; i < MAX_NUM_TABLES; i++)
        {
            if (active_tables & (1 << i)) {
                return (i+1);
            }
        }
    for (int i=0;i<table_num-1;i++)
    {
        if (active_tables & (1 << i)) {
            return (i+1);
        }
    }
    return 0;
}

void enterSleepMode(void)
{
    SYSKEY = 0x0;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    OSCCONbits.SLPEN = 1;
    SYSKEY = 0x0;
    asm volatile("wait");
}

uint16_t interpolateRange(float targetAngle)
{
    if (targetAngle<table_data[0].angle)
        return 0;
    for (uint8_t i = 0; i < (TABLE_SIZE - 1U); i++) {
        float angle1 = table_data[i].angle;
        float angle2 = table_data[i + 1].angle;
        if ((targetAngle >= angle1 && targetAngle <= angle2) && (angle2<=300))
        {
            float range1 = (float)table_data[i].range;
            float range2 = (float)table_data[i + 1].range;
            float interpolatedRange = range1 + (targetAngle - angle1) * (range2 - range1) / (angle2 - angle1);
            return ((uint16_t)interpolatedRange);
        }
    }
    return 0xFFFF;
}

void buttons_isr(void)
{
    if (handle_both_buttons())
        return;
    handle_button_one();
    handle_button_two();
}

void update_angle(int16_t new_angle)
{
    uint8_t rollnow;

    if (LIS_read_register(WHO_AM_I)!=0x33)
    {
        clear_roll();
        return;
    }
    rollnow=determine_roll_state((int16_t)new_angle);
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
    char cur_text[TABLE_NAME_TEXT_LEN];

    table_num=find_next_active_table();
    sprintf(cur_text,"%s",table_names[table_num-1]);
    write_str_LCD(TABLE_START_W,TABLE_START_H,cur_text);
    write_byte_eerpom(LAST_TABLE,table_num);
    load_ballistic_table_data(table_num);
}

float load_cal_angle(void)
{
    uint32_t stored_angle = read_u32_from_eeprom(CAL_VALUE);

    if (stored_angle==0xFFFFFFFF)
        return 0;

    return decode_stored_angle(stored_angle);
}

void init_ballistic_table(void)
{
    table_num=read_byte_eerpom(LAST_TABLE);
    if (table_num>MAX_NUM_TABLES)
        table_num=1;
    load_ballistic_table_data(table_num);
}

void clear_screen_buffer(void)
{
    uint8_t j,i;

    for (i=0;i<=63;i++)
        for (j=0;j<=5;j++)
            screen_buffer[i][j]=0;
}

void set_display_drawings(void)
{
    draw_rect(ROLL_START_W,ROLL_START_H,ROLL_WIDTH,ROLL_HEIGHT);
    draw_rect(ROLL_START_W+ROLL_WIDTH/2,ROLL_START_H-2,1,2);
    draw_roll(-7);
}

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

void draw_roll(int16_t x)
{
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
        blink_roll=2;
    else if (roll_state==BLINK_LEFT)
        blink_roll=1;
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

    return ROLL_M5_5;
}
