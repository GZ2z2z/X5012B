/****************************************Copyright (c)**************************************************
**                               	杭州鑫高科技有限公司
**                                   
**                                 
** 文   件   名: delay.c
** 最后修改日期: 2014/4/25 21:03:22
** 描        述: 日期处理函数
** 版	     本: V1.0
** 主  控  芯 片: 	 晶振频率:	
** IDE:MDK4.12
SYSCLK=72000000
HCLK=72000000
PCLK1=36000000
PCLK2=72000000
ADCCLK=36000000
**********************************************************************************************************/
#include "stm32f4xx_hal.h" 
#include "delay.h" 




/**********************************************************************
Function Name	:void us_delay(uint8_t ud)
Description				:us级别的函数
**********************************************************************/
void delay_us(uint16_t ud)
{
	uint16_t i;
	while(ud--)
	{
		i=8;
		while(i--);	
	}	
}


/**********************************************************************
Function Name	:void delay_ms(uint8_t md)
Description				:ms级别的函数
**********************************************************************/
void delay_ms(uint16_t md)
{		    
	while(md--)
	{
		delay_us(800);
	}	 
}

#define		TIM_DELAY			TIM2
#define		TIM_DELAY_Periph	RCC_APB1Periph_TIM2


// /**********************************************************************
// Function Name	:void delay_init(void)
// Description				:硬件精确延时的初始化使用定时器2
// **********************************************************************/
// void delay_init_t2(void)
// {  
// 	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;   
	 
// 	RCC_APB1PeriphClockCmd(TIM_DELAY_Periph, ENABLE); 
//     TIM_TimeBaseStructure.TIM_Prescaler=(72000000/10000l-1); 		//CK_CNT的频率为10000Hz
// 	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Down;
//     TIM_TimeBaseStructure.TIM_Period=1;
// 	TIM_TimeBaseStructure.TIM_ClockDivision=0;  
// 	TIM_TimeBaseInit(TIM_DELAY,&TIM_TimeBaseStructure);   
	
// 	TIM_ClearFlag(TIM_DELAY,TIM_FLAG_Update); 
// }
  

// /**********************************************************************
// Function Name	:void delay_t2(uint32_t t)
// Description				: TIM2的延时函数，单位0.1mS
// **********************************************************************/
// void delay_t2(uint32_t t)
// { 
// 	if(t)
// 	t=t-1;	
// 	TIM_SetCounter(TIM_DELAY,t);  
// 	TIM_ClearFlag(TIM_DELAY,TIM_FLAG_Update);  
// 	TIM_Cmd(TIM_DELAY, ENABLE);		//启动定时器 
// 	while(TIM_GetFlagStatus(TIM_DELAY,TIM_FLAG_Update)==RESET)
// 	{
// 		;
// 	}
// 	TIM_Cmd(TIM_DELAY, DISABLE);		//启动定时器 
// }


// ///**********************************************************************
// //Function Name	:void wait_systick(void)
// //Description				: 等待定时器1的systick
// //**********************************************************************/  
// //void wait_systick(void)
// //{
// //	 ;
// //} 
// //
// //
// //
// ///**********************************************************************
// //Function Name	:void systick_init(uint32_t ms)
// //Description				: 等待定时器4的systick,设置单位为mS
// //**********************************************************************/ 
// //void systick_init(uint32_t ms)
// //{
// //	
// //}


// /**********************************************************************
// Function Name	:void timeout_init(uint32_t ms)
// Description				:使用定时器4，超时初始化设置
// **********************************************************************/
// void timeout_init(void)
// {
// 	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;   
	 
// 	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); 
//     TIM_TimeBaseStructure.TIM_Prescaler=(72000000/10000l-1); 		//CK_CNT的频率为10000Hz
// 	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Down;
//     TIM_TimeBaseStructure.TIM_Period=1;
// 	TIM_TimeBaseStructure.TIM_ClockDivision=0;  
// 	TIM_TimeBaseInit(TIM4,&TIM_TimeBaseStructure);    
// 	TIM_ClearFlag(TIM4,TIM_FLAG_Update); 
// }

// /**********************************************************************
// Function Name	:void timeout_set(uint32_t t)
// Description				:超时设置，单位ms
// **********************************************************************/
// void timeout_set(uint32_t t)
// {
// 	if(t)
// 	t=t-1;	
// 	t=t*10;
// 	TIM_SetCounter(TIM4,t);  
// 	TIM_ClearFlag(TIM4,TIM_FLAG_Update);  
// 	TIM_Cmd(TIM4, ENABLE);		//启动定时器  
// }

// /**********************************************************************
// Function Name	:TestStatus timeout_query(void)
// Description				:超时查询，如果返回FAILED，表示超时
// **********************************************************************/
// TIMEOUT_Typedef timeout_query(void)
// { 
// 	if(TIM_GetFlagStatus(TIM4,TIM_FLAG_Update)!=RESET)
// 	{
// 		TIM_Cmd(TIM4, DISABLE);		//启动定时器  
// 		return TIMEOUT;
// 	}
// 	return TIMEIN;	
// }

 
   
   
   
   


/******************* (C) COPYRIGHT 2012 XinGao Tech *****END OF FILE****/
