#include "app_comm.h"

#include "app_actions.h"
#include "app_ballistics.h"
#include "app_display.h"
#include "app_power.h"
#include "app_runtime.h"
#include "app_system.h"

typedef char (*AppCommandHandler)(const uint8_t *payload, uint8_t payload_size);

typedef struct {
    uint8_t opcode;
    AppCommandHandler handler;
    bool debug_only;
} AppCommandDescriptor;

static bool comm_queue_push(const uint8_t *frame, uint8_t frame_size)
{
    if ((frame == NULL) || (frame_size == 0U) || (frame_size > COMM_FRAME_MAX_LEN))
        return false;

    if (comm_queue_count >= COMM_QUEUE_DEPTH)
    {
        comm_queue_overflow = true;
        return false;
    }

    memcpy(comm_queue[comm_queue_tail], frame, frame_size);
    comm_queue_sizes[comm_queue_tail] = frame_size;
    comm_queue_tail = (uint8_t)((comm_queue_tail + 1U) % COMM_QUEUE_DEPTH);
    comm_queue_count++;
    return true;
}

static bool comm_queue_pop(uint8_t *frame_out, uint8_t *frame_size)
{
    if ((frame_out == NULL) || (frame_size == NULL) || (comm_queue_count == 0U))
        return false;

    *frame_size = comm_queue_sizes[comm_queue_head];
    memcpy(frame_out, comm_queue[comm_queue_head], *frame_size);
    comm_queue_head = (uint8_t)((comm_queue_head + 1U) % COMM_QUEUE_DEPTH);
    comm_queue_count--;
    return true;
}

static void enqueue_pending_rx_frame(void)
{
    if (!CommAv)
        return;

    if (!comm_queue_push((const uint8_t *)PCComm, (uint8_t)PCComm[1]))
        comm_queue_overflow = true;

    CommAv = 0;
    CommPointer = 0;
    CommStart = 0;
    MsgSize = 0xFF;
    app_register_activity();
}

static float convert_raw_angle_to_degrees(uint16_t raw_angle_value)
{
    return (((float)raw_angle_value) / RAW_ANGLE_MAX_COUNT) * RAW_ANGLE_FULL_SCALE_DEG;
}

static void print_all_table_names(bool include_index)
{
    uint8_t table_index;

    for (table_index = 0; table_index < MAX_NUM_TABLES; table_index++)
    {
        if (include_index)
            sprintf(PCDebug, "\ttable %u name:%s\n", table_index + 1U, table_names[table_index]);
        else
            sprintf(PCDebug, "%s\n", table_names[table_index]);
        send_string_UART2(PCDebug);
    }
}

static void print_loaded_table(void)
{
    uint16_t row_index;

    for (row_index = 0; row_index < loaded_table_rows; row_index++)
    {
        sprintf(PCDebug, "%u,%.6f\n", table_data[row_index].range, table_data[row_index].angle);
        send_string_UART2(PCDebug);
    }
}

static void debug_test_eeprom(void)
{
    uint8_t msb = read_byte_eerpom(EEPROM_TEST_ADDRESS);

    sprintf(PCDebug, "\tRead from address 0x0102:%u\n", 0x00FF & msb);
    send_string_UART2(PCDebug);
    send_string_UART2("\tWriting previous val + 5...\n");
    write_byte_eerpom(EEPROM_TEST_ADDRESS, msb + 5U);
    msb = read_byte_eerpom(EEPROM_TEST_ADDRESS);
    sprintf(PCDebug, "\tRead from address 0x0102:%u\n", 0x00FF & msb);
    send_string_UART2(PCDebug);
}

static void debug_send_angle_samples(void)
{
    uint8_t sample_index;

    send_string_UART2("\n");
    for (sample_index = 0; sample_index < DEBUG_SAMPLE_COUNT; sample_index++)
    {
        cur_ang = read_angle(50);
        sprintf(PCDebug, "%.4f\n", cur_ang);
        send_string_UART2(PCDebug);
        __delay_ms(DEBUG_SAMPLE_DELAY_MS);
    }
}

static void debug_encoder_readback(void)
{
    uint16_t raw_angle_value;
    float angle_deg;

    raw_angle_value = read_one_angle();
    angle_deg = convert_raw_angle_to_degrees(raw_angle_value);
    sprintf(PCDebug, "%u ; %.4f\n", raw_angle_value, angle_deg);
    send_string_UART2(PCDebug);

    raw_angle_value = read_one_angle_look_for_error();
    angle_deg = convert_raw_angle_to_degrees(raw_angle_value);
    sprintf(PCDebug, "%u ; %.4f\n", raw_angle_value, angle_deg);
    send_string_UART2(PCDebug);
}

static bool validate_payload_size(uint8_t payload_size, uint8_t expected_min_size)
{
    return payload_size >= expected_min_size;
}

static char reject_invalid_payload(void)
{
    send_ack_to_PC(COMM_CRC_ERROR_OPCODE);
    return 0;
}

static char handle_ack_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    send_ack_to_PC(0x15);
    return 0;
}

static char handle_ping_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    send_string_UART2("PING\n");
    return 0;
}

static char handle_write_table_row_debug_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload_size;
    write_table_row_debug((char *)payload);
    return 0;
}

static char handle_eeprom_debug_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    debug_test_eeprom();
    return 0;
}

static char handle_send_table_command(const uint8_t *payload, uint8_t payload_size)
{
    if (!validate_payload_size(payload_size, 1U))
        return reject_invalid_payload();

    send_table_to_pc(payload[0]);
    return 0;
}

static char handle_send_table_monitor_command(const uint8_t *payload, uint8_t payload_size)
{
    if (!validate_payload_size(payload_size, 1U))
        return reject_invalid_payload();

    send_table_to_pc_monitor(payload[0]);
    return 0;
}

static char handle_write_table_row_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload_size;
    write_table_to_row((char *)payload);
    send_ack_to_PC(0x21);
    return 0;
}

static char handle_send_info_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    send_info_back();
    return 0;
}

static char handle_write_table_rows_count_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload_size;
    write_table_rows_count((char *)payload);
    send_ack_to_PC(0x23);
    return 0;
}

static char handle_write_table_rows_count_debug_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload_size;
    write_table_rows_count_debug((char *)payload);
    return 0;
}

static char handle_read_table_rows_count_debug_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    read_table_rows_count_debug();
    return 0;
}

static char handle_read_table_rows_count_command(const uint8_t *payload, uint8_t payload_size)
{
    if (!validate_payload_size(payload_size, 1U))
        return reject_invalid_payload();

    read_table_rows_count(payload[0]);
    return 0;
}

static char handle_print_loaded_table_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    print_loaded_table();
    return 0;
}

static char handle_angle_to_range_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    sprintf(PCDebug, "\nangle: %.6f -> range: %u\n", cur_ang, interpolateRange(cur_ang));
    send_string_UART2(PCDebug);
    return 0;
}

static char handle_update_active_tables_command(const uint8_t *payload, uint8_t payload_size)
{
    if (!validate_payload_size(payload_size, 1U))
        return reject_invalid_payload();

    app_action_update_active_tables(payload[0]);
    send_ack_to_PC(0x29);
    return 0;
}

static char handle_print_active_tables_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    sprintf(PCDebug, "\n active tables : %u\n", active_tables);
    send_string_UART2(PCDebug);
    return 0;
}

static char handle_write_table_name_command(const uint8_t *payload, uint8_t payload_size)
{
    if (!validate_payload_size(payload_size, TABLE_NAME_TEXT_LEN))
        return reject_invalid_payload();

    app_action_write_table_name(payload[0], (const char *)&payload[1]);
    send_ack_to_PC(0x31);
    return 0;
}

static char handle_print_table_names_indexed_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    print_all_table_names(true);
    return 0;
}

static char handle_sleep_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    goto_sleep();
    return 0;
}

static char handle_toggle_lcd_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    send_string_UART2("\nToggling LCD enable\n");
    LCD_EN = !LCD_EN;
    return 0;
}

static char handle_toggle_dcdc_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    DCDC_nSHDN = !DCDC_nSHDN;
    sprintf(PCDebug, "\nnSHDN:%u\n", DCDC_nSHDN);
    send_string_UART2(PCDebug);
    return 0;
}

static char handle_reset_pot_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    send_string_UART2("\nreseting and setting pot to mid range\n");
    pot_reset();
    pot_set_resistenace(0x7F);
    return 0;
}

static char handle_print_brightness_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    sprintf(PCDebug, "\nbrightness:%u\n", lcd_brightness);
    send_string_UART2(PCDebug);
    return 0;
}

static char handle_verbose_status_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    prt_stt();
    return 0;
}

static char handle_print_table_names_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    print_all_table_names(false);
    return 0;
}

static char handle_calibrate_angle_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    app_action_calibrate_zero();
    send_ack_to_PC(0x40);
    return 0;
}

static char handle_print_zero_angle_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    sprintf(PCDebug, "\n zero angle: %.4f\n", zero_angle);
    send_string_UART2(PCDebug);
    return 0;
}

static char handle_angle_samples_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    debug_send_angle_samples();
    return 0;
}

static char handle_enable_sleep_mode_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    app_action_set_sleep_enabled(true);
    send_ack_to_PC(0x43);
    return 0;
}

static char handle_disable_sleep_mode_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    app_action_set_sleep_enabled(false);
    send_ack_to_PC(0x44);
    return 0;
}

static char handle_kick_spi_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    send_string_UART2("kick spi\n");
    SPI2_Initialize();
    as5048a_spi_init_seq();
    return 0;
}

static char handle_display_test_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    send_string_UART2("sending text to display\n");
    write_str_LCD_large_thick_font(RANGE_START_W, RANGE_START_H, "RRR");
    __delay_ms(2000);
    return 0;
}

static char handle_battery_test_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    send_string_UART2("bat debug\n");
    set_battery_display_vertical(45);
    __delay_ms(2000);
    return 0;
}

static char handle_encoder_debug_command(const uint8_t *payload, uint8_t payload_size)
{
    (void)payload;
    (void)payload_size;
    debug_encoder_readback();
    return 0;
}

static const AppCommandDescriptor app_command_table[] = {
    { 0x15U, handle_ack_command, false },
    { 0x16U, handle_ping_command, false },
    { 0x17U, handle_write_table_row_debug_command, true },
    { 0x18U, handle_eeprom_debug_command, true },
    { 0x19U, handle_send_table_command, false },
    { 0x20U, handle_send_table_monitor_command, false },
    { 0x21U, handle_write_table_row_command, false },
    { 0x22U, handle_send_info_command, false },
    { 0x23U, handle_write_table_rows_count_command, false },
    { 0x24U, handle_write_table_rows_count_debug_command, true },
    { 0x25U, handle_read_table_rows_count_debug_command, true },
    { 0x26U, handle_read_table_rows_count_command, false },
    { 0x27U, handle_print_loaded_table_command, true },
    { 0x28U, handle_angle_to_range_command, false },
    { 0x29U, handle_update_active_tables_command, false },
    { 0x30U, handle_print_active_tables_command, true },
    { 0x31U, handle_write_table_name_command, false },
    { 0x32U, handle_print_table_names_indexed_command, true },
    { 0x33U, handle_sleep_command, false },
    { 0x34U, handle_toggle_lcd_command, true },
    { 0x35U, handle_toggle_dcdc_command, true },
    { 0x36U, handle_reset_pot_command, true },
    { 0x37U, handle_print_brightness_command, true },
    { 0x38U, handle_verbose_status_command, true },
    { 0x39U, handle_print_table_names_command, true },
    { 0x40U, handle_calibrate_angle_command, false },
    { 0x41U, handle_print_zero_angle_command, true },
    { 0x42U, handle_angle_samples_command, true },
    { 0x43U, handle_enable_sleep_mode_command, false },
    { 0x44U, handle_disable_sleep_mode_command, false },
    { 0x45U, handle_kick_spi_command, true },
    { 0x46U, handle_display_test_command, true },
    { 0x47U, handle_battery_test_command, true },
    { 0x48U, handle_encoder_debug_command, true }
};

static const AppCommandDescriptor *find_command_descriptor(uint8_t opcode)
{
    size_t command_index;

    for (command_index = 0U; command_index < (sizeof(app_command_table) / sizeof(app_command_table[0])); command_index++)
    {
        if (app_command_table[command_index].opcode == opcode)
            return &app_command_table[command_index];
    }

    return NULL;
}

static bool decode_frame(
    const uint8_t *frame,
    uint8_t frame_size,
    uint8_t *opcode,
    const uint8_t **payload,
    uint8_t *payload_size)
{
    if ((frame == NULL) || (opcode == NULL) || (payload == NULL) || (payload_size == NULL) || (frame_size < 3U))
        return false;

    if (frame[1] != frame_size)
        return false;

    *opcode = frame[0];
    *payload = &frame[2];
    *payload_size = (uint8_t)(frame_size - 3U);

    return CheckCRC((char)*opcode, *payload_size, (char *)*payload) == 0;
}

static char handle_frame(const uint8_t *frame, uint8_t frame_size)
{
    const AppCommandDescriptor *descriptor;
    const uint8_t *payload;
    uint8_t opcode;
    uint8_t payload_size;

    if (!decode_frame(frame, frame_size, &opcode, &payload, &payload_size))
    {
        send_ack_to_PC(COMM_CRC_ERROR_OPCODE);
        return 0;
    }

    descriptor = find_command_descriptor(opcode);
    if (descriptor == NULL)
        return 0;

#ifdef OPERATIONAL
    if (descriptor->debug_only)
    {
        send_ack_to_PC(COMM_CRC_ERROR_OPCODE);
        return 0;
    }
#endif

    return descriptor->handler(payload, payload_size);
}

void app_comm_process(void)
{
    uint8_t frame[COMM_FRAME_MAX_LEN];
    uint8_t frame_size = 0U;

    if (U2STAbits.OERR == 1)
        U2STAbits.OERR = 0b0;

    enqueue_pending_rx_frame();

    if (comm_queue_overflow)
    {
        send_ack_to_PC(COMM_CRC_ERROR_OPCODE);
        comm_queue_overflow = false;
    }

    if (comm_queue_pop(frame, &frame_size))
        (void)handle_frame(frame, frame_size);
}

void comm_isr(void)
{
    app_comm_process();
}

void send_info_back(void)
{
    uint8_t msg_buffer[6];
    uint8_t crc;

    msg_buffer[0] = 0x22;
    msg_buffer[1] = 0x05;
    msg_buffer[2] = FW_VER;
    msg_buffer[3] = LIS_read_register(WHO_AM_I);
    msg_buffer[4] = read_M95128_ID();
    crc = CalcCRC(5, (char *)msg_buffer);
    msg_buffer[5] = crc;

    for (crc = 0; crc <= 5U; crc++)
        send_byte_UART2(msg_buffer[crc]);
}

void send_ack_to_PC(uint8_t cur_opcode)
{
    uint8_t msg_buffer[3];
    uint8_t crc;

    msg_buffer[0] = cur_opcode;
    msg_buffer[1] = 0x03;
    crc = CalcCRC(2, (char *)msg_buffer);
    msg_buffer[2] = crc;

    for (crc = 0; crc <= 2U; crc++)
        send_byte_UART2(msg_buffer[crc]);
}

char CalcCRC(unsigned char tx_msg_size, char *msg_out)
{
    char csr = 0;
    unsigned char index;

    for (index = 1; index <= tx_msg_size; index++)
        csr ^= *msg_out++;

    return csr;
}

char CheckCRC(char opcode, unsigned char rx_msg_size, char *check_comm)
{
    char csr = opcode ^ (rx_msg_size + 3);
    unsigned char index;

    for (index = 1; index <= rx_msg_size; index++)
        csr ^= *check_comm++;

    return (csr != *check_comm);
}
