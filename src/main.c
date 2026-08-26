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

本例程基于STC8H8K64U为主控芯片的实验箱8进行编写测试，STC8G、STC8H系列芯片可通用参考.

双串口全双工中断方式收发通讯程序。

通过PC向MCU发送数据, MCU收到后通过串口把收到的数据原样返回, 默认波特率：115200,N,8,1.

通过开启 STC8G_H_UART.h 头文件里面的 UART1~UART4 定义，启动不同通道的串口通信。

用定时器做波特率发生器，建议使用1T模式(除非低波特率用12T)，并选择可被波特率整除的时钟频率，以提高精度。

下载时, 选择时钟 22.1184MHz (用户可在"config.h"修改频率).

******************************************/

/*************	本地常量声明	**************/


/*************	本地变量声明	**************/
u8 slave_address = 0x01; //从机地址
u8 mode = 1; // 选择485或do模式，1 = 485,0 = do
u8 response_buff[32]; //响应缓冲区
u8 function_code;        // 功能码 (如 0x03: 读取保持寄存器)
u32 baudrate = 115200ul; //波特率

//typedef struct {
//    u8 slave_address;        // 从机地址
//    u8 function_code;        // 功能码 (如 0x03: 读取保持寄存器)
//		u16 addr;								// 查询寄存器地址
//    u8 num;           // 查询寄存器数 
//    u16 crc;                // CRC 校验码
//} ModbusRequest;

//typedef struct {
//    u8 slave_address;        // 从机地址
//    u8 function_code;        // 功能码 (如 0x03: 读取保持寄存器)
//    u8 byteCount;           // 数据字节数 (根据寄存器数量计算, 每个寄存器 2 字节)
//    u8 *_data;               // 寄存器数据
//    u16 crc;                // CRC 校验码
//} ModbusResponse;

//ModbusRequest request;
//ModbusResponse response;

/*************	本地函数声明	**************/
u16 calculate_CRC(u8 *_data, u16 length) {
    // CRC 计算函数
  u16 crc = 0xFFFF;
	u16 i = 0;
	u8 j = 0;
    for (i = 0; i < length; i++) {
        crc ^= _data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x01) {
                crc >>= 1;
                crc ^= 0xA001;
            }
						else {
                crc >>= 1;
            }
        }
    }
		crc = (crc >> 8) + (crc << 8);
    return crc;
}

u8 MB_create_response_01() {	//生成 01 指令响应
	u16 crc = 0;
	response_buff[0] = slave_address;
	response_buff[1] = function_code;
	response_buff[2] = !P55;
	crc = calculate_CRC(response_buff, 3);
	response_buff[3] = crc >> 8;
	response_buff[4] = crc;
	response_buff[5] = '\0' ;
	return 5;
}

u8 MB_create_Response_09_0A() { //生成 09、0A 指令响应
	u16 crc = 0;
	response_buff[0] = slave_address;
	response_buff[1] = function_code;
	response_buff[2] = RX1_Buffer[2];
	crc = calculate_CRC(response_buff, 3);
	response_buff[3] = crc >> 8;
	response_buff[4] = crc;
	response_buff[5] = '\0' ;
	return 5; //返回待发送字节数
}

void modified_baudrate() { //修改波特率，重新初始化串口
	COMx_InitDefine		COMx_InitStructure;					//结构定义
	switch (RX1_Buffer[2]) {
		case 0x00: 
			baudrate  = 2400ul;
			break;
		case 0x01: 
			baudrate  = 4800ul;
			break;
		case 0x02: 
			baudrate  = 9600ul;
			break;
		case 0x03: 
			baudrate  = 14400ul;
			break;
		case 0x04: 
			baudrate  = 19200ul;
			break;
		case 0x05: 
			baudrate  = 38400ul;
			break;
		case 0x06: 
			baudrate  = 56000ul; 
			break;
		case 0x07: 
			baudrate  = 57600ul; 
			break;
		case 0x08: 
			baudrate  = 115200ul; 
			break;
		default: break;
	}
	SCON = 0x00; //禁用串口，重新初始化
	
	COMx_InitStructure.UART_Mode      = UART_8bit_BRTx;	//模式, UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
	COMx_InitStructure.UART_BRT_Use   = BRT_Timer1;			//选择波特率发生器, BRT_Timer1, BRT_Timer2 (注意: 串口2固定使用BRT_Timer2)
	COMx_InitStructure.UART_BaudRate  = baudrate;			//波特率, 一般 110 ~ 115200
	COMx_InitStructure.UART_RxEnable  = ENABLE;				//接收允许,   ENABLE或DISABLE
	COMx_InitStructure.BaudRateDouble = DISABLE;			//波特率加倍, ENABLE或DISABLE
	UART_Configuration(UART1, &COMx_InitStructure);		//初始化串口1 UART1,UART2,UART3,UART4
	NVIC_UART1_Init(ENABLE,Priority_1);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3

	UART1_SW(UART1_SW_P36_P37);		//UART1_SW_P30_P31,UART1_SW_P36_P37,UART1_SW_P16_P17,UART1_SW_P43_P44
}

u8 MB_Parse_Data() //解析modbus请求
{
	u8 len = 0;
	u8 i = 0;
	u8 cmd_slave_address = 0x00;
	u16 request_crc = 0;
	request_crc = (u16)(RX1_Buffer[COM1.RX_Cnt - 2] << 8) + RX1_Buffer[COM1.RX_Cnt - 1];
	
	if (request_crc == calculate_CRC(RX1_Buffer, COM1.RX_Cnt - 2)) {
		cmd_slave_address = RX1_Buffer[0];	
		
		if (cmd_slave_address == slave_address) {
			function_code = RX1_Buffer[1];
			switch (function_code) {
				case 0x01: 
					len = MB_create_response_01(); 
				  break;
				case 0x09: 
				  len = MB_create_Response_09_0A();
					for (i = 0; i < len; i++) {
						TX1_write2buff(response_buff[i]);
					}
					len = 0;
					modified_baudrate();
				  break;
				case 0x0A: 
					len = MB_create_Response_09_0A();
				  slave_address = RX1_Buffer[2];
				  break;
				default: break;
			}
		}
	}
	return len;
}
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
	COMx_InitDefine		COMx_InitStructure;					//结构定义

	COMx_InitStructure.UART_Mode      = UART_8bit_BRTx;	//模式, UART_ShiftRight,UART_8bit_BRTx,UART_9bit,UART_9bit_BRTx
	COMx_InitStructure.UART_BRT_Use   = BRT_Timer1;			//选择波特率发生器, BRT_Timer1, BRT_Timer2 (注意: 串口2固定使用BRT_Timer2)
	COMx_InitStructure.UART_BaudRate  = baudrate;			//波特率, 一般 110 ~ 115200
	COMx_InitStructure.UART_RxEnable  = ENABLE;				//接收允许,   ENABLE或DISABLE
	COMx_InitStructure.BaudRateDouble = DISABLE;			//波特率加倍, ENABLE或DISABLE
	UART_Configuration(UART1, &COMx_InitStructure);		//初始化串口1 UART1,UART2,UART3,UART4
	NVIC_UART1_Init(ENABLE,Priority_1);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3

	UART1_SW(UART1_SW_P30_P31);		//UART1_SW_P30_P31,UART1_SW_P36_P37,UART1_SW_P16_P17,UART1_SW_P43_P44
}

/**********************************************/
void main(void)
{
	//01 01 C1 E0
	//01 09 08 27 96
	//01 0A 02 A7 61
	
	u8	i = 0;
	u8 response_len = 0; 
	
	EAXSFR();		/* 扩展寄存器访问使能 */
	
	GPIO_config();
	UART_config();
	EA = 1;
	
	printf("STC8H8K64U UART1 Test Programme!\r\n");	//UART1发送一个字符串
	
	UART1_SW(UART1_SW_P36_P37);
	
	//printf("start\n");
	
	while (1)
	{
//		printf("************\n");
//		printf("i = %x **%u\n", i, i);
//		printf("%02X\n", RSTCFG);
		
	}
}



