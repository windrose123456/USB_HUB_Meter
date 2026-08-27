/*---------------------------------------------------------------------*/
/* --- STC MCU Limited ------------------------------------------------*/
/* --- STC 1T Series MCU Demo Programme -------------------------------*/
/* --- Mobile: (86)13922805190 ----------------------------------------*/
/* --- Fax: 86-0513-55012956,55012947,55012969 ------------------------*/
/* --- Tel: 86-0513-55012928,55012929,55012966 ------------------------*/
/* --- Web: www.STCAI.com ---------------------------------------------*/
/* --- BBS: www.STCAIMCU.com  -----------------------------------------*/
/* --- QQ:  800003751 -------------------------------------------------*/
/* 如果要在程序中使用此代码,请在程序中注明使用了STC的资料及程序            */
/*---------------------------------------------------------------------*/

#include	"config.h"
#include	"STC8G_H_GPIO.h"
#include	"STC8G_H_UART.h"
#include	"STC8G_H_Timer.h"
#include	"STC8G_H_Delay.h"
#include	"STC8G_H_NVIC.h"
#include	"STC8G_H_Switch.h"

/*************	功能说明	**************


******************************************/

/*************	本地常量声明	**************/


/*************	本地变量声明	**************/

// 串口接收相关变量
volatile u16 uart_rx_timeout_cnt = 0;   // 串口接收超时计数器（10us单位）
static u16 uart_timeout_threshold = 0;         // 3.5字符时间阈值（10us单位）
static bit uart_frame_complete = 0;

/*************  外部函数和变量声明 *****************/


/******************* IO配置函数 *******************/
void	GPIO_config(void)
{
	GPIO_InitTypeDef	GPIO_InitStructure;		//结构定义

	//GPIO_InitStructure.Pin  = GPIO_Pin_0 | GPIO_Pin_1;		//指定要初始化的IO, GPIO_Pin_0 ~ GPIO_Pin_7
	//GPIO_InitStructure.Mode = GPIO_PullUp;	//指定IO的输入或输出方式,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
	//GPIO_Inilize(GPIO_P3,&GPIO_InitStructure);	//初始化
	
	//GPIO_InitStructure.Pin  = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.Pin  = GPIO_Pin_5;
	GPIO_InitStructure.Mode = GPIO_OUT_PP;
	GPIO_Inilize(GPIO_P5,&GPIO_InitStructure);
	
//	GPIO_InitStructure.Pin  = GPIO_Pin_5;
//	GPIO_InitStructure.Mode = GPIO_HighZ;
//	GPIO_Inilize(GPIO_P5,&GPIO_InitStructure);
	
	//将RST作为模式选择引脚，上电瞬间会有6.5ms高电平
	GPIO_InitStructure.Pin  = GPIO_Pin_4;
	GPIO_InitStructure.Mode = GPIO_HighZ;
	GPIO_Inilize(GPIO_P5,&GPIO_InitStructure);
	delay_ms(10);
}

/***************  串口初始化函数 *****************/
void	UART_config(void)
{
	GPIO_InitTypeDef	GPIO_InitStructure;		//结构定义
	COMx_InitDefine		COMx_InitStructure;					//结构定义
	
	GPIO_InitStructure.Pin  = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.Mode = GPIO_PullUp;
	GPIO_Inilize(GPIO_P3,&GPIO_InitStructure);
	
	COMx_InitStructure.UART_Mode      = UART_8bit_BRTx;	//模式, UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
	COMx_InitStructure.UART_BRT_Use   = BRT_Timer1;			//选择波特率发生器, BRT_Timer1, BRT_Timer2 (注意: 串口2固定使用BRT_Timer2)
	COMx_InitStructure.UART_BaudRate  = 115200ul;			//波特率, 在本代码中，因定时器16位，波特率能设置到2400~115200
	COMx_InitStructure.UART_RxEnable  = ENABLE;				//接收允许,   ENABLE或DISABLE
	COMx_InitStructure.BaudRateDouble = DISABLE;			//波特率加倍, ENABLE或DISABLE
	
	// **计算3.5字符时间阈值（10us单位，向上取整）**
    uart_timeout_threshold = UART_Get_3_5CharTime_10us(COMx_InitStructure.UART_BaudRate);
	
	UART_Configuration(UART1, &COMx_InitStructure);		//初始化串口1 UART1,UART2,UART3,UART4
	NVIC_UART1_Init(ENABLE,Priority_1);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3

	UART1_SW(UART1_SW_P30_P31);		//UART1_SW_P30_P31,UART1_SW_P36_P37,UART1_SW_P16_P17,UART1_SW_P43_P44
}

/**********************************************/

//========================================================================
// 函数: TIMER_Config（统一配置入口）
//========================================================================
void TIMER_Config(void)
{
    //Timer0_10us_Config();   // 配置定时器0为10us中断
	UART_Timeout_Timer0_Config(115200ul);
}

//========================================================================
// 函数: Process_UART_Frame（回显模式）
// 描述: 收到什么就打印什么
// 参数: *buf - 数据缓冲区, len - 数据长度
// 返回: none
//========================================================================
void Process_UART_Frame(u8 *buf, u8 len)
{
    u8 i;
    
    // 方式1：使用 printf（会经过 putchar，最终调用 TX1_write2buff）
    // printf("收到 %d 字节: ", len);
    // for (i = 0; i < len; i++) {
    //     printf("%02X ", buf[i]);   // 以十六进制打印
    // }
    // printf("\r\n");
    
    // ★ 方式2：直接原样回显（推荐用于简单测试）
    for (i = 0; i < len; i++) {
        TX1_write2buff(buf[i]);
    }
    TX1_write2buff('\r');   // 换行
    TX1_write2buff('\n');
	COM1.RX_Cnt = 0;
}

void main(void)
{
	
	EAXSFR();		/* 扩展寄存器访问使能 */
	
	GPIO_config();
	TIMER_Config();
	UART_config();
	EA = 1;
	
	// 2. 等待系统稳定
    delay_ms(100);

	
	printf("STC8G1K08 UART1 Test Programme!\r\n");	//UART1发送一个字符串
	delay_ms(1000);
	printf("************\n");
	delay_ms(1000);
	printf("------------\n");
	
	NVIC_Timer0_Init(ENABLE, Priority_2);
	
//	Timer0_Run(1);    // 启动
//	Timer0_Stop();    // 停止
//	T0_Load(65536UL - (MAIN_Fosc / 100000UL));  // 计数重置

	P55 = 1;
	
	while (1)
	{
		if (uart_rx_timeout_flag) 
		{
			uart_rx_timeout_flag = 0;
			Process_UART_Frame(RX1_Buffer, COM1.RX_Cnt);
		}
//		// 检查是否超时（等待时间 >= 3.5字符时间）
//        if (uart_rx_timeout_cnt >= uart_timeout_threshold)
//        {
//            // 防止重复进入
//            uart_rx_timeout_cnt = 0xFFFF;
//            
//            if (COM1.RX_Cnt > 0)
//            {
//                // 收到数据，回显
//                Process_UART_Frame(RX1_Buffer, COM1.RX_Cnt);
//                COM1.RX_Cnt = 0;
//            }
//            
//            // ★ 复位计数器，准备下一次判断（关键！）
//            uart_rx_timeout_cnt = 0;
//        }
//        
//        // 其他任务...
	}
}



