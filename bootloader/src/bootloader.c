/************************************************************
 * bootloader.c - STC8G1K08A IAP Bootloader
 * Target address: 0x1C00
 * Minimal test version: only sends heartbeat '2E'
 ************************************************************/
#include "config.h"
#include "STC8G_H_UART.h"
#include <intrins.h>

typedef struct
{
	u8	Mode;		//IOģʽ,  		GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
	u8	Pin;		//Ҫ���õĶ˿�	
} GPIO_InitTypeDef;

#define	GPIO_Pin_0		0x01	//IO���� Px.0
#define	GPIO_Pin_1		0x02	//IO���� Px.1

#define	GPIO_PullUp		0	//����׼˫���
#define	GPIO_HighZ		1	//��������
#define	GPIO_OUT_OD		2	//��©���
#define	GPIO_OUT_PP		3	//�������

#define	GPIO_P3			3

#define	UART1_SW_P30_P31	0

#define  UART1_SW(Pin)				P_SW1 = (P_SW1 & 0x3F) | (Pin << 6)

u8	GPIO_Inilize(u8 GPIO, GPIO_InitTypeDef *GPIOx)
{
	if(GPIOx->Mode > GPIO_OUT_PP)	return FAIL;	//����

	if(GPIO == GPIO_P3)
	{
		if(GPIOx->Mode == GPIO_PullUp)		P3M1 &= ~GPIOx->Pin,	P3M0 &= ~GPIOx->Pin;	 //����׼˫���
		if(GPIOx->Mode == GPIO_HighZ)			P3M1 |=  GPIOx->Pin,	P3M0 &= ~GPIOx->Pin;	 //��������
		if(GPIOx->Mode == GPIO_OUT_OD)		P3M1 |=  GPIOx->Pin,	P3M0 |=  GPIOx->Pin;	 //��©���
		if(GPIOx->Mode == GPIO_OUT_PP)		P3M1 &= ~GPIOx->Pin,	P3M0 |=  GPIOx->Pin;	 //�������
	}

	return SUCCESS;	//�ɹ�
}

//#define	Priority_0			0	//中断优先级为 0 级（最低级）
//#define	Priority_1			1	//中断优先级为 1 级（较低级）
//#define	Priority_2			2	//中断优先级为 2 级（较高级）
#define	Priority_3			3	//中断优先级为 3 级（最高级）
//#define		PLVDH	0x40
//#define		PADCH	0x20
#define		PSH		0x10
//#define		PT1H	0x08
//#define		PX1H	0x04
//#define		PT0H	0x02
//#define		PX0H	0x01

#define		UART1_Interrupt(n)	(n==0?(ES = 0):(ES = 1))			/* UART1中断使能 */
//串口1中断优先级控制
#define 	UART1_Priority(n)			do{if(n == 0) IPH &= ~PSH, PS = 0; \
																if(n == 1) IPH &= ~PSH, PS = 1; \
																if(n == 2) IPH |= PSH, PS = 0; \
																if(n == 3) IPH |= PSH, PS = 1; \
															}while(0)

u8 NVIC_UART1_Init(u8 State, u8 Priority)
{
//	if(State > ENABLE) return FAIL;
//	if(Priority > Priority_3) return FAIL;
	UART1_Interrupt(State);
	UART1_Priority(Priority);
	return SUCCESS;
}

/* ---- delay ---- */
static void boot_delay_ms(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 120; j++);
}

/* ---- UART init (using STC8G library) ---- */
static void uart_init(void) {
	//GPIO_InitTypeDef	GPIO_InitStructure;		//锟结构锟斤拷锟斤拷锟?
	COMx_InitDefine		COMx_InitStructure;					//锟结构锟斤拷锟斤拷锟?
	
//	GPIO_InitStructure.Pin  = GPIO_Pin_0 | GPIO_Pin_1;
//	GPIO_InitStructure.Mode = GPIO_PullUp;
//	GPIO_Inilize(GPIO_P3,&GPIO_InitStructure);
	
	COMx_InitStructure.UART_Mode      = UART_8bit_BRTx;	//模式, UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
	COMx_InitStructure.UART_BRT_Use   = BRT_Timer1;			//选锟斤拷锟斤拷锟绞凤拷锟斤拷锟斤拷, BRT_Timer1, BRT_Timer2 (注锟斤拷: 锟斤拷锟斤拷2锟教讹拷使锟斤拷BRT_Timer2)
	COMx_InitStructure.UART_BaudRate  = 115200ul;			//锟斤拷锟斤拷锟斤拷, 锟节憋拷系统锟斤拷, 锟斤拷时锟斤拷16位锟皆讹拷锟斤拷装模式锟铰匡拷锟斤拷锟斤拷2400~115200
	COMx_InitStructure.UART_RxEnable  = ENABLE;				//锟斤拷锟斤拷锟斤拷锟斤拷, ENABLE锟斤拷DISABLE
	COMx_InitStructure.BaudRateDouble = DISABLE;			//锟斤拷锟斤拷锟绞加憋拷, ENABLE锟斤拷DISABLE
		
	UART_Configuration(UART1, &COMx_InitStructure);		//锟斤拷始锟斤拷锟斤拷锟斤拷1 UART1,UART2,UART3,UART4
	NVIC_UART1_Init(ENABLE,Priority_1);		//锟叫讹拷使锟斤拷, ENABLE/DISABLE; 锟斤拷锟饺硷拷(锟接低碉拷锟斤拷) Priority_0,Priority_1,Priority_2,Priority_3

	UART1_SW(UART1_SW_P30_P31);	
}

/* ---- UART send (direct SBUF, no interrupt needed) ---- */
static void uart_send(unsigned char c) {
    SBUF = c; while (!TI); TI = 0;
}

/* ---- main ---- */
void main(void) {
    unsigned int cnt = 0;

    //uart_init();
    boot_delay_ms(100);

    /* Send 'B' once to confirm bootloader started */
    TX1_write2buff('1');
	TX1_write2buff('1');
	//uart_send('B');
	TX1_write2buff('2');

    while (1) {
        boot_delay_ms(100);
        cnt++;
        if (cnt >= 5) {     /* ~500ms */
            cnt = 0;
			TX1_write2buff('3');
            //uart_send(0x2E);    /* heartbeat '.' */
        }
    }
}
