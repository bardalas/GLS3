#ifndef APP_BALLISTICS_H
#define APP_BALLISTICS_H

#include "app_state.h"

void load_table(void);
void init_ballistic_table(void);
uint16_t interpolateRange(float targetAngle);
void update_range(void);
uint16_t roundToNearest5(uint16_t value);
float load_cal_angle(void);

#endif
