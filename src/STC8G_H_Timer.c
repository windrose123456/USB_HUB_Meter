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

#include	"STC8G_H_Timer.h"

volatile u16 SysTick_10us = 0;   // 10us递增计数器
volatile u16 SysTick_1ms = 0;    // 1ms递增计数器
volatile bit uart_rx_timeout_flag = 0;
volatile u16 timer0_reload_val = 0;

//========================================================================
// 函数: u8	Timer_Inilize(u8 TIM, TIM_InitTypeDef *TIMx)
// 描述: 定时器初始化程序.
// 参数: TIMx: 结构参数,请参考timer.h里的定义.
// 返回: 成功返回 SUCCESS, 错误返回 FAIL.
// 版本: V1.0, 2012-10-22
//========================================================================
u8	Timer_Inilize(u8 TIM, TIM_InitTypeDef *TIMx)
{
	if(TIM == Timer0)
	{
		Timer0_Stop();		//停止计数
		if(TIMx->TIM_Mode > TIM_16BitAutoReloadNoMask)	return FAIL;	//错误
		TMOD = (TMOD & ~0x03) | TIMx->TIM_Mode;	//工作模式,0: 16位自动重装, 1: 16位定时/计数, 2: 8位自动重装, 3: 不可屏蔽16位自动重装
		if(TIMx->TIM_ClkSource >  TIM_CLOCK_Ext)	return FAIL;
		Timer0_CLK_Select(TIMx->TIM_ClkSource);	//对外计数或分频, 定时12T/1T
		Timer0_CLK_Output(TIMx->TIM_ClkOut);		//输出时钟使能
		T0_Load(TIMx->TIM_Value);
		Timer0_Run(TIMx->TIM_Run);
		return	SUCCESS;		//成功
	}

	if(TIM == Timer1)
	{
		Timer1_Stop();		//停止计数
		if(TIMx->TIM_Mode > TIM_16BitAutoReloadNoMask)	return FAIL;	//错误
		TMOD = (TMOD & ~0x30) | (TIMx->TIM_Mode << 4);	//工作模式,0: 16位自动重装, 1: 16位定时/计数, 2: 8位自动重装, 3: 停止工作
		if(TIMx->TIM_ClkSource >  TIM_CLOCK_Ext)	return FAIL;
		Timer1_CLK_Select(TIMx->TIM_ClkSource);	//对外计数或分频, 定时12T/1T
		Timer1_CLK_Output(TIMx->TIM_ClkOut);		//输出时钟使能
		T1_Load(TIMx->TIM_Value);
		Timer1_Run(TIMx->TIM_Run);
		return	SUCCESS;		//成功
	}

	if(TIM == Timer2)		//Timer2,固定为16位自动重装, 中断无优先级
	{
		Timer2_Stop();	//停止计数
		Timer2_CLK_Select(TIMx->TIM_ClkSource);	//对外计数或分频, 定时12T/1T
		Timer2_CLK_Output(TIMx->TIM_ClkOut);		//输出时钟使能

		T2_Load(TIMx->TIM_Value);
        TM2PS = TIMx->TIM_PS;
		Timer2_Run(TIMx->TIM_Run);
		return	SUCCESS;		//成功
	}

	if(TIM == Timer3)		//Timer3,固定为16位自动重装, 中断无优先级
	{
		Timer3_Stop();	//停止计数
		if(TIMx->TIM_ClkSource >  TIM_CLOCK_Ext)	return FAIL;
		Timer3_CLK_Select(TIMx->TIM_ClkSource);	//对外计数或分频, 定时12T/1T
		Timer3_CLK_Output(TIMx->TIM_ClkOut);		//输出时钟使能

		T3_Load(TIMx->TIM_Value);
        TM3PS = TIMx->TIM_PS;
		Timer3_Run(TIMx->TIM_Run);
		return	SUCCESS;		//成功
	}

	if(TIM == Timer4)		//Timer3,固定为16位自动重装, 中断无优先级
	{
		Timer4_Stop();	//停止计数
		if(TIMx->TIM_ClkSource >  TIM_CLOCK_Ext)	return FAIL;
		Timer4_CLK_Select(TIMx->TIM_ClkSource);	//对外计数或分频, 定时12T/1T
		Timer4_CLK_Output(TIMx->TIM_ClkOut);		//输出时钟使能

		T4_Load(TIMx->TIM_Value);
        TM4PS = TIMx->TIM_PS;
		Timer4_Run(TIMx->TIM_Run);
		return	SUCCESS;		//成功
	}
	return FAIL;	//错误
}

//========================================================================
// 函数: void Timer0_10us_Config(void)
// 描述: 配置定时器0为10us中断，用于超时计时
// 参数: none
// 返回: none
// 版本: V1.0, 2026-08-27
//========================================================================
void Timer0_10us_Config(void)
{
    TIM_InitTypeDef TIM_InitStructure;
    
    TIM_InitStructure.TIM_Mode      = TIM_16BitAutoReload;   // 16位自动重载
    TIM_InitStructure.TIM_ClkSource = TIM_CLOCK_1T;           // 1T模式
    TIM_InitStructure.TIM_ClkOut    = DISABLE;
    TIM_InitStructure.TIM_Value     = 65536UL - (MAIN_Fosc / 100000UL);  // 10us中断一次
    TIM_InitStructure.TIM_PS        = 0;
    TIM_InitStructure.TIM_Run       = DISABLE;
    
    Timer_Inilize(Timer0, &TIM_InitStructure);
    
    ET0 = 1;    // 使能定时器0中断
}

////========================================================================
//// 函数: void Timer0_Start(void)
//// 描述: 启动定时器0
//// 参数: none
//// 返回: none
////========================================================================
//void Timer0_Start(void)
//{
//    TR0 = 1;        // 启动定时器0
//    ET0 = 1;        // 使能定时器0中断
//}

////========================================================================
//// 函数: void Timer0_Stop(void)
//// 描述: 停止定时器0
//// 参数: none
//// 返回: none
////========================================================================
//void Timer0_Stop(void)
//{
//    TR0 = 0;        // 停止定时器0
//    ET0 = 0;        // 关闭定时器0中断
//}

////========================================================================
//// 函数: void Timer0_Reset(void)
//// 描述: 定时器0计数值置0（重装初值）
//// 参数: none
//// 返回: none
////========================================================================
//void Timer0_Reset(void)
//{
//    TH0 = 0;
//    TL0 = 0;
//}

//========================================================================
// 函数: u16 UART_Get_3_5CharTime_10us(u32 baudrate)
// 描述: 计算指定波特率下的3.5字符时间，以10us为单位，向上取整
// 参数: baudrate: 波特率（如 115200, 9600 等）
// 返回: 3.5字符时间（10us的整数倍，向上取整）
// 版本: V1.0, 2026-08-27
//========================================================================
u16 UART_Get_3_5CharTime_10us(u32 baudrate)
{
    // 3.5字符时间 = 35 / 波特率（秒）
    // 转换为微秒：35 * 1000000 / 波特率
    u32 us_time = (35UL * 1000000UL) / baudrate;
    
    // 转换为10us单位，向上取整
    // (us_time + 9) / 10 即为向上取整到10us
    u32 cnt_10us = (us_time + 9) / 10;
    
    // 额外增加1个周期作为安全余量，防止临界情况误判
    cnt_10us += 1;
    
    // 返回u16范围
    return (u16)cnt_10us;
}

void UART_Timeout_Timer0_Config(u32 baudrate)
{
    // 3.5字符时间的timer ticks = 3.5 × 11bits × MAIN_Fosc / baudrate
    // = 38.5 × MAIN_Fosc / baudrate
    // 为避免浮点，用 385 × MAIN_Fosc / (baudrate × 10)
    u32 timer_ticks = (385UL * MAIN_Fosc) / (baudrate * 10UL);
    timer0_reload_val = 65536UL - (u16)timer_ticks;
    
    Timer0_Stop();
    TMOD = (TMOD & ~0x03) | 0x01;  // ★ Mode 1: 16位非自动重装
    Timer0_CLK_Select(1);           // 1T模式
    Timer0_CLK_Output(DISABLE);
    T0_Load(timer0_reload_val);
    ET0 = 1;                        // 使能Timer0中断
    TR0 = 0;                        // 先不启动
}