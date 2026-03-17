#include "app_comm.h"

static float convert_raw_angle_to_degrees(uint16_t raw_angle_value)
{
    return (((float)raw_angle_value) / RAW_ANGLE_MAX_COUNT) * RAW_ANGLE_FULL_SCALE_DEG;
}

static void set_sleep_mode_enabled(uint8_t enabled)
{
    activate_sleep = enabled;
    write_byte_eerpom(ACT_SLP, enabled);
    if (enabled)
        sleep_timer = 56000;
}

static void update_active_tables(uint8_t new_active_tables)
{
    active_tables = new_active_tables;
    write_byte_eerpom(ACTIVE_TAB, active_tables);
    table_num = find_first_active_table();
    if ((table_num == 0U) || (table_num > MAX_NUM_TABLES))
        table_num = 1U;
    write_byte_eerpom(LAST_TABLE, table_num);
    init_ballistic_table();
    write_str_LCD(TABLE_START_W, TABLE_START_H, table_names[table_num - 1U]);
}

static void print_all_table_names(bool include_index)
{
    uint8_t help;

    for (help = 0; help < MAX_NUM_TABLES; help++)
    {
        if (include_index)
            sprintf(PCDebug,"\ttable %u name:%s\n",help+1,table_names[help]);
        else
            sprintf(PCDebug,"%s\n",table_names[help]);
        send_string_UART2(PCDebug);
    }
}

static void print_loaded_table(void)
{
    uint16_t sel_add;

    for (sel_add = 0; sel_add < TABLE_SIZE; sel_add++)
    {
        sprintf(PCDebug,"%u,%.6f\n", table_data[sel_add].range, table_data[sel_add].angle);
        send_string_UART2(PCDebug);
    }
}

static void debug_test_eeprom(void)
{
    uint8_t msb = read_byte_eerpom(EEPROM_TEST_ADDRESS);

    sprintf(PCDebug,"\tRead from address 0x0102:%u\n",0x00FF&msb);send_string_UART2(PCDebug);
    send_string_UART2("\tWriting previous val + 5...\n");
    write_byte_eerpom(EEPROM_TEST_ADDRESS, msb + 5U);
    msb = read_byte_eerpom(EEPROM_TEST_ADDRESS);
    sprintf(PCDebug,"\tRead from address 0x0102:%u\n",0x00FF&msb);send_string_UART2(PCDebug);
}

static void debug_send_angle_samples(void)
{
    uint8_t help;

    send_string_UART2("\n");
    for (help = 0; help < DEBUG_SAMPLE_COUNT; help++)
    {
        cur_ang = read_angle(50);
        sprintf(PCDebug,"%.4f\n",cur_ang);send_string_UART2(PCDebug);
        __delay_ms(DEBUG_SAMPLE_DELAY_MS);
    }
}

static void debug_encoder_readback(void)
{
    uint16_t anghelp;
    float angle_deg_help;

    anghelp = read_one_angle();
    angle_deg_help = convert_raw_angle_to_degrees(anghelp);
    sprintf(PCDebug,"%u ; %.4f\n",anghelp,angle_deg_help);send_string_UART2(PCDebug);
    anghelp = read_one_angle_look_for_error();
    angle_deg_help = convert_raw_angle_to_degrees(anghelp);
    sprintf(PCDebug,"%u ; %.4f\n",anghelp,angle_deg_help);send_string_UART2(PCDebug);
}

static void handle_table_name_write(char *comm)
{
    uint8_t sel_table = (uint8_t)*comm++;
    uint8_t help;

    if ((sel_table == 0U) || (sel_table > MAX_NUM_TABLES))
        return;

    for (help = 0; help <= 3; help++)
    {
        table_names[sel_table-1][help] = *comm;
        write_byte_eerpom((uint16_t)TAB1N + (uint16_t)(4 * (sel_table - 1U)) + (uint16_t)help, *comm++);
    }
    table_names[sel_table-1][TABLE_NAME_TEXT_LEN - 1U] = '\0';

    if (sel_table == table_num)
        write_str_LCD(TABLE_START_W, TABLE_START_H, table_names[sel_table - 1U]);
}

static char process_comm_opcode(uint8_t opcode, char *comm)
{
    switch (opcode)
    {
        case 0x15:
            send_ack_to_PC(opcode);
            break;
        case 0x16:
            send_string_UART2("PING\n");
            break;
        case 0x17:
            write_table_row_debug(comm);
            break;
        case 0x18:
            debug_test_eeprom();
            break;
        case 0x19:
            send_table_to_pc((uint8_t)*comm);
            break;
        case 0x20:
            send_table_to_pc_monitor((uint8_t)*comm);
            break;
        case 0x21:
            write_table_to_row(comm);
            send_ack_to_PC(opcode);
            break;
        case 0x22:
            send_info_back();
            break;
        case 0x23:
            write_table_rows_count(comm);
            send_ack_to_PC(opcode);
            break;
        case 0x24:
            write_table_rows_count_debug(comm);
            break;
        case 0x25:
            read_table_rows_count_debug();
            break;
        case 0x26:
            read_table_rows_count((uint8_t)*comm);
            break;
        case 0x27:
            print_loaded_table();
            break;
        case 0x28:
            sprintf(PCDebug,"\nangle: %.6f -> range: %u\n",cur_ang,interpolateRange(cur_ang));
            send_string_UART2(PCDebug);
            break;
        case 0x29:
            update_active_tables((uint8_t)*comm);
            send_ack_to_PC(opcode);
            break;
        case 0x30:
            sprintf(PCDebug,"\n active tables : %u\n",active_tables); send_string_UART2(PCDebug);
            break;
        case 0x31:
            handle_table_name_write(comm);
            send_ack_to_PC(opcode);
            break;
        case 0x32:
            print_all_table_names(true);
            break;
        case 0x33:
            goto_sleep();
            break;
        case 0x34:
            send_string_UART2("\nToggling LCD enable\n");
            LCD_EN = !LCD_EN;
            break;
        case 0x35:
            DCDC_nSHDN = !DCDC_nSHDN;
            sprintf(PCDebug,"\nnSHDN:%u\n",DCDC_nSHDN);
            send_string_UART2(PCDebug);
            break;
        case 0x36:
            send_string_UART2("\nreseting and setting pot to mid range\n");
            pot_reset();
            pot_set_resistenace(0x7F);
            break;
        case 0x37:
            sprintf(PCDebug,"\nbrightness:%u\n",lcd_brightness);send_string_UART2(PCDebug);
            break;
        case 0x38:
            prt_stt();
            break;
        case 0x39:
            print_all_table_names(false);
            break;
        case 0x40:
            calibrate_angle();
            send_ack_to_PC(opcode);
            break;
        case 0x41:
            sprintf(PCDebug,"\n zero angle: %.4f\n",zero_angle);send_string_UART2(PCDebug);
            break;
        case 0x42:
            debug_send_angle_samples();
            break;
        case 0x43:
            set_sleep_mode_enabled(1);
            send_ack_to_PC(opcode);
            break;
        case 0x44:
            set_sleep_mode_enabled(0);
            send_ack_to_PC(opcode);
            break;
        case 0x45:
            send_string_UART2("kick spi\n");
            SPI2_Initialize();
            as5048a_spi_init_seq();
            break;
        case 0x46:
            send_string_UART2("sending text to display\n");
            write_str_LCD_large_thick_font(RANGE_START_W,RANGE_START_H,"RRR");
            __delay_ms(2000);
            break;
        case 0x47:
            send_string_UART2("bat debug\n");
            set_battery_display_vertical(45);
            __delay_ms(2000);
            break;
        case 0x48:
            debug_encoder_readback();
            break;
        default:
            break;
    }

    return 0;
}

void comm_isr(void)
{
    if (U2STAbits.OERR == 1)
        U2STAbits.OERR = 0b0;

    if (CommAv)
    {
        sleep_timer = 0;
        handle_comm(PCComm);
        CommAv = 0;
        CommPointer = 0;
        CommStart = 0;
    }
}

char handle_comm(char *Comm)
{
    unsigned char RxMsgSize, OPCODE;

    OPCODE = *Comm++;
    RxMsgSize = (unsigned char)*Comm++ - 3;

    if (CheckCRC(OPCODE, RxMsgSize, Comm))
    {
        send_ack_to_PC(COMM_CRC_ERROR_OPCODE);
        return 0;
    }

    return process_comm_opcode(OPCODE, Comm);
}

void send_info_back(void)
{
    uint8_t msg_buffer[6],CRC;

    msg_buffer[0]=0x22;
    msg_buffer[1]=0x05;
    msg_buffer[2]=FW_VER;
    msg_buffer[3]=LIS_read_register(WHO_AM_I);
    msg_buffer[4]=read_M95128_ID();
    CRC=CalcCRC(5,(char *)msg_buffer);
    msg_buffer[5]=CRC;
    for (CRC=0;CRC<=5;CRC++)
        send_byte_UART2(msg_buffer[CRC]);
}

void send_ack_to_PC(uint8_t cur_opcode)
{
    uint8_t msg_buffer[3],CRC;

    msg_buffer[0]=cur_opcode;
    msg_buffer[1]=0x03;
    CRC=CalcCRC(2,(char *)msg_buffer);
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

    return(CSR!=*CheckComm);
}
