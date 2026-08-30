#ifndef		__PEOCOTOL_H
#define		__PEOCOTOL_H

#include "config.h"

/* ---- 协议帧格式 ---- */
#define HEAD1   0xAA
#define HEAD2   0x55
#define MAX_DATA    10

/* ---- 命令定义 ---- */
#define CMD_GET_DATA    0x01
#define CMD_SET_LED     0x02
#define CMD_RESET_HUB   0x03
#define CMD_GET_INFO    0x04
#define CMD_ECHO        0x05
#define CMD_ENTER_IAP   0x10

/* ---- 状态码 ---- */
#define STS_OK      0x00
#define STS_ERR     0x01

/* ---- 函数声明 ---- */
unsigned char parse_packet(unsigned char *cmd, unsigned char *buf, unsigned char *len);
void process_cmd(unsigned char cmd, unsigned char *buf, unsigned char len);
void protocol_process(void);

#endif