/************************************************************
 * bootloader.c — STC8G1K08A IAP 引导程序
 * 编译目标地址: 0x1C00 (ISP/IAP 区域, 通过 STC-ISP 工具配置 1KB IAP)
 * 协议: 简化的字节协议 (无帧头, ACK/NAK)
 ************************************************************/
#include "STC8G.h"
#include <intrins.h>

/* ---- IAP 寄存器 ---- */
//sfr IAP_DATA  = 0xC2;
//sfr IAP_ADDRH = 0xC3;
//sfr IAP_ADDRL = 0xC4;
//sfr IAP_CMD   = 0xC5;
//sfr IAP_TRIG  = 0xC6;
//sfr IAP_CONTR = 0xC7;

#define IAP_IDLE    0
#define IAP_READ    1
#define IAP_WRITE   2
#define IAP_ERASE   3

/* ---- Flash 布局 ---- */
#define APP_ADDR    0x0000
#define APP_SIZE    0x1C00    /* 7KB 应用区 */
#define FLAG_ADDR   0x1F00    /* IAP 标志存储位置 */
#define IAP_FLAG    0xA5      /* 标志值 */

/* ---- Bootloader 命令 ---- */
#define BL_INFO     0x05
#define BL_ERASE    0x01
#define BL_WRITE    0x02
#define BL_REBOOT   0x04
#define BL_ACK      0x06
#define BL_NAK      0x15

#define FOSC 11059200UL
#define BAUD 115200UL

/* ---- 延时 ---- */
static void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 120; j++);
}

/* ---- UART (轮询模式) ---- */
static void uart_init(void) {
    SCON = 0x50;            /* Mode 1, REN=1 */
    AUXR |= 0x40;           /* Timer1 1T模式 */
    TMOD &= 0x0F;
    TH1 = (unsigned char)(256UL - (FOSC / 32UL / BAUD));
    TL1 = TH1;
    TR1 = 1;
}

static void uart_send(unsigned char c) {
    SBUF = c; while (!TI); TI = 0;
}

static unsigned char uart_recv(void) {
    while (!RI); RI = 0;
    return SBUF;
}

/* 带超时接收, 返回 0=超时 */
static unsigned char uart_recv_tout(unsigned int ms) {
    unsigned int t;
    for (t = 0; t < ms; t++) {
        if (RI) { RI = 0; return SBUF; }
        { unsigned int d; for (d = 0; d < 120; d++); }
    }
    return 0;
}

/* ---- IAP 操作 ---- */
static void iap_off(void) {
    IAP_CONTR = 0; IAP_CMD = 0; IAP_TRIG = 0;
}

static unsigned char iap_read(unsigned int addr) {
    unsigned char d;
    IAP_CONTR = 0x80;
    IAP_CMD = IAP_READ;
    IAP_ADDRH = (unsigned char)(addr >> 8);
    IAP_ADDRL = (unsigned char)(addr & 0xFF);
    IAP_TRIG = 0x5A; IAP_TRIG = 0xA5;
    _nop_(); _nop_();
    d = IAP_DATA;
    iap_off();
    return d;
}

static void iap_write(unsigned int addr, unsigned char d) {
    IAP_CONTR = 0x80;
    IAP_CMD = IAP_WRITE;
    IAP_ADDRH = (unsigned char)(addr >> 8);
    IAP_ADDRL = (unsigned char)(addr & 0xFF);
    IAP_DATA = d;
    IAP_TRIG = 0x5A; IAP_TRIG = 0xA5;
    _nop_(); _nop_();
    iap_off();
}

static void iap_erase_page(unsigned int addr) {
    IAP_CONTR = 0x80;
    IAP_CMD = IAP_ERASE;
    IAP_ADDRH = (unsigned char)(addr >> 8);
    IAP_ADDRL = (unsigned char)(addr & 0xFF);
    IAP_TRIG = 0x5A; IAP_TRIG = 0xA5;
    _nop_(); _nop_();
    iap_off();
}

/* 写 IAP 标志 (写在配置区, 不会影响代码) */
static void set_flag(unsigned char val) {
    iap_erase_page(FLAG_ADDR);
    iap_write(FLAG_ADDR, val);
}

/* 跳转到应用程序 */
static void jump_app(void) {
    iap_off();
    EA = 0;
    /* 复位到应用区: SWBS=0, SWRST=1 */
    IAP_CONTR = 0x20;
}

/* ---- 主函数 ---- */
void main(void) {
    unsigned char flag, cmd;
    unsigned int addr;
    unsigned char len, i, buf[32];

    SP = 0x7F;              /* 手动设置栈顶 */
    uart_init();
    delay_ms(100);

    /* 读取 IAP 标志 */
    flag = iap_read(FLAG_ADDR);

    if (flag != IAP_FLAG) {
        /* 检查应用区是否有有效代码 */
        if (iap_read(APP_ADDR) != 0xFF) {
            jump_app();     /* 正常跳转到应用 */
        }
        /* 应用区为空, 停留在 bootloader */
    }

    /* ====== IAP 模式 ====== */
    while (1) {
        cmd = uart_recv_tout(30000); /* 30秒超时 */
        if (cmd == 0) jump_app();     /* 超时则重启 */

        switch (cmd) {
        case BL_INFO:
            uart_send(BL_ACK);
            uart_send('S'); uart_send('T'); uart_send('C');
            uart_send('8'); uart_send('G');
            uart_send(0x08); /* 8KB */
            break;

        case BL_ERASE:
            addr = (unsigned int)uart_recv_tout(2000) << 8;
            addr |= uart_recv_tout(2000);
            if (addr < APP_SIZE)
                iap_erase_page(addr);
            uart_send(BL_ACK);
            break;

        case BL_WRITE:
            addr = (unsigned int)uart_recv_tout(2000) << 8;
            addr |= uart_recv_tout(2000);
            len = uart_recv_tout(2000);
            if (len > 32) len = 32;
            for (i = 0; i < len; i++)
                buf[i] = uart_recv_tout(2000);
            for (i = 0; i < len; i++)
                iap_write(addr + i, buf[i]);
            uart_send(BL_ACK);
            break;

        case BL_REBOOT:
            uart_send(BL_ACK);
            delay_ms(20);
            set_flag(0x00);     /* 清除 IAP 标志 */
            delay_ms(20);
            jump_app();
            break;

        default:
            uart_send(BL_NAK);
            break;
        }
    }
}