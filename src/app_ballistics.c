#include "app_ballistics.h"

uint8_t find_first_active_table(void);

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
    return (uint16_t)(256U * (BALLISTIC_TABLE_START_PAGE + (BALLISTIC_TABLE_PAGES_PER_TAB * ((uint16_t)selected_table - 1U))));
}

static void load_ballistic_table_data(uint8_t selected_table)
{
    uint16_t table_address = get_ballistic_table_base_address(selected_table);
    uint16_t row_ct = 0;
    uint16_t add_ct;
    float previous_angle = -1.0f;

    loaded_table_rows = 0U;
    ballistic_table_valid = false;

    for (add_ct = table_address; add_ct < (uint16_t)(table_address + BALLISTIC_TABLE_BYTES); add_ct += BALLISTIC_TABLE_ENTRY_SIZE)
    {
        uint16_t range = (uint16_t)((uint16_t)read_byte_eerpom(add_ct) * 256U + read_byte_eerpom(add_ct + 1U));
        float angle = decode_stored_angle(read_u32_from_eeprom((uint16_t)(add_ct + 2U)));

        if ((!isfinite(angle)) || (angle < 0.0f) || (angle > 300.0f) || ((row_ct > 0U) && (angle <= previous_angle)))
            break;

        table_data[row_ct].range = range;
        table_data[row_ct].angle = angle;
        previous_angle = angle;
        row_ct++;
    }

    loaded_table_rows = row_ct;
    ballistic_table_valid = (loaded_table_rows >= 2U);
}

uint16_t roundToNearest5(uint16_t value)
{
    return (uint16_t)(((value + 2U) / 5U) * 5U);
}

uint16_t interpolateRange(float targetAngle)
{
    uint8_t i;

    if (!ballistic_table_valid)
        return 0xFFFFU;

    if (targetAngle < table_data[0].angle)
        return 0U;

    if (targetAngle > table_data[loaded_table_rows - 1U].angle)
        return 0xFFFFU;

    for (i = 0; i < (loaded_table_rows - 1U); i++)
    {
        float angle1 = table_data[i].angle;
        float angle2 = table_data[i + 1U].angle;

        if ((targetAngle >= angle1) && (targetAngle <= angle2) && (angle2 > angle1))
        {
            float range1 = (float)table_data[i].range;
            float range2 = (float)table_data[i + 1U].range;
            float interpolated_range = range1 + (targetAngle - angle1) * (range2 - range1) / (angle2 - angle1);
            return (uint16_t)interpolated_range;
        }
    }

    return 0xFFFFU;
}

void update_range(void)
{
    static char last_range_text[5] = "";
    char cur_text[5];
    uint16_t cur_range = interpolateRange(cur_ang);

    if ((cur_range != 0U) && (cur_range != 0xFFFFU))
        cur_range = roundToNearest5(cur_range);

    if (cur_range == 0U)
        sprintf(cur_text, "LOW ");
    else if (cur_range == 0xFFFFU)
        sprintf(cur_text, " HI ");
    else if (cur_range < 100U)
        sprintf(cur_text, " %um", cur_range);
    else
        sprintf(cur_text, "%um", cur_range);

    if (strcmp(last_range_text, cur_text) != 0)
    {
        strcpy(last_range_text, cur_text);
        write_str_LCD_large_font(RANGE_START_W, RANGE_START_H, cur_text);
    }
}

float load_cal_angle(void)
{
    uint32_t stored_angle = read_u32_from_eeprom(CAL_VALUE);

    if (stored_angle == 0xFFFFFFFFUL)
        return 0.0f;

    return decode_stored_angle(stored_angle);
}

void init_ballistic_table(void)
{
    uint8_t stored_table = read_byte_eerpom(LAST_TABLE);

    table_num = stored_table;
    if ((table_num == 0U) || (table_num > MAX_NUM_TABLES) || ((active_tables != 0U) && ((active_tables & (1U << (table_num - 1U))) == 0U)))
        table_num = find_first_active_table();
    if ((table_num == 0U) || (table_num > MAX_NUM_TABLES))
        table_num = 1U;

    write_byte_eerpom(LAST_TABLE, table_num);
    load_ballistic_table_data(table_num);
}

void load_table(void)
{
    char cur_text[TABLE_NAME_TEXT_LEN];

    table_num = find_next_active_table();
    if ((table_num == 0U) || (table_num > MAX_NUM_TABLES))
        table_num = find_first_active_table();
    if ((table_num == 0U) || (table_num > MAX_NUM_TABLES))
        table_num = 1U;

    sprintf(cur_text, "%s", table_names[table_num - 1U]);
    write_str_LCD(TABLE_START_W, TABLE_START_H, cur_text);
    write_byte_eerpom(LAST_TABLE, table_num);
    load_ballistic_table_data(table_num);
}

bool is_ballistic_table_ready(void)
{
    return ballistic_table_valid;
}
