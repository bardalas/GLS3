#ifndef APP_COMM_H
#define APP_COMM_H

#include "app_state.h"

void app_comm_process(void);
void comm_isr(void);
char CalcCRC(unsigned char tx_msg_size, char *msg_out);
char CheckCRC(char opcode, unsigned char rx_msg_size, char *check_comm);
void send_ack_to_PC(uint8_t cur_opcode);
void send_info_back(void);

#endif
