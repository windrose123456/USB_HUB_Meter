/************************************************************
 * bootloader.c - STC8G1K08A 最简测试
 * 验证 UART 通信: 发送 'B' + 心跳 '.'
 ************************************************************/
#include "STC8H.h"

#define		PSH		0x10

///* ---- SFR ---- */
//sfr IE    = 0xA8;
//sfr TCON  = 0x88;
//sfr SCON  = 0x98;
//sfr SBUF  = 0x99;
//sfr TMOD  = 0x89;
//sfr TL1   = 0x8B;
//sfr TH1   = 0x8D;
//sfr AUXR  = 0x8E;

//sbit TR1  = TCON^6;   /* Timer1 运行控制 */
//sbit TI   = SCON^1;   /* 发送完成标志 */

void main(void) {
    IE = 0;             /* 关中断 */
	
	P3M1 &= ~0x03;
    P3M0 &= ~0x03;

    /* --- 3. UART1 模式: 8位可变波特率, 允许接收 --- */
    SCON = (SCON & 0x3F) | 0x40;   // 模式1 (8位UART)
    REN = 1;                        // 允许接收

    /* --- 4. Timer1 波特率发生器 (1T, 115200) --- */
    TR1 = 0;                        // 先停止Timer1
    AUXR &= ~0x01;                  // S1 BRT 使用 Timer1
    TMOD &= ~(1 << 6);             // Timer1 = 定时器模式
    TMOD &= ~0x30;                  // Timer1 = 16位自动重装
    AUXR |= (1 << 6);              // Timer1 = 1T模式
	
//	j = (MAIN_Fosc / 4) / 115200ul;
//	j = 65536UL - j;
//	TH1 = (u8)(j>>8);
//	TL1 = (u8)j;
	TH1 = 0xFF;
	TL1 = 0xE8;
	ET1 = 0;                        // 禁止Timer1中断
    TMOD &= ~0x40;                  // 定时模式(非计数)
    INTCLKO &= ~0x02;              // 禁止T1时钟输出
    TR1 = 1;                        // 启动Timer1
	/* --- 5. NVIC: UART1中断使能, 优先级1 --- */
    ES = 1;
    IPH &= ~PSH;
    PS = 1;

    /* --- 6. UART1 引脚切换: P3.0(RXD) / P3.1(TXD) --- */
    P_SW1 = (P_SW1 & 0x3F) | (0 << 6);

    /* 延时等待稳定 */
    { unsigned int i; for (i = 0; i < 50000; i++); }

    /* 发送 'B' 确认启动 */
    SBUF = 'B'; while (!TI); TI = 0;

    /* 心跳循环: 每~500ms 发送 '.' */
    while (1) {
        { unsigned int j; for (j = 0; j < 50000; j++); }
        SBUF = 0x2E;
        while (!TI);
        TI = 0;
    }
}
