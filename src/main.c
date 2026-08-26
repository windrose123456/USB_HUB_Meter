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
#include	"STC8G_H_Delay.h"
#include	"STC8G_H_NVIC.h"
#include	"STC8G_H_Switch.h"

/*************	功能说明	**************


******************************************/

/*************	本地常量声明	**************/


/*************	本地变量声明	**************/


/*************  外部函数和变量声明 *****************/


/******************* IO配置函数 *******************/
void	GPIO_config(void)
{
	GPIO_InitTypeDef	GPIO_InitStructure;		//结构定义

	//GPIO_InitStructure.Pin  = GPIO_Pin_0 | GPIO_Pin_1;		//指定要初始化的IO, GPIO_Pin_0 ~ GPIO_Pin_7
	//GPIO_InitStructure.Mode = GPIO_PullUp;	//指定IO的输入或输出方式,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
	//GPIO_Inilize(GPIO_P3,&GPIO_InitStructure);	//初始化
	
	//GPIO_InitStructure.Pin  = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.Pin  = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.Mode = GPIO_PullUp;
	GPIO_Inilize(GPIO_P3,&GPIO_InitStructure);
	
	GPIO_InitStructure.Pin  = GPIO_Pin_5;
	GPIO_InitStructure.Mode = GPIO_HighZ;
	GPIO_Inilize(GPIO_P5,&GPIO_InitStructure);
	
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
	COMx_InitStructure.UART_BaudRate  = 115200ul;			//波特率, 一般 110 ~ 115200
	COMx_InitStructure.UART_RxEnable  = ENABLE;				//接收允许,   ENABLE或DISABLE
	COMx_InitStructure.BaudRateDouble = DISABLE;			//波特率加倍, ENABLE或DISABLE
	UART_Configuration(UART1, &COMx_InitStructure);		//初始化串口1 UART1,UART2,UART3,UART4
	NVIC_UART1_Init(ENABLE,Priority_1);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3

	UART1_SW(UART1_SW_P30_P31);		//UART1_SW_P30_P31,UART1_SW_P36_P37,UART1_SW_P16_P17,UART1_SW_P43_P44
}

/**********************************************/
void main(void)
{
	
	EAXSFR();		/* 扩展寄存器访问使能 */
	
	//GPIO_config();
	UART_config();
	EA = 1;
	
	printf("STC8G1K08 UART1 Test Programme!\r\n");	//UART1发送一个字符串
	delay_ms(1000);
	printf("************\n");
	delay_ms(1000);
	printf("------------\n");
	
	while (1)
	{
//		SBUF = 'A';
//		while(!TI);
//		TI = 0;
		printf("************\n");
		delay_ms(1000);
	}
}



