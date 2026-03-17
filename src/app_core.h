#ifndef APP_CORE_H
#define APP_CORE_H

#define OPERATIONAL
#define FW_VER 2
//#define FORRONEN
//#define SLEEP_ACT

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "definitions.h"
#include "defs.h"
#include "drivers.h"
#include "config/default/M95128.h"
#include "LIS3DHTR.h"
#include "AS5048A.h"
#include "config/default/MAX5394.h"

#define TABLE_NAME_TEXT_LEN            5U
#define BATTERY_PERCENT_MIN_VOLTAGE    2.4f
#define BATTERY_PERCENT_RANGE_VOLTAGE  0.6f
#define BATTERY_SEGMENT_EMPTY_MAX      4U
#define BATTERY_SEGMENT_LOW_MAX        24U
#define BATTERY_SEGMENT_MID_MAX        49U
#define BATTERY_SEGMENT_HIGH_MAX       74U
#define BATTERY_LEVEL_EMPTY            1U
#define BATTERY_LEVEL_25               2U
#define BATTERY_LEVEL_50               3U
#define BATTERY_LEVEL_75               4U
#define BATTERY_LEVEL_FULL             5U
#define BRIGHTNESS_STEP                50
#define BRIGHTNESS_MAX                 250
#define ANGLE_STORAGE_SCALE            10000000.0f
#define BALLISTIC_TABLE_ENTRY_SIZE     6U
#define BALLISTIC_TABLE_BYTES          800U
#define BALLISTIC_TABLE_START_PAGE     1U
#define BALLISTIC_TABLE_PAGES_PER_TAB  5U
#define COMM_CRC_ERROR_OPCODE          0x55U
#define EEPROM_TEST_ADDRESS            0x0102U
#define RAW_ANGLE_MAX_COUNT            16383.0f
#define RAW_ANGLE_FULL_SCALE_DEG       360.0f
#define DEBUG_SAMPLE_COUNT             100U
#define DEBUG_SAMPLE_DELAY_MS          10U

typedef struct {
    uint16_t range;
    float angle;
} RangeAngle;

#endif
