#ifndef APP_COMM_H
#define APP_COMM_H

#include "app_input.h"

char handle_comm(char *Comm);
void comm_isr(void);
char CalcCRC(unsigned char tx_msg_size, char *msg_out);
char CheckCRC(char opcode, unsigned char rx_msg_size, char *check_comm);
void send_ack_to_PC(uint8_t cur_opcode);
void send_info_back(void);

#endif
