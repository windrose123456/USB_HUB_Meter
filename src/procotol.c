#include "procotol.h"
#include "STC8G_H_UART.h"

/* ========== 协议帧解析 ========== */

/*
 * 从 RX1_Buffer 解析一帧数据 (状态机, 每次调用消费已接收字节)
 * 返回 1=收到有效帧, 0=无数据或未完成
 */
unsigned char parse_packet(unsigned char *cmd, unsigned char *buf, unsigned char *len)
{
    static unsigned char st = 0;
    static unsigned char s_cmd, s_len, s_idx, s_chk;
    static unsigned char buf_pos = 0;

    while (buf_pos < COM1.RX_Cnt) {
        unsigned char c = RX1_Buffer[buf_pos++];

        switch (st) {
        case 0: /* 等待 HEAD1 */
            if (c == HEAD1) st = 1;
            break;

        case 1: /* 等待 HEAD2 */
            if (c == HEAD2) st = 2;
            else st = 0;
            break;

        case 2: /* 读取 LEN */
            if (c > MAX_DATA) { st = 0; break; }
            s_len = c;
            s_chk = HEAD1 ^ HEAD2 ^ c;
            s_idx = 0;
            st = 3;
            break;

        case 3: /* 读取 CMD */
            s_cmd = c;
            s_chk ^= c;
            if (s_len == 0) st = 5;
            else st = 4;
            break;

        case 4: /* 读取 DATA */
            buf[s_idx++] = c;
            s_chk ^= c;
            if (s_idx >= s_len) st = 5;
            break;

        case 5: /* 读取 CHECKSUM */
            st = 0;
            if (c == s_chk) {
                /* 解析成功, 丢弃已消费的字节, 重置读位置 */
                COM1.RX_Cnt = 0;
                buf_pos = 0;
                *cmd = s_cmd;
                *len = s_len;
                return 1;
            }
            break;
        }
    }
    /* 已消费完缓冲区, 重置 */
    COM1.RX_Cnt = 0;
    buf_pos = 0;
    return 0;
}

/* 发送响应帧 */
void send_resp(unsigned char cmd, unsigned char sts,
               unsigned char *buf, unsigned char len)
{
    unsigned char i, chk;
    TX1_write2buff(HEAD1);
    TX1_write2buff(HEAD2);
    TX1_write2buff(len + 1);
    TX1_write2buff(cmd | 0x80);
    TX1_write2buff(sts);
    chk = HEAD1 ^ HEAD2 ^ (len + 1) ^ (cmd | 0x80) ^ sts;
    for (i = 0; i < len; i++) {
        TX1_write2buff(buf[i]);
        chk ^= buf[i];
    }
    TX1_write2buff(chk);
}

/* ========== 命令处理 ========== */
void process_cmd(unsigned char cmd,
                 unsigned char *buf, unsigned char len)
{
    unsigned char resp[10];

    switch (cmd) {

    case CMD_GET_DATA: {
        /* INA226 暂未接入, 返回占位数据 0xFF */
        unsigned char i;
        for (i = 0; i < 10; i++) resp[i] = 0xFF;
        send_resp(cmd, STS_OK, resp, 10);
        break;
    }

    case CMD_SET_LED: {
        /* LED 控制暂未接入 */
        send_resp(cmd, STS_ERR, 0, 0);
        break;
    }

    case CMD_RESET_HUB: {
        /* HUB 复位暂未接入 */
        send_resp(cmd, STS_OK, 0, 0);
        break;
    }

    case CMD_GET_INFO: {
        resp[0] = 0x01;            /* 固件版本 1.0 */
        resp[1] = 0x00;
        resp[2] = 0x08;            /* 8KB Flash */
        resp[3] = 0x00;            /* LED 状态 (暂未接入) */
        send_resp(cmd, STS_OK, resp, 4);
        break;
    }

    case CMD_ECHO: {
        /* 回显命令, 用于协议调试 */
        send_resp(cmd, STS_OK, buf, len);
        break;
    }

    case CMD_ENTER_IAP: {
        /* IAP 暂未接入, 只回复确认 */
        send_resp(cmd, STS_OK, 0, 0);
        break;
    }

    default:
        send_resp(cmd, STS_ERR, 0, 0);
        break;
    }
}

/* ========== 协议主处理 ========== */
void protocol_process(void)
{
    unsigned char cmd, buf[MAX_DATA], len;

    while (parse_packet(&cmd, buf, &len)) {
        process_cmd(cmd, buf, len);
    }
}
