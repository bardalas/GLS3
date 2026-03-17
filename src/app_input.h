#ifndef APP_INPUT_H
#define APP_INPUT_H

#include "app_state.h"

void app_input_poll(void);
void app_input_reset(void);
void buttons_isr(void);

#endif
