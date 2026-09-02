/*---------------------------------------------------------------------*/
/* --- STC MCU Limited ------------------------------------------------*/
/* --- STC 1T Series MCU Demo Programme -------------------------------*/
/*---------------------------------------------------------------------*/

#include	"config.h"
#include	"STC8G_H_GPIO.h"
#include	"STC8G_H_UART.h"
#include	"STC8G_H_Timer.h"
#include	"STC8G_H_Delay.h"
#include	"STC8G_H_NVIC.h"
#include	"STC8G_H_Switch.h"
#include	"procotol.h"

/*************  ���ڳ�������	**************/
volatile u16 uart_rx_timeout_cnt = 0;
static u16 uart_timeout_threshold = 0;
static bit uart_frame_complete = 0;

u8 get_reg_buf[8];

/******************* IO���ú��� *******************/
void	GPIO_config(void)
{
	GPIO_InitTypeDef	GPIO_InitStructure;

	GPIO_InitStructure.Pin  = GPIO_Pin_5;
	GPIO_InitStructure.Mode = GPIO_OUT_PP;
	GPIO_Inilize(GPIO_P5,&GPIO_InitStructure);

	GPIO_InitStructure.Pin  = GPIO_Pin_4;
	GPIO_InitStructure.Mode = GPIO_HighZ;
	GPIO_Inilize(GPIO_P5,&GPIO_InitStructure);
	delay_ms(10);
}

/***************  ���ڳ�ʼ������ *****************/
void	UART_config(void)
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	COMx_InitDefine		COMx_InitStructure;

	GPIO_InitStructure.Pin  = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.Mode = GPIO_PullUp;
	GPIO_Inilize(GPIO_P3,&GPIO_InitStructure);

	COMx_InitStructure.UART_Mode      = UART_8bit_BRTx;
	COMx_InitStructure.UART_BRT_Use   = BRT_Timer1;
	COMx_InitStructure.UART_BaudRate  = 115200ul;
	COMx_InitStructure.UART_RxEnable  = ENABLE;
	COMx_InitStructure.BaudRateDouble = DISABLE;

    uart_timeout_threshold = UART_Get_3_5CharTime_10us(COMx_InitStructure.UART_BaudRate);

	UART_Configuration(UART1, &COMx_InitStructure);
	NVIC_UART1_Init(ENABLE,Priority_1);
	UART1_SW(UART1_SW_P30_P31);
}

void Boot_UART_config(void)
{
    u16 i;
	u32	j;

    /* --- 1. GPIO 配置: P3.0 / P3.1 准双向上拉 --- */
    P3M1 &= ~0x03;
    P3M0 &= ~0x03;

    /* --- 2. COM1 结构体初始化 --- */
    COM1.TX_send    = 0;
    COM1.TX_write   = 0;
    COM1.B_TX_busy  = 0;
    COM1.RX_Cnt     = 0;
    COM1.RX_TimeOut = 0;
    for (i = 0; i < COM_TX1_Lenth; i++) TX1_Buffer[i] = 0;
    for (i = 0; i < COM_RX1_Lenth; i++) RX1_Buffer[i] = 0;

    /* --- 3. UART1 模式: 8位可变波特率, 允许接收 --- */
    SCON = (SCON & 0x3F) | 0x40;   // 模式1 (8位UART)
    REN = 1;                        // 允许接收

    /* --- 4. Timer1 波特率发生器 (1T, 115200) --- */
    TR1 = 0;                        // 先停止Timer1
    AUXR &= ~0x01;                  // S1 BRT 使用 Timer1
    TMOD &= ~(1 << 6);             // Timer1 = 定时器模式
    TMOD &= ~0x30;                  // Timer1 = 16位自动重装
    AUXR |= (1 << 6);              // Timer1 = 1T模式
	
	j = (MAIN_Fosc / 4) / 115200ul;
	j = 65536UL - j;
	TH1 = (u8)(j>>8);
	TL1 = (u8)j;
	TL1 = 0xE8;
	
//    TH1 = 0xFF;                     // 重装值高字节
//    TL1 = 0xF4;                     // 重装值低字节 (115200 @ 11.0592MHz)
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

    /* --- 8. 测试发送 (保留原有) --- */
	delay_ms(1000);
    SBUF = 'B'; while (!TI); TI = 0;
    SBUF = 'O'; while (!TI); TI = 0;
    SBUF = 'O'; while (!TI); TI = 0;
    SBUF = 'T'; while (!TI); TI = 0;
    SBUF = '\r'; while (!TI); TI = 0;
    SBUF = '\n'; while (!TI); TI = 0;
}

/***************  Bootloader UART ���Կ��� *****************/
//void	Boot_UART_config(void)
//{
//    /* Bootloader ԭʼ������ UART ��ʼ�� (������ʽ) */
////	P3M1 &= 0xFC;   
////	P3M0 &= 0xFC;
////		
////	SCON  = 0x50;         // 模式1 + 允许接收
////	AUXR |= 0x40;         // Timer1 1T模式 (读改写)
////	TMOD &= 0x8F;         // Timer1 16位自动重装 (读改写)
////	TH1   = 0xFF;         // ┐
////	TL1   = 0xF8;         // ┘ 重装值 (11.0592MHz → 115200)
////	TR1   = 1;            // 启动Timer1

//    SBUF = 'B'; while (!TI); TI = 0; //发送是ok的
//    SBUF = 'O'; while (!TI); TI = 0;
//    SBUF = 'O'; while (!TI); TI = 0;
//    SBUF = 'T'; while (!TI); TI = 0;
//    SBUF = '\r'; while (!TI); TI = 0;
//    SBUF = '\n'; while (!TI); TI = 0;
//}

//========================================================================
// 诊断: 打印关键寄存器值 (阻塞方式, UART_config之后调用)
//========================================================================
// 基础发送
static void UART_SendByte(u8 dat)
{
    SBUF = dat;
    while (!TI);
    TI = 0;
}

static void UART_SendString(char *s)
{
    while (*s) {
        UART_SendByte(*s++);
    }
}

// 十六进制打印
static void UART_PrintHex(u8 val)
{
    u8 hex[] = "0123456789ABCDEF";
    UART_SendByte(hex[(val >> 4) & 0x0F]);
    UART_SendByte(hex[val & 0x0F]);
}

void Dump_Registers(void)
{
    
}

void dump_regs(void)
{
    // 帧头
    SBUF = 0xAA; while(!TI); TI = 0;
    SBUF = 0x55; while(!TI); TI = 0;

    // 每个寄存器前后加标记，方便定位
    SBUF = 0x01; while(!TI); TI = 0;  // 标记：下面是SCON
    SBUF = SCON;  while(!TI); TI = 0;

    SBUF = 0x02; while(!TI); TI = 0;  // 标记：AUXR
    SBUF = AUXR;  while(!TI); TI = 0;

    SBUF = 0x03; while(!TI); TI = 0;  // 标记：TMOD
    SBUF = TMOD;  while(!TI); TI = 0;

    SBUF = 0x04; while(!TI); TI = 0;  // 标记：TH1
    SBUF = TH1;   while(!TI); TI = 0;

    SBUF = 0x05; while(!TI); TI = 0;  // 标记：TL1
    SBUF = TL1;   while(!TI); TI = 0;

    SBUF = 0x06; while(!TI); TI = 0;  // 标记：P_SW1
    SBUF = P_SW1; while(!TI); TI = 0;

    SBUF = 0x07; while(!TI); TI = 0;  // 标记：P3M1
    SBUF = P3M1;  while(!TI); TI = 0;

    SBUF = 0x08; while(!TI); TI = 0;  // 标记：P3M0
    SBUF = P3M0;  while(!TI); TI = 0;

    SBUF = 0xFF; while(!TI); TI = 0;  // 结束标记
}

//========================================================================
void TIMER_Config(void)
{
	UART_Timeout_Timer0_Config(115200ul);
}

//========================================================================
void Process_UART_Frame(u8 *buf, u8 len)
{
    u8 i;
    for (i = 0; i < len; i++) {
        TX1_write2buff(buf[i]);
    }
    TX1_write2buff('\r');
    TX1_write2buff('\n');
	COM1.RX_Cnt = 0;
}

void main(void)
{
	EAXSFR();

	GPIO_config();
	TIMER_Config();
	
	Boot_UART_config();
	UART_config();
	
	//dump_regs();
	
	EA = 1;          // 恢复中断
	
    delay_ms(100);
	printf("STC8G1K08 UART1 Test Programme!\r\n");
	delay_ms(1000);
	printf("************\n");
	delay_ms(1000);
	printf("------------\n");

	NVIC_Timer0_Init(ENABLE, Priority_2);

	P55 = 1;

	while (1)
	{
		if (uart_rx_timeout_flag)
		{
			uart_rx_timeout_flag = 0;
			protocol_process();
		}
	}
}
