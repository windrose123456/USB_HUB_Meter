一、系统架构
┌──────────────────────────────────────────────────────┐
│                      USB 接口                         │
├──────────────────────────────────────────────────────┤
│                                                      │
│  ┌─────────┐  UART   ┌──────────┐                    │
│  │  CH340  │◄───────►│ STC8G1K  │   ┌───────────┐   │
│  │ (USB↔串)│  115200 │  08A     │──►│  INA226   │   │
│  └─────────┘         │ (8-pin)  │I2C│ (计量芯片) │   │
│                      │          │   └───────────┘   │
│         PC           │  P3.5 ──┼──► LED             │
│                      │  P5.5 ──┼──► CH634X RESET#   │
│                      └──────────┘   └───────────┘   │
│                                         HUB芯片       │
└──────────────────────────────────────────────────────┘

Flash 布局 (8KB):
┌─────────────────┐ 0x1FFF
│ ISP/IAP 区域    │
│  0x1C00-0x1EFF  │ Bootloader (768B)
│  0x1F00-0x1FFF  │ 配置/标志区 (256B)
├─────────────────┤ 0x1BFF
│ 应用程序区域    │
│  0x0000-0x1BFF  │ App Code (7KB)
└─────────────────┘ 0x0000

二、通信协议

2.1 帧格式

text
┌────────┬────────┬───────┬───────┬────────────┬──────────┐
│ HEAD1  │ HEAD2  │  LEN  │  CMD  │  DATA[0..N]│ CHECKSUM │
│  0xAA  │  0x55  │  1B   │  1B   │    N B     │    1B    │
└────────┴────────┴───────┴───────┴────────────┴──────────┘
CHECKSUM = HEAD1 ⊕ HEAD2 ⊕ LEN ⊕ CMD ⊕ DATA[0] ⊕ ... ⊕ DATA[N]
LEN = DATA 字节数
响应 CMD = 请求 CMD | 0x80
┌────────┬────────┬───────┬───────┬────────────┬──────────┐
│ HEAD1  │ HEAD2  │  LEN  │  CMD  │  DATA[0..N]│ CHECKSUM │
│  0xAA  │  0x55  │  1B   │  1B   │    N B     │    1B    │
└────────┴────────┴───────┴───────┴────────────┴──────────┘
CHECKSUM = HEAD1 ⊕ HEAD2 ⊕ LEN ⊕ CMD ⊕ DATA[0] ⊕ ... ⊕ DATA[N]
LEN = DATA 字节数
响应 CMD = 请求 CMD | 0x80

2.2 命令定义

CMD	名称	方向	DATA	说明
0x01	GET_DATA	PC→MCU	无	读取 INA226 计量数据
0x81	GET_DATA Resp	MCU→PC	10B	INA226 原始寄存器值
0x02	SET_LED	PC→MCU	1B: [ON/OFF]	控制 LED
0x82	SET_LED Resp	MCU→PC	1B: [状态]	返回当前状态
0x03	RESET_HUB	PC→MCU	无	复位 CH634X
0x83	RESET_HUB Resp	MCU→PC	无	复位完成
0x04	GET_INFO	PC→MCU	无	获取设备信息
0x84	GET_INFO Resp	MCU→PC	4B	版本+LED状态
0x10	ENTER_IAP	PC→MCU	无	进入固件更新模式
0x90	ENTER_IAP Resp	MCU→PC	无	确认后复位

2.3 GET_DATA 响应数据 (10 字节)

字节	内容	编码
0-1	Bus Voltage 寄存器原始值	uint16 BE, LSB=1.25mV
2-3	Shunt Voltage 寄存器原始值	int16 BE, LSB=2.5µV
4-5	Current 寄存器原始值	int16 BE
6-7	Power 寄存器原始值	uint16 BE, LSB=25×current_LSB
8-9	Manufacturer ID	uint16 BE, 固定 0x5449

2.4 Bootloader 协议（简化，无帧头校验）

发送: [CMD][参数...]
接收: [ACK=0x06 / NAK=0x15][响应数据...]

CMD_INFO    (0x05): 发 → 无参 → 收 [ACK][芯片信息]
CMD_ERASE   (0x01): 发 [AH][AL] → 收 [ACK]
CMD_WRITE   (0x02): 发 [AH][AL][LEN][DATA...] → 收 [ACK]
CMD_REBOOT  (0x04): 发 → 无参 → 收 [ACK] 然后复位

三、硬件接线

STC8G1K08A SOP-8 引脚分配

Pin	功能	连接目标
1	P3.5 / LED	LED→330Ω→GND (低电平点亮)
2	P3.3 / SDA	INA226 SDA (4.7kΩ上拉到VCC)
3	VCC	3.3V
4	P3.0 / RXD	CH340 TXD
5	P3.1 / TXD	CH340 RXD
6	P3.4 / SCL	INA226 SCL (4.7kΩ上拉到VCC)
7	P5.5 / RSTL	CH634X RESET# (加10kΩ上拉)
8	GND	GND

四、STC8G1K08A 固件

4.1 Bootloader — bootloader.c

编译到 ISP/IAP 区域（Flash 末尾 1KB，起始地址 0x1C00），Keil 中需去掉默认 STARTUP.A51，手动设置 SP。

/************************************************************
 * bootloader.c — STC8G1K08A IAP 引导程序
 * 编译目标地址: 0x1C00 (ISP/IAP 区域, 通过 STC-ISP 工具配置 1KB IAP)
 * 协议: 简化的字节协议 (无帧头, ACK/NAK)
 ************************************************************/
#include "STC8G.h"
#include <intrins.h>

/* ---- IAP 寄存器 ---- */
sfr IAP_DATA  = 0xC2;
sfr IAP_ADDRH = 0xC3;
sfr IAP_ADDRL = 0xC4;
sfr IAP_CMD   = 0xC5;
sfr IAP_TRIG  = 0xC6;
sfr IAP_CONTR = 0xC7;

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


4.2 Application — main.c

/************************************************************
 * main.c — STC8G1K08A 应用程序
 * 编译到 0x0000 起始地址 (默认设置)
 ************************************************************/
#include "STC8G.h"
#include <intrins.h>
#include <string.h>

/* ========== 常量定义 ========== */
#define FOSC    11059200UL
#define BAUD    115200UL

/* ---- IAP 寄存器 ---- */
sfr IAP_DATA  = 0xC2;
sfr IAP_ADDRH = 0xC3;
sfr IAP_ADDRL = 0xC4;
sfr IAP_CMD   = 0xC5;
sfr IAP_TRIG  = 0xC6;
sfr IAP_CONTR = 0xC7;
#define FLAG_ADDR   0x1F00
#define IAP_FLAG    0xA5

/* ---- 引脚 ---- */
sbit LED     = P3^5;    /* LED, 低电平点亮 */
sbit HUB_RST = P5^5;    /* CH634X RESET#, 低有效 */
sbit SDA     = P3^3;    /* I2C SDA */
sbit SCL     = P3^4;    /* I2C SCL */

/* ---- INA226 ---- */
#define INA_ADDR    0x40
#define REG_CFG     0x00
#define REG_SV      0x01    /* Shunt Voltage */
#define REG_BV      0x02    /* Bus Voltage */
#define REG_PWR     0x03    /* Power */
#define REG_CUR     0x04    /* Current */
#define REG_CAL     0x05    /* Calibration */
#define REG_MFR     0xFE    /* Manufacturer ID */

/* ---- 协议 ---- */
#define HEAD1   0xAA
#define HEAD2   0x55
#define MAX_DATA    10

#define CMD_GET_DATA    0x01
#define CMD_SET_LED     0x02
#define CMD_RESET_HUB   0x03
#define CMD_GET_INFO    0x04
#define CMD_ECHO        0x05
#define CMD_ENTER_IAP   0x10

#define STS_OK      0x00
#define STS_ERR     0x01

/* ========== 全局变量 ========== */
static volatile unsigned char xdata rx_ring[32];
static volatile unsigned char rx_head, rx_tail;

static unsigned char g_led_state;

/* ========== 延时 ========== */
static void delay_us(unsigned int us) {
    while (us--) { _nop_(); _nop_(); }
}

static void delay_ms(unsigned int ms) {
    unsigned int i;
    for (i = 0; i < ms; i++) {
        unsigned char j;
        for (j = 0; j < 120; j++);
    }
}

/* ========== UART ========== */
static void uart_init(void) {
    SCON = 0x50;
    AUXR |= 0x40;
    TMOD &= 0x0F;
    TH1 = (unsigned char)(256UL - (FOSC / 32UL / BAUD));
    TL1 = TH1;
    TR1 = 1;
}

static void uart_send(unsigned char c) {
    SBUF = c; while (!TI); TI = 0;
}

/* UART ISR — 收集到环形缓冲区 */
static void uart_isr(void) interrupt 4 {
    if (RI) {
        RI = 0;
        {
            unsigned char next = (rx_head + 1) & 0x1F;
            if (next != rx_tail) {
                rx_ring[rx_head] = SBUF;
                rx_head = next;
            }
        }
    }
    if (TI) TI = 0;
}

static unsigned char rx_available(void) {
    return (rx_head != rx_tail);
}

static unsigned char rx_read(void) {
    unsigned char d;
    while (rx_head == rx_tail);
    d = rx_ring[rx_tail];
    rx_tail = (rx_tail + 1) & 0x1F;
    return d;
}

/* ========== I2C Bit-Bang ========== */
static void i2c_start(void) {
    SDA = 1; SCL = 1; delay_us(5);
    SDA = 0; delay_us(5); SCL = 0;
}

static void i2c_stop(void) {
    SDA = 0; delay_us(5);
    SCL = 1; delay_us(5); SDA = 1;
}

static void i2c_write(unsigned char d) {
    unsigned char i;
    for (i = 0; i < 8; i++) {
        SDA = (d & 0x80) >> 7; d <<= 1;
        delay_us(3); SCL = 1; delay_us(5); SCL = 0;
    }
    SDA = 1; delay_us(3); SCL = 1; delay_us(5); SCL = 0;
}

static unsigned char i2c_read(unsigned char ack) {
    unsigned char i, d = 0;
    SDA = 1;
    for (i = 0; i < 8; i++) {
        d <<= 1; delay_us(3);
        SCL = 1; delay_us(5); d |= SDA; SCL = 0;
    }
    SDA = ack ? 0 : 1;
    delay_us(3); SCL = 1; delay_us(5); SCL = 0;
    SDA = 1;
    return d;
}

/* ========== INA226 ========== */
static void ina226_write(unsigned char reg, unsigned int val) {
    i2c_start();
    i2c_write(INA_ADDR << 1);
    i2c_write(reg);
    i2c_write((unsigned char)(val >> 8));
    i2c_write((unsigned char)(val & 0xFF));
    i2c_stop();
}

static unsigned int ina226_read(unsigned char reg) {
    unsigned int v;
    i2c_start();
    i2c_write(INA_ADDR << 1);
    i2c_write(reg);
    i2c_start();
    i2c_write((INA_ADDR << 1) | 1);
    v  = (unsigned int)i2c_read(1) << 8;
    v |= i2c_read(0);
    i2c_stop();
    return v;
}

static void ina226_init(void) {
    unsigned int id;
    id = ina226_read(REG_MFR);
    if (id != 0x5449) return;   /* 未检测到 INA226 */

    /* 配置: 1次平均, 1.1ms转换, 连续测量 */
    ina226_write(REG_CFG, 0x0247);

    /* 校准: Rshunt=10mΩ, MaxI=3.2A → Cal=5120 */
    ina226_write(REG_CAL, 0x1400);
}

/* ========== 协议帧处理 ========== */

/*
 * 从环形缓冲区解析一帧数据
 * 返回 1=收到有效帧, 0=无数据
 */
static unsigned char parse_packet(unsigned char *pcmd,
                                  unsigned char *pdata,
                                  unsigned char *plen)
{
    static unsigned char st = 0;
    static unsigned char cmd, len, idx, chk;

    while (rx_available()) {
        unsigned char c = rx_read();
        switch (st) {
        case 0: if (c == HEAD1) st = 1; break;
        case 1: if (c == HEAD2) st = 2; else st = 0; break;
        case 2:
            len = c;
            if (len > MAX_DATA) { st = 0; break; }
            chk = HEAD1 ^ HEAD2 ^ c;
            idx = 0; st = 3;
            break;
        case 3:
            cmd = c; chk ^= c; st = 4;
            break;
        case 4:
            if (idx < len) { pdata[idx++] = c; chk ^= c; }
            if (idx >= len) st = 5;
            break;
        case 5:
            st = 0;
            if (c == chk) {
                *pcmd = cmd;
                *plen = len;
                return 1;
            }
            break;
        }
    }
    return 0;
}

/* 发送响应帧 */
static void send_resp(unsigned char cmd, unsigned char sts,
                       unsigned char *data, unsigned char len)
{
    unsigned char i, chk;
    uart_send(HEAD1);
    uart_send(HEAD2);
    uart_send(len + 1);
    uart_send(cmd | 0x80);
    uart_send(sts);
    chk = HEAD1 ^ HEAD2 ^ (len + 1) ^ (cmd | 0x80) ^ sts;
    for (i = 0; i < len; i++) {
        uart_send(data[i]);
        chk ^= data[i];
    }
    uart_send(chk);
}

/* ========== 命令处理 ========== */
static void process_cmd(unsigned char cmd,
                         unsigned char *data, unsigned char len)
{
    unsigned char resp[10];

    switch (cmd) {

    case CMD_GET_DATA: {
        unsigned int raw;
        raw = ina226_read(REG_BV);
        resp[0] = (unsigned char)(raw >> 8);
        resp[1] = (unsigned char)(raw & 0xFF);
        raw = ina226_read(REG_SV);
        resp[2] = (unsigned char)(raw >> 8);
        resp[3] = (unsigned char)(raw & 0xFF);
        raw = ina226_read(REG_CUR);
        resp[4] = (unsigned char)(raw >> 8);
        resp[5] = (unsigned char)(raw & 0xFF);
        raw = ina226_read(REG_PWR);
        resp[6] = (unsigned char)(raw >> 8);
        resp[7] = (unsigned char)(raw & 0xFF);
        raw = ina226_read(REG_MFR);
        resp[8] = (unsigned char)(raw >> 8);
        resp[9] = (unsigned char)(raw & 0xFF);
        send_resp(cmd, STS_OK, resp, 10);
        break;
    }

    case CMD_SET_LED:
        if (len >= 1) {
            g_led_state = data[0] ? 1 : 0;
            LED = g_led_state ? 0 : 1;     /* 低电平点亮 */
            resp[0] = g_led_state;
            send_resp(cmd, STS_OK, resp, 1);
        } else {
            send_resp(cmd, STS_ERR, 0, 0);
        }
        break;

    case CMD_RESET_HUB:
        HUB_RST = 0;
        delay_ms(20);
        HUB_RST = 1;
        delay_ms(100);
        send_resp(cmd, STS_OK, 0, 0);
        break;

    case CMD_GET_INFO:
        resp[0] = 0x01;            /* 固件版本 1.0 */
        resp[1] = 0x00;
        resp[2] = 0x08;            /* 8KB Flash */
        resp[3] = g_led_state;
        send_resp(cmd, STS_OK, resp, 4);
        break;

    case CMD_ECHO:
        send_resp(cmd, STS_OK, data, len);
        break;

    case CMD_ENTER_IAP: {
        /* 先回复确认 */
        send_resp(cmd, STS_OK, 0, 0);
        delay_ms(50);
        /* 擦除标志页并写入 IAP 标志 */
        IAP_CONTR = 0x80;
        IAP_CMD = IAP_ERASE;
        IAP_ADDRH = (unsigned char)(FLAG_ADDR >> 8);
        IAP_ADDRL = (unsigned char)(FLAG_ADDR & 0xFF);
        IAP_TRIG = 0x5A; IAP_TRIG = 0xA5;
        _nop_(); _nop_();
        IAP_CMD = IAP_WRITE;
        IAP_DATA = IAP_FLAG;
        IAP_TRIG = 0x5A; IAP_TRIG = 0xA5;
        _nop_(); _nop_();
        IAP_CONTR = 0; IAP_CMD = 0;
        delay_ms(50);
        /* 软件复位到 ISP 区域 */
        IAP_CONTR = 0x60;  /* SWBS=1, SWRST=1 */
        break;
    }

    default:
        send_resp(cmd, STS_ERR, 0, 0);
        break;
    }
}

/* ========== 主函数 ========== */
void main(void) {
    unsigned char cmd, data[MAX_DATA], len;

    /* 初始化 */
    LED = 1;                /* LED 关 (高) */
    HUB_RST = 1;            /* HUB 正常 */
    g_led_state = 0;

    rx_head = 0; rx_tail = 0;
    uart_init();
    EA = 1;                 /* 开总中断 */

    delay_ms(200);
    ina226_init();

    /* 主循环 */
    while (1) {
        if (parse_packet(&cmd, data, &len)) {
            process_cmd(cmd, data, len);
        }
    }
}

4.3 Keil 工程配置

Bootloader 工程 (bootloader.uvproj)：


设置项	值
Device	STC8G1K08A
Code ROM Size	LARGE
STARTUP.A51	不包含（去掉或禁用）
BL51 → Code 范围	0x1C00-0x1FFF
优化级别	Level 8 (Size)

编译后用 STC-ISP 工具下载到 ISP/IAP 区域（在 STC-ISP 中设置 IAP 大小为 1KB）。


Application 工程 (app.uvproj)：


设置项	值
Device	STC8G1K08A
STARTUP.A51	包含
优化级别	Level 8 (Size)

编译后用 STC-ISP 下载到 应用区域（起始地址 0x0000）。


STC-ISP 工具配置要点：

IRC 频率设为 11.0592 MHz
IAP/IAP 大小设为 1024 字节 (1KB)
第一次烧录时先通过 STC-ISP 同时下载 Bootloader 和 Application

五、C# 上位机

5.1 创建工程

Visual Studio → 新建 Windows Forms App (.NET Framework) → 项目名 USBMonitor。


将 Form1.cs 全部替换为以下代码：

using System;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace USBMonitor
{
    public class MainForm : Form
    {
        /* ========== 协议常量 ========== */
        const byte HEAD1 = 0xAA, HEAD2 = 0x55;
        const byte CMD_GET_DATA = 0x01, CMD_SET_LED = 0x02,
                    CMD_RESET_HUB = 0x03, CMD_GET_INFO = 0x04,
                    CMD_ECHO = 0x05, CMD_ENTER_IAP = 0x10;

        /* Bootloader 命令 */
        const byte BL_INFO = 0x05, BL_ERASE = 0x01,
                    BL_WRITE = 0x02, BL_REBOOT = 0x04,
                    BL_ACK = 0x06, BL_NAK = 0x15;

        /* ========== 成员 ========== */
        SerialPort _port;
        bool _connected, _ledOn;
        double _currentLsb = 0.0001; // 0.1mA per bit (Rshunt=10mΩ, 3.2A)

        /* ========== UI 控件 ========== */
        ComboBox _cmbPorts;
        Button _btnRefresh, _btnConnect;
        Label _lblStatus;
        TabControl _tabs;

        /* INA226 Tab */
        Label _lblBV, _lblSV, _lblCur, _lblPwr, _lblRawBV, _lblRawSV;
        Button _btnRead;
        CheckBox _chkAuto;
        NumericUpDown _nudInterval;
        Timer _timer;

        /* Control Tab */
        Button _btnLED, _btnReset;
        Label _lblLEDDisplay;

        /* Firmware Tab */
        TextBox _txtFwPath;
        Button _btnBrowse, _btnUpdate;
        ProgressBar _progBar;
        Label _lblFwStatus;
        RichTextBox _rtbLog;

        /* ====== 构造 ====== */
        public MainForm()
        {
            BuildUI();
            RefreshPorts();
            UpdateConnectionUI();
        }

        /* ================================================================
         *  UI 构建 — 全部代码创建，无 Designer 依赖
         * ================================================================ */
        void BuildUI()
        {
            Text = "USB Power Monitor — STC8G1K08A";
            Size = new Size(640, 560);
            FormBorderStyle = FormBorderStyle.FixedSingle;
            MaximizeBox = false;
            StartPosition = FormStartPosition.CenterScreen;
            BackColor = Color.FromArgb(24, 24, 28);
            ForeColor = Color.FromArgb(220, 220, 224);
            Font = new Font("Segoe UI", 9f);

            /* ---- 顶部: 串口连接栏 ---- */
            var pnl = MakePanel(new Rectangle(8, 8, 608, 48));

            _cmbPorts = new ComboBox {
                Location = new Point(10, 11), Size = new Size(130, 25),
                DropDownStyle = ComboBoxStyle.DropDownList, BackColor = Color.FromArgb(40, 40, 44)
            };
            _btnRefresh = MakeBtn("刷新", 150, 10, 55, DoRefresh);
            _btnConnect = MakeBtn("连接", 215, 10, 60, DoConnect);
            _lblStatus = new Label {
                Text = "未连接", AutoSize = true,
                Location = new Point(295, 15),
                ForeColor = Color.FromArgb(120, 120, 124)
            };
            pnl.Controls.AddRange(new Control[] { _cmbPorts, _btnRefresh, _btnConnect, _lblStatus });

            /* ---- TabControl ---- */
            _tabs = new TabControl {
                Location = new Point(8, 64), Size = new Size(608, 448),
                BackColor = Color.FromArgb(24, 24, 28)
            };

            /* -- Tab 1: INA226 -- */
            var tp1 = new TabPage("INA226 计量");
            tp1.BackColor = Color.FromArgb(30, 30, 34);
            tp1.ForeColor = Color.White;

            var grpV = MakeGroup("Bus Voltage (总线电压)", 10, 10, 280, 80);
            _lblBV = MakeValueLabel("— V", grpV, 10, 22);
            _lblRawBV = MakeSmallLabel("Raw: —", grpV, 10, 52);

            var grpS = MakeGroup("Shunt Voltage (分流电压)", 300, 10, 280, 80);
            _lblSV = MakeValueLabel("— mV", grpS, 10, 22);
            _lblRawSV = MakeSmallLabel("Raw: —", grpS, 10, 52);

            var grpI = MakeGroup("Current (电流)", 10, 100, 280, 80);
            _lblCur = MakeValueLabel("— A", grpI, 10, 22);

            var grpP = MakeGroup("Power (功率)", 300, 100, 280, 80);
            _lblPwr = MakeValueLabel("— W", grpP, 10, 22);

            _btnRead = MakeBtn("读取一次", 10, 195, 100, DoReadINA);
            _chkAuto = new CheckBox {
                Text = "自动刷新", Location = new Point(125, 198),
                AutoSize = true
            };
            _chkAuto.CheckedChanged += (s, e) => {
                _timer.Enabled = _chkAuto.Checked;
            };
            var lblInt = new Label { Text = "间隔 ms:", Location = new Point(230, 200), AutoSize = true };
            _nudInterval = new NumericUpDown {
                Location = new Point(295, 196), Size = new Size(70, 23),
                Minimum = 100, Maximum = 5000, Value = 500, Increment = 100
            };
            _nudInterval.ValueChanged += (s, e) => _timer.Interval = (int)_nudInterval.Value;

            _timer = new Timer { Interval = 500 };
            _timer.Tick += (s, e) => DoReadINA();

            tp1.Controls.AddRange(new Control[] {
                grpV, grpS, grpI, grpP,
                _btnRead, _chkAuto, lblInt, _nudInterval
            });

            /* -- Tab 2: 控制 -- */
            var tp2 = new TabPage("控制");
            tp2.BackColor = Color.FromArgb(30, 30, 34);

            var grpLED = MakeGroup("LED 控制", 10, 10, 280, 120);
            _btnLED = MakeBtn("开启 LED", 15, 28, 120, DoToggleLED);
            _lblLEDDisplay = new Label {
                Text = "● OFF", AutoSize = true,
                Location = new Point(150, 32),
                Font = new Font("Segoe UI", 12f, FontStyle.Bold),
                ForeColor = Color.FromArgb(100, 100, 100)
            };
            grpLED.Controls.AddRange(new Control[] { _btnLED, _lblLEDDisplay });

            var grpHub = MakeGroup("CH634X HUB", 300, 10, 280, 120);
            _btnReset = MakeBtn("复位 HUB", 15, 28, 120, DoResetHUB);
            grpHub.Controls.Add(_btnReset);

            tp2.Controls.AddRange(new Control[] { grpLED, grpHub });

            /* -- Tab 3: 固件更新 -- */
            var tp3 = new TabPage("固件更新");
            tp3.BackColor = Color.FromArgb(30, 30, 34);

            var lbl1 = new Label { Text = "固件文件 (.bin / .hex):", Location = new Point(10, 15), AutoSize = true };
            _txtFwPath = new TextBox {
                Location = new Point(10, 38), Size = new Size(430, 23),
                BackColor = Color.FromArgb(40, 40, 44)
            };
            _btnBrowse = MakeBtn("浏览...", 450, 37, 70, DoBrowseFirmware);
            _btnUpdate = MakeBtn("开始更新", 10, 72, 110, DoFirmwareUpdate);
            _progBar = new ProgressBar {
                Location = new Point(130, 73), Size = new Size(450, 22)
            };
            _lblFwStatus = new Label {
                Text = "", Location = new Point(10, 102),
                AutoSize = true, ForeColor = Color.FromArgb(180, 180, 180)
            };
            _rtbLog = new RichTextBox {
                Location = new Point(10, 128), Size = new Size(580, 270),
                ReadOnly = true, BackColor = Color.FromArgb(18, 18, 22),
                ForeColor = Color.FromArgb(160, 200, 160),
                Font = new Font("Consolas", 8.5f),
                BorderStyle = BorderStyle.FixedSingle
            };
            tp3.Controls.AddRange(new Control[] {
                lbl1, _txtFwPath, _btnBrowse,
                _btnUpdate, _progBar, _lblFwStatus, _rtbLog
            });

            _tabs.TabPages.AddRange(new[] { tp1, tp2, tp3 });
            Controls.AddRange(new Control[] { pnl, _tabs });
        }

        /* ---- UI 辅助方法 ---- */
        Panel MakePanel(Rectangle r) {
            var p = new Panel { Bounds = r, BackColor = Color.FromArgb(38, 38, 42) };
            return p;
        }
        GroupBox MakeGroup(string t, int x, int y, int w, int h) {
            return new GroupBox { Text = t, Location = new Point(x, y), Size = new Size(w, h),
                ForeColor = Color.FromArgb(180, 180, 184) };
        }
        Button MakeBtn(string t, int x, int y, int w, EventHandler h) {
            var b = new Button {
                Text = t, Location = new Point(x, y), Size = new Size(w, 28),
                FlatStyle = FlatStyle.Flat,
                BackColor = Color.FromArgb(50, 50, 56),
                ForeColor = Color.White
            };
            b.FlatAppearance.BorderColor = Color.FromArgb(80, 80, 86);
            b.Click += h;
            return b;
        }
        Label MakeValueLabel(string init, Control parent, int x, int y) {
            var l = new Label {
                Text = init, Location = new Point(x, y), AutoSize = true,
                Font = new Font("Consolas", 16f, FontStyle.Bold),
                ForeColor = Color.FromArgb(0, 200, 120)
            };
            parent.Controls.Add(l);
            return l;
        }
        Label MakeSmallLabel(string init, Control parent, int x, int y) {
            var l = new Label {
                Text = init, Location = new Point(x, y), AutoSize = true,
                Font = new Font("Consolas", 8f), ForeColor = Color.Gray
            };
            parent.Controls.Add(l);
            return l;
        }

        /* ================================================================
         *  串口操作
         * ================================================================ */
        void RefreshPorts() {
            _cmbPorts.Items.Clear();
            _cmbPorts.Items.AddRange(SerialPort.GetPortNames());
            if (_cmbPorts.Items.Count > 0) _cmbPorts.SelectedIndex = 0;
        }
        void DoRefresh(object s, EventArgs e) => RefreshPorts();

        void DoConnect(object s, EventArgs e) {
            if (_connected) {
                _port?.Close();
                _connected = false;
            } else {
                if (_cmbPorts.SelectedItem == null) return;
                try {
                    _port = new SerialPort(_cmbPorts.SelectedItem.ToString(), 115200, Parity.None, 8, StopBits.One) {
                        ReadTimeout = 1000, WriteTimeout = 1000
                    };
                    _port.Open();
                    _connected = true;
                } catch (Exception ex) {
                    MessageBox.Show("串口打开失败: " + ex.Message, "错误",
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
            }
            UpdateConnectionUI();
        }

        void UpdateConnectionUI() {
            _btnConnect.Text = _connected ? "断开" : "连接";
            _lblStatus.Text = _connected ? "● 已连接" : "未连接";
            _lblStatus.ForeColor = _connected ? Color.FromArgb(0, 180, 100) : Color.Gray;
        }

        /* ================================================================
         *  协议: 发送命令 / 接收响应
         * ================================================================ */
        byte[] SendCmd(byte cmd, byte[] data, int timeoutMs = 1000) {
            if (!_connected || _port == null) return null;

            byte[] pkt = BuildPacket(cmd, data);

            _port.DiscardInBuffer();
            _port.Write(pkt, 0, pkt.Length);

            /* 接收响应 */
            return ReadResponse(timeoutMs);
        }

        byte[] BuildPacket(byte cmd, byte[] data) {
            int dLen = data?.Length ?? 0;
            byte len = (byte)dLen;
            byte[] pkt = new byte[5 + dLen];
            pkt[0] = HEAD1; pkt[1] = HEAD2;
            pkt[2] = len; pkt[3] = cmd;
            if (data != null) Array.Copy(data, 0, pkt, 4, dLen);
            byte chk = 0;
            for (int i = 0; i < pkt.Length - 1; i++) chk ^= pkt[i];
            pkt[pkt.Length - 1] = chk;
            return pkt;
        }

        byte[] ReadResponse(int timeoutMs) {
            var buf = new byte[64];
            int count = 0;
            var sw = System.Diagnostics.Stopwatch.StartNew();

            while (sw.ElapsedMilliseconds < timeoutMs) {
                try {
                    if (_port.BytesToRead > 0) {
                        buf[count++] = (byte)_port.ReadByte();
                        if (count >= 5) {
                            int dataLen = buf[2];
                            if (count >= 5 + dataLen) break;
                        }
                    } else {
                        Thread.Sleep(5);
                    }
                } catch { break; }
            }
            if (count < 5) return null;

            int dLen = buf[2];
            if (count < 5 + dLen) return null;

            /* 校验 */
            byte chk = 0;
            for (int i = 0; i < 4 + dLen; i++) chk ^= buf[i];
            if (chk != buf[4 + dLen]) return null;

            byte resCmd = buf[3];
            byte status = buf[4];
            byte[] payload = new byte[dLen];
            if (dLen > 0) Array.Copy(buf, 5, payload, 0, dLen);
            return payload;
        }

        /* ================================================================
         *  INA226 数据读取与显示
         * ================================================================ */
        void DoReadINA() {
            if (!_connected) return;
            try {
                byte[] r = SendCmd(CMD_GET_DATA, null, 500);
                if (r == null || r.Length < 10) {
                    _lblBV.Text = "读取失败";
                    return;
                }

                ushort rawBV = (ushort)((r[0] << 8) | r[1]);
                short rawSV = (short)((r[2] << 8) | r[3]);
                short rawCur = (short)((r[4] << 8) | r[5]);
                ushort rawPwr = (ushort)((r[6] << 8) | r[7]);
                ushort mfrId = (ushort)((r[8] << 8) | r[9]);

                /* INA226 换算 */
                double busV = rawBV * 1.25 / 1000.0;          // mV → V
                double shuntV = rawSV * 2.5 / 1000.0;         // µV → mV
                double current = rawCur * _currentLsb * 1000;  // → mA
                double power = rawPwr * 25.0 * _currentLsb;    // → W

                _lblBV.Text = $"{busV:F4} V";
                _lblSV.Text = $"{shuntV:F4} mV";
                _lblCur.Text = $"{current / 1000.0:F4} A";
                _lblPwr.Text = $"{power:F4} W";
                _lblRawBV.Text = $"Raw: 0x{rawBV:X4}";
                _lblRawSV.Text = $"Raw: 0x{rawSV:X4}  MFR: 0x{mfrId:X4}";
            } catch (Exception ex) {
                _lblBV.Text = "异常: " + ex.Message;
            }
        }

        /* ================================================================
         *  LED / HUB 控制
         * ================================================================ */
        void DoToggleLED(object s, EventArgs e) {
            if (!_connected) return;
            _ledOn = !_ledOn;
            byte[] r = SendCmd(CMD_SET_LED, new byte[] { (byte)(_ledOn ? 1 : 0) });
            if (r != null && r.Length >= 1) {
                _ledOn = r[0] != 0;
            }
            _btnLED.Text = _ledOn ? "关闭 LED" : "开启 LED";
            _lblLEDDisplay.Text = _ledOn ? "● ON" : "● OFF";
            _lblLEDDisplay.ForeColor = _ledOn ? Color.LimeGreen : Color.Gray;
        }

        void DoResetHUB(object s, EventArgs e) {
            if (!_connected) return;
            _btnReset.Enabled = false;
            _btnReset.Text = "复位中...";
            var t = new Thread(() => {
                SendCmd(CMD_RESET_HUB, null, 2000);
                this.BeginInvoke((Action)(() => {
                    _btnReset.Enabled = true;
                    _btnReset.Text = "复位 HUB";
                    MessageBox.Show("CH634X HUB 已复位", "完成",
                        MessageBoxButtons.OK, MessageBoxIcon.Information);
                }));
            }) { IsBackground = true };
            t.Start();
        }

        /* ================================================================
         *  固件更新
         * ================================================================ */
        void DoBrowseFirmware(object s, EventArgs e) {
            using (var dlg = new OpenFileDialog()) {
                dlg.Filter = "固件文件|*.bin;*.hex|所有文件|*.*";
                if (dlg.ShowDialog() == DialogResult.OK)
                    _txtFwPath.Text = dlg.FileName;
            }
        }

        byte[] LoadFirmware(string path) {
            if (path.EndsWith(".bin", StringComparison.OrdinalIgnoreCase)) {
                return File.ReadAllBytes(path);
            }
            if (path.EndsWith(".hex", StringComparison.OrdinalIgnoreCase)) {
                return ParseIntelHex(path);
            }
            return null;
        }

        byte[] ParseIntelHex(string path) {
            var lines = File.ReadAllLines(path);
            var data = new System.Collections.Generic.List<byte>();
            int maxAddr = 0;
            foreach (var line in lines) {
                if (string.IsNullOrEmpty(line) || line[0] != ':') continue;
                int byteCount = int.Parse(line.Substring(1, 2), NumberStyles.Hex);
                int address = int.Parse(line.Substring(3, 4), NumberStyles.Hex);
                int recType = int.Parse(line.Substring(7, 2), NumberStyles.Hex);
                if (recType == 0x01) break; // EOF
                if (recType != 0x00) continue;

                while (data.Count < address + byteCount)
                    data.Add(0xFF);

                for (int i = 0; i < byteCount; i++) {
                    byte b = byte.Parse(line.Substring(9 + i * 2, 2), NumberStyles.Hex);
                    data[address + i] = b;
                    if (address + i > maxAddr) maxAddr = address + i;
                }
            }
            return data.GetRange(0, maxAddr + 1).ToArray();
        }

        void DoFirmwareUpdate(object s, EventArgs e) {
            if (!_connected) { MessageBox.Show("请先连接设备"); return; }
            if (string.IsNullOrEmpty(_txtFwPath.Text) || !File.Exists(_txtFwPath.Text)) {
                MessageBox.Show("请选择有效的固件文件"); return;
            }

            byte[] fwBin = LoadFirmware(_txtFwPath.Text);
            if (fwBin == null || fwBin.Length == 0) {
                MessageBox.Show("固件文件为空或格式不支持"); return;
            }

            const int PAGE_SIZE = 512;
            const int APP_MAX = 0x1C00; // 7KB
            if (fwBin.Length > APP_MAX) {
                MessageBox.Show($"固件大小 {fwBin.Length} 超过应用区 {APP_MAX} 字节"); return;
            }

            /* Pad to page boundary */
            int pageCount = (fwBin.Length + PAGE_SIZE - 1) / PAGE_SIZE;
            int totalSize = pageCount * PAGE_SIZE;
            Array.Resize(ref fwBin, totalSize);

            _btnUpdate.Enabled = false;
            _rtbLog.Clear();
            _progBar.Value = 0;
            _progBar.Maximum = pageCount * 2 + 2; // erase + write + reboot

            var t = new Thread(() => RunFirmwareUpdate(fwBin, pageCount, PAGE_SIZE));
            t.IsBackground = true;
            t.Start();
        }

        void RunFirmwareUpdate(byte[] fw, int pageCount, int pageSize)
        {
            try {
                Log("===== 固件更新开始 =====");
                Log($"固件大小: {fw.Length} bytes, {pageCount} 页");

                /* 1. 发送 ENTER_IAP 命令 */
                Log("步骤 1/4: 进入 IAP 模式...");
                byte[] r = SendCmd(CMD_ENTER_IAP, null, 2000);
                if (r == null) {
                    Log("警告: 未收到确认, 继续尝试...");
                } else {
                    Log("设备已进入 IAP 模式");
                }

                /* 等待 MCU 复位进入 bootloader */
                Thread.Sleep(1500);
                _port.DiscardInBuffer();

                /* 2. 握手 bootloader */
                Log("步骤 2/4: 与 Bootloader 握手...");
                if (!SendBlCmd(BL_INFO, null, 200)) {
                    Log("错误: Bootloader 无响应!");
                    FinishUpdate(false);
                    return;
                }
                Log("Bootloader 已就绪");

                /* 3. 擦除 + 写入 */
                Log($"步骤 3/4: 擦写 {pageCount} 页...");
                for (int pg = 0; pg < pageCount; pg++) {
                    int addr = pg * pageSize;

                    /* Erase */
                    byte[] eraseParam = new byte[] {
                        (byte)(addr >> 8), (byte)(addr & 0xFF)
                    };
                    if (!SendBlCmd(BL_ERASE, eraseParam, 1000)) {
                        Log($"擦除页 {pg} 失败 (0x{addr:X4})");
                        FinishUpdate(false);
                        return;
                    }
                    SetProgress(pg * 2 + 1);
                    Log($"  擦除页 {pg}: 0x{addr:X4} OK");

                    /* Write (分 32 字节块) */
                    for (int offset = 0; offset < pageSize; offset += 32) {
                        int chunkLen = Math.Min(32, pageSize - offset);
                        byte[] wrParam = new byte[3 + chunkLen];
                        wrParam[0] = (byte)((addr + offset) >> 8);
                        wrParam[1] = (byte)((addr + offset) & 0xFF);
                        wrParam[2] = (byte)chunkLen;
                        Array.Copy(fw, addr + offset, wrParam, 3, chunkLen);

                        if (!SendBlCmd(BL_WRITE, wrParam, 1000)) {
                            Log($"写入失败: 0x{addr + offset:X4}");
                            FinishUpdate(false);
                            return;
                        }
                    }
                    SetProgress(pg * 2 + 2);
                }

                /* 4. 复位到应用 */
                Log("步骤 4/4: 复位 MCU...");
                SendBlCmd(BL_REBOOT, null, 200);
                SetProgress(_progBar.Maximum);
                Log("===== 固件更新完成! =====");
                FinishUpdate(true);

            } catch (Exception ex) {
                Log("异常: " + ex.Message);
                FinishUpdate(false);
            }
        }

        bool SendBlCmd(byte cmd, byte[] data, int timeoutMs) {
            var buf = new byte[1 + (data?.Length ?? 0)];
            buf[0] = cmd;
            if (data != null) Array.Copy(data, 0, buf, 1, data.Length);

            _port.DiscardInBuffer();
            _port.Write(buf, 0, buf.Length);

            /* 等待 ACK/NAK */
            var sw = System.Diagnostics.Stopwatch.StartNew();
            while (sw.ElapsedMilliseconds < timeoutMs) {
                try {
                    if (_port.BytesToRead > 0) {
                        byte resp = (byte)_port.ReadByte();
                        if (resp == BL_ACK) return true;
                        if (resp == BL_NAK) return false;
                    } else {
                        Thread.Sleep(5);
                    }
                } catch { break; }
            }
            return false;
        }

        void Log(string msg) {
            if (InvokeRequired) {
                BeginInvoke((Action)(() => Log(msg)));
                return;
            }
            _rtbLog.AppendText($"[{DateTime.Now:HH:mm:ss}] {msg}\n");
            _rtbLog.ScrollToCaret();
        }

        void SetProgress(int val) {
            if (InvokeRequired) {
                BeginInvoke((Action)(() => SetProgress(val)));
                return;
            }
            _progBar.Value = Math.Min(val, _progBar.Maximum);
        }

        void FinishUpdate(bool ok) {
            if (InvokeRequired) {
                BeginInvoke((Action)(() => FinishUpdate(ok)));
                return;
            }
            _btnUpdate.Enabled = true;
            _lblFwStatus.Text = ok ? "更新成功!" : "更新失败，请重试";
            _lblFwStatus.ForeColor = ok ? Color.LimeGreen : Color.OrangeRed;
            if (ok) _progBar.Value = _progBar.Maximum;
        }

        /* ================================================================
         *  清理
         * ================================================================ */
        protected override void OnFormClosing(FormClosingEventArgs e) {
            _timer?.Stop();
            if (_port?.IsOpen == true) _port.Close();
            base.OnFormClosing(e);
        }

        [STAThread]
        static void Main() {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
        }
    }
}

六、使用流程

首次烧录
1. 用 STC-ISP 连接 STC8G1K08A:
   - IRC 频率: 11.0592 MHz
   - IAP/IAP 大小: 1024 字节

2. 烧录 bootloader.hex → ISP/IAP 区域

3. 烧录 app.hex → 应用区域 (起始 0x0000)

日常使用
1. 打开 C# 上位机 → 选择 COM 口 → 点击连接
2. INA226 标签页: 点击"读取一次"或勾选"自动刷新"查看实时数据
3. 控制标签页: LED 开关 / HUB 复位
4. 固件更新:
   a. 选择 .bin 或 .hex 固件文件
   b. 点击"开始更新"
   c. 上位机自动: 进入IAP → 握手Bootloader → 擦写 → 复位
   d. 日志窗口显示详细进度

IAP 更新流程时序
PC                    MCU(App)              MCU(Boot)
 │  GET_DATA(0x01)  →  [正常响应]
 │                       │
 │  ENTER_IAP(0x10) →   [回复ACK]
 │                       [写标志→复位]
 │                  ──────┤
 │  等待 1.5s              │
 │  BL_INFO(0x05)   →     │  [ACK + 芯片信息]
 │  BL_ERASE ×14    →     │  [ACK] × 14
 │  BL_WRITE ×14    →     │  [ACK] × 14
 │  BL_REBOOT(0x04) →     │  [ACK]
 │                       │  [清标志→跳转APP]
 │                  ←─────┤
 │  设备重新枚举          新固件开始运行

 关键注意事项：


1.引脚确认：SOP-8 封装引脚顺序请务必核对 STC 官方 datasheet
2.INA226 校准：默认按 10mΩ 采样电阻、3.2A 量程配置，如需修改请调整 REG_CAL 写入值和 C# 端 _currentLsb
3.波特率：两端必须一致 (115200)，STC-ISP 中确认 IRC 频率为 11.0592 MHz
4.CH634X 时序：复位低电平持续 20ms 已足够，如芯片手册有更长要求请调整 delay_ms