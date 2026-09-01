/*---------------------------------------------------------------------*/
/* --- STC MCU Limited ------------------------------------------------*/
/* --- STC 1T Series MCU Demo Programme -------------------------------*/
/* --- Mobile: (86)13922805190 ----------------------------------------*/
/* --- Fax: 86-0513-55012956,55012947,55012969 ------------------------*/
/* --- Tel: 86-0513-55012928,55012929,55012966 ------------------------*/
/* --- Web: www.STCAI.com ---------------------------------------------*/
/* --- BBS: www.STCAIMCU.com  -----------------------------------------*/
/* --- QQ:  800003751 -------------------------------------------------*/
/* ������δ��STC�������ɵ�����½����������������ҵĿ��                  */
/*---------------------------------------------------------------------*/

#include	"config.h"
#include	"STC8G_H_GPIO.h"
#include	"STC8G_H_UART.h"
#include	"STC8G_H_Timer.h"
#include	"STC8G_H_Delay.h"
#include	"STC8G_H_NVIC.h"
#include	"STC8G_H_Switch.h"
#include	"procotol.h"

/*************	����˵��	**************


******************************************/

/*************	������ر�������	**************/


/*************	���ڳ�������	**************/

// ���ڽ�����ر���
volatile u16 uart_rx_timeout_cnt = 0;   // ���ڽ��ճ�ʱ������,��λ10us
static u16 uart_timeout_threshold = 0;         // 3.5�ַ�ʱ����ֵ,��λ10us
static bit uart_frame_complete = 0;

/*************  �ⲿ�����ͱ��ر������� *****************/


/******************* IO���ú��� *******************/
void	GPIO_config(void)
{
	GPIO_InitTypeDef	GPIO_InitStructure;		//�ṹ�����

	//GPIO_InitStructure.Pin  = GPIO_Pin_0 | GPIO_Pin_1;		//ָ��Ҫ��ʼ����IO, GPIO_Pin_0 ~ GPIO_Pin_7
	//GPIO_InitStructure.Mode = GPIO_PullUp;	//ָ��IO�ڵ����������ʽ,GPIO_PullUp,GPIO_HighZ,GPIO_OUT_OD,GPIO_OUT_PP
	//GPIO_Inilize(GPIO_P3,&GPIO_InitStructure);	//��ʼ��
	
	//GPIO_InitStructure.Pin  = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.Pin  = GPIO_Pin_5;
	GPIO_InitStructure.Mode = GPIO_OUT_PP;
	GPIO_Inilize(GPIO_P5,&GPIO_InitStructure);
	
//	GPIO_InitStructure.Pin  = GPIO_Pin_5;
//	GPIO_InitStructure.Mode = GPIO_HighZ;
//	GPIO_Inilize(GPIO_P5,&GPIO_InitStructure);
	
	//��RST����Ϊģʽѡ���,�ϵ�˲����6.5ms�ߵ�ƽ
	GPIO_InitStructure.Pin  = GPIO_Pin_4;
	GPIO_InitStructure.Mode = GPIO_HighZ;
	GPIO_Inilize(GPIO_P5,&GPIO_InitStructure);
	delay_ms(10);
}

/***************  ���ڳ�ʼ������ *****************/
void	UART_config(void)
{
	GPIO_InitTypeDef	GPIO_InitStructure;		//�ṹ�����
	COMx_InitDefine		COMx_InitStructure;					//�ṹ�����
	
	GPIO_InitStructure.Pin  = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.Mode = GPIO_PullUp;
	GPIO_Inilize(GPIO_P3,&GPIO_InitStructure);
	
	COMx_InitStructure.UART_Mode      = UART_8bit_BRTx;	//ģʽ, UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
	COMx_InitStructure.UART_BRT_Use   = BRT_Timer1;			//ѡ�����ʷ�����, BRT_Timer1, BRT_Timer2 (ע��: ����2�̶�ʹ��BRT_Timer2)
	COMx_InitStructure.UART_BaudRate  = 115200ul;			//������, �ڱ�ϵͳ��, ��ʱ��16λ�Զ���װģʽ�¿�����2400~115200
	COMx_InitStructure.UART_RxEnable  = ENABLE;				//��������, ENABLE��DISABLE
	COMx_InitStructure.BaudRateDouble = DISABLE;			//�����ʼӱ�, ENABLE��DISABLE
	
	// ����3.5�ַ�ʱ����ֵ,��λ10us, ���ڳ�ʱ�ж�
    uart_timeout_threshold = UART_Get_3_5CharTime_10us(COMx_InitStructure.UART_BaudRate);
	
	UART_Configuration(UART1, &COMx_InitStructure);		//��ʼ������1 UART1,UART2,UART3,UART4
	NVIC_UART1_Init(ENABLE,Priority_1);		//�ж�ʹ��, ENABLE/DISABLE; ���ȼ�(�ӵ͵���) Priority_0,Priority_1,Priority_2,Priority_3

	UART1_SW(UART1_SW_P30_P31);		//UART1_SW_P30_P31,UART1_SW_P36_P37,UART1_SW_P16_P17,UART1_SW_P43_P44
}

/**********************************************/

//========================================================================
// ����: TIMER_Config, ͳһ���ö�ʱ��
//========================================================================
void TIMER_Config(void)
{
    //Timer0_10us_Config();   // ���ö�ʱ��0Ϊ10us�ж�
	UART_Timeout_Timer0_Config(115200ul);
}

//========================================================================
// ����: Process_UART_Frame, ��������֡
// ˵��: �յ�ʲô�ʹ�ӡʲô
// ����: *buf - ���ݻ�����ָ��, len - ���ݳ���
// ����: none
//========================================================================
void Process_UART_Frame(u8 *buf, u8 len)
{
    u8 i;
    
    // ��ʽ1: ʹ�� printf, �ᾭ�� putchar, ���յ��� TX1_write2buff
    // printf("�յ� %d �ֽ�: ", len);
    // for (i = 0; i < len; i++) {
    //     printf("%02X ", buf[i]);   // ��ʮ�����Ƹ�ʽ��ӡ
    // }
    // printf("\r\n");
    
    // ��ʽ2: ֱ��ԭ������, �Ƽ����ڼ򵥲���
    for (i = 0; i < len; i++) {
        TX1_write2buff(buf[i]);
    }
    TX1_write2buff('\r');   // �س�
    TX1_write2buff('\n');
	COM1.RX_Cnt = 0;
}

void	Boot_UART_config(void)
{
	GPIO_InitTypeDef	GPIO_InitStructure;		//�ṹ�����?
	COMx_InitDefine		COMx_InitStructure;					//�ṹ�����?
	
	GPIO_InitStructure.Pin  = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.Mode = GPIO_PullUp;
	GPIO_Inilize(GPIO_P3,&GPIO_InitStructure);
	
	COMx_InitStructure.UART_Mode      = UART_8bit_BRTx;	//ģʽ, UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
	COMx_InitStructure.UART_BRT_Use   = BRT_Timer1;			//ѡ�����ʷ�����, BRT_Timer1, BRT_Timer2 (ע��: ����2�̶�ʹ��BRT_Timer2)
	COMx_InitStructure.UART_BaudRate  = 115200ul;			//������, �ڱ�ϵͳ��, ��ʱ��16λ�Զ���װģʽ�¿�����2400~115200
	COMx_InitStructure.UART_RxEnable  = ENABLE;				//��������, ENABLE��DISABLE
	COMx_InitStructure.BaudRateDouble = DISABLE;			//�����ʼӱ�, ENABLE��DISABLE
		
	UART_Configuration(UART1, &COMx_InitStructure);		//��ʼ������1 UART1,UART2,UART3,UART4
	NVIC_UART1_Init(ENABLE,Priority_1);		//�ж�ʹ��, ENABLE/DISABLE; ���ȼ�(�ӵ͵���) Priority_0,Priority_1,Priority_2,Priority_3

	UART1_SW(UART1_SW_P30_P31);		//UART1_SW_P30_P31,UART1_SW_P36_P37,UART1_SW_P16_P17,UART1_SW_P43_P44
}

void main(void)
{	
	EAXSFR();		/* ��չ�Ĵ�������ʹ�� */
	
	GPIO_config();
	TIMER_Config();

	UART_config();
//	Boot_UART_config();
	
	EA = 1;
	
	// �ȴ�ϵͳ�ȶ�
    delay_ms(100);
	printf("STC8G1K08 UART1 Test Programme!\r\n");	//UART1����һ���ַ���
	delay_ms(1000);
	printf("************\n");
	delay_ms(1000);
	printf("------------\n");
	
	NVIC_Timer0_Init(ENABLE, Priority_2);
	
//	Timer0_Run(1);    // ����
//	Timer0_Stop();    // ֹͣ
//	T0_Load(65536UL - (MAIN_Fosc / 100000UL));  // װ�س�ֵ

	P55 = 1;
	
	while (1)
	{
		if (uart_rx_timeout_flag) 
		{
			uart_rx_timeout_flag = 0;
			protocol_process();
		}
//		// ����Ƿ�ʱ(�յ���ʱ >= 3.5�ַ�ʱ��)
//        if (uart_rx_timeout_cnt >= uart_timeout_threshold)
//        {
//            // ��ֹ�ظ�����
//            uart_rx_timeout_cnt = 0xFFFF;
//            
//            if (COM1.RX_Cnt > 0)
//            {
//                // �յ�����, ����
//                Process_UART_Frame(RX1_Buffer, COM1.RX_Cnt);
//                COM1.RX_Cnt = 0;
//            }
//            
//            // ��λ������, ׼����һ���ж�, �ؼ�����
//            uart_rx_timeout_cnt = 0;
//        }
//        
//        // ��������...
	}
}

