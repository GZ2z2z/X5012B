/*
*********************************************************************************************************
*
*	模块名称 : 定时器模块
*	文件名称 : bsp_timer.c
*	版    本 : V1.3
*	说    明 : 配置systick定时器作为系统滴答定时器。缺省定时周期为10ms。
*
*				实现了多个软件定时器供主程序使用(精度10ms)， 可以通过修改 TMR_COUNT 增减定时器个数
*				实现了ms级别延迟函数（精度10ms） 和us级延迟函数
*				实现了系统运行时间函数（10ms单位）
*				程序修改自安富莱电子 www.armfly.com
*
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2013-02-01 armfly  正式发布
*		V1.1    2013-06-21 armfly  增加us级延迟函数 bsp_DelayUS
*   	V1.2	2015/6/23  chengs  定时器改为10mS,程序可以移植到STM32F1xx、STM32F4XX芯片，增加US级计时函数	
*		V1.3	2015-9-17  sy	   g_iRunTime改为 uint32_t
*		V1.4	2015-9-22  sy	   使用NVIC_SetPriority();修改滴答定时器优先级为最低优先级。
*	Copyright (C), 2004-2015, 杭州鑫高科技 www.hzxingao.com
*
*********************************************************************************************************
*/

#include "bsp.h"


#define		Systick_Int_Enable()			(SysTick->CTRL)|=(1<<SysTick_CTRL_TICKINT_Pos)
#define		Systick_Int_Disable()			(SysTick->CTRL)&=~(1<<SysTick_CTRL_TICKINT_Pos)


/* 定于软件定时器结构体变量 */
static SOFT_TMR s_tTmr[TMR_COUNT];

/*
	全局运行时间，单位1ms
	最长可以表示 49天，如果你的产品连续运行时间超过这个数，则必须考虑溢出问题
*/
static __IO uint32_t g_RunTime = 0;
static __IO uint32_t g_InsideRunTime = 0;

/*
*********************************************************************************************************
*	函 数 名: bsp_InitTimer
*	功能说明: 配置systick中断，并初始化软件定时器变量
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitTimer(void)
{
	uint8_t i;

	/* 清零所有的软件定时器 */
	for (i = 0; i < TMR_COUNT; i++)
	{
		s_tTmr[i].Count = 0;
		s_tTmr[i].PreLoad = 0;
		s_tTmr[i].Flag = 0;
		s_tTmr[i].Mode = TMR_ONCE_MODE;	/* 缺省是1次性工作模式 */
	}

	ECHO(ECHO_DEBUG,"[BSP] 系统时钟频率：%d MHz", BSP_CPU_ClkFreq()/1000000);	
	ECHO(ECHO_DEBUG,"[BSP] TIMER 初始化 .......... OK");
}

/*
*********************************************************************************************************
*    函 数 名: bsp_DelayUS
*    功能说明: us级延迟。 必须在systick定时器启动后才能调用此函数。
*    形    参:  n : 延迟长度，单位1 us
*    返 回 值: 无
*********************************************************************************************************
*/
void bsp_DelayUS(uint32_t n)
{
    uint32_t told = SysTick->VAL;
    uint32_t reload = SysTick->LOAD;           
    uint32_t ticks = n * (SystemCoreClock / 1000000);	 /* 需要的节拍数 */ 
	uint32_t tnow;
    uint32_t tcnt = 0;
    while (1)
    {
        tnow = SysTick->VAL;    
        if (tnow != told)
        {    
            /* SYSTICK是一个递减的计数器 */    
            if (tnow < told)
            {
                tcnt += told - tnow;    
            }
            /* 重新装载递减 */
            else
            {
                tcnt += reload - tnow + told;    
            }        
            told = tnow;

            /* 时间超过/等于要延迟的时间,则退出 */
            if (tcnt >= ticks)
            {
            	break;
            }
        }  
    }
} 

/*
*********************************************************************************************************
*	函 数 名: bsp_DelayMS
*	功能说明: 1ms级延迟，延迟精度为正负1ms
*	形    参:  n : 延迟长度，单位1ms。 n 应大于2
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_DelayMS(uint32_t n)
{
	while(n)
	{
		bsp_DelayUS(1000);
		n--;
	}
}


/*
*********************************************************************************************************
*	函 数 名: bsp_StartTimer
*	功能说明: 启动一个定时器，并设置定时周期。
*	形    参:  	_id     : 定时器ID，值域【0,TMR_COUNT-1】。用户必须自行维护定时器ID，以避免定时器ID冲突。
*				_period : 定时周期，单位10ms
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_StartTimer(uint8_t _id, uint32_t _period)
{
	if (_id >= TMR_COUNT)
	{
		/* 打印出错的源代码文件名、函数名称 */
		ECHO(ECHO_DEBUG, "[BSP][TIMER] Error: file %s, function %s()", __FILE__, __FUNCTION__);
		
		while(1); /* 参数异常，死机等待看门狗复位 */
	}

	Systick_Int_Disable();  			/* 关中断 */

	s_tTmr[_id].Count = _period;		/* 实时计数器初值 */
	s_tTmr[_id].PreLoad = _period;		/* 计数器自动重装值，仅自动模式起作用 */
	s_tTmr[_id].Flag = 0;				/* 定时时间到标志 */
	s_tTmr[_id].Mode = TMR_ONCE_MODE;	/* 1次性工作模式 */

	Systick_Int_Enable();  				/* 开中断 */
}

/*
*********************************************************************************************************
*	函 数 名: bsp_StartAutoTimer
*	功能说明: 启动一个自动定时器，并设置定时周期。
*	形    参:  	_id     : 定时器ID，值域【0,TMR_COUNT-1】。用户必须自行维护定时器ID，以避免定时器ID冲突。
*				_period : 定时周期，单位10ms
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_StartAutoTimer(uint8_t _id, uint32_t _period)
{
	if (_id >= TMR_COUNT)
	{
		/* 打印出错的源代码文件名、函数名称 */
		ECHO(ECHO_DEBUG,"[BSP][TIMER] Error: file %s, function %s()", __FILE__, __FUNCTION__);
		
		while(1); /* 参数异常，死机等待看门狗复位 */
	}

	Systick_Int_Disable();  			/* 关中断 */

	s_tTmr[_id].Count = _period;		/* 实时计数器初值 */
	s_tTmr[_id].PreLoad = _period;		/* 计数器自动重装值，仅自动模式起作用 */
	s_tTmr[_id].Flag = 0;				/* 定时时间到标志 */
	s_tTmr[_id].Mode = TMR_AUTO_MODE;	/* 自动工作模式 */

	Systick_Int_Enable();  				/* 开中断 */
}

/*
*********************************************************************************************************
*	函 数 名: bsp_StopTimer
*	功能说明: 停止一个定时器
*	形    参:  	_id     : 定时器ID，值域【0,TMR_COUNT-1】。用户必须自行维护定时器ID，以避免定时器ID冲突。
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_StopTimer(uint8_t _id)
{
	if (_id >= TMR_COUNT)
	{
		/* 打印出错的源代码文件名、函数名称 */
		ECHO(ECHO_DEBUG,"[BSP][TIMER] Error: file %s, function %s()", __FILE__, __FUNCTION__);
		
		while(1); /* 参数异常，死机等待看门狗复位 */
	}

	Systick_Int_Disable();  			/* 关中断 */

	s_tTmr[_id].Count = 0;				/* 实时计数器初值 */
	s_tTmr[_id].Flag = 0;				/* 定时时间到标志 */
	s_tTmr[_id].Mode = TMR_ONCE_MODE;	/* 1次性工作模式 */

	Systick_Int_Enable();  				/* 开中断 */
}

/*
*********************************************************************************************************
*	函 数 名: bsp_CheckTimer
*	功能说明: 检测定时器是否超时
*	形    参:  	_id     : 定时器ID，值域【0,TMR_COUNT-1】。用户必须自行维护定时器ID，以避免定时器ID冲突。
*				_period : 定时周期，单位1ms
*	返 回 值: 返回 0 表示定时未到， 1表示定时到
*********************************************************************************************************
*/
uint8_t bsp_CheckTimer(uint8_t _id)
{
	if (_id >= TMR_COUNT)
	{
		return 0;
	}

	if (s_tTmr[_id].Flag == 1)
	{
		s_tTmr[_id].Flag = 0;
		return 1;
	}
	else
	{
		return 0;
	}
}

/*
*********************************************************************************************************
*	函 数 名: bsp_GetRunTime
*	功能说明: 获取CPU运行时间，单位1ms。最长可以表示 49天，如果你的产品连续运行时间超过这个数，则必须考虑溢出问题
*	形    参:  无
*	返 回 值: CPU运行时间，单位1ms
*********************************************************************************************************
*/
uint32_t bsp_GetRunTime(void)
{
	return g_RunTime;
}
 
static SOFT_TMR_US	TmrUs[TMR_COUNT_US];

/*
*********************************************************************************************************
*	函 数 名: bsp_StartTimeUS
*	功能说明: US级计时开始
*	形    参:  计时器ID,0~3
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_StartTimeUS(uint8_t _id)
{ 
	TmrUs[_id].startRunTime=g_InsideRunTime;
	TmrUs[_id].startTick=SysTick->VAL;
}
  
  
/*
*********************************************************************************************************
*	函 数 名: bsp_GetTimeUS
*	功能说明: 精确到US计时函数，最长可以计时2147秒,精度1uS。
*	形    参: 计时器ID,0~3
*	返 回 值: 返回自执行 bsp_StartTimerUS开始到本函数执行所运行的时间，单位Us
*********************************************************************************************************
*/ 
uint32_t bsp_GetTimeUS(uint8_t _id)
{
	uint32_t endTick=SysTick->VAL;
	uint32_t endRunTime=g_InsideRunTime;
	uint32_t load=SysTick->LOAD;  
	uint32_t Us=((endRunTime-TmrUs[_id].startRunTime)*(load+1)+TmrUs[_id].startTick-endTick)/(SystemCoreClock / 1000000L); 	 
	return Us;
}
 

/*
*********************************************************************************************************
*	函 数 名: SysTick_Handler
*	功能说明: 系统嘀嗒定时器中断服务程序。启动文件中引用了该函数。
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
void SysTick_Handler(void)
{
	/* 每隔1ms，对软件定时器的计数器进行减一操作 */
	for (uint8_t i = 0; i < TMR_COUNT; i++) {
		if (s_tTmr[i].Count > 0) {
			/* 如果定时器变量减到1则设置定时器到达标志 */
			if (--s_tTmr[i].Count == 0) {
				s_tTmr[i].Flag = 1;
				/* 如果是自动模式，则自动重装计数器 */
				if(s_tTmr[i].Mode == TMR_AUTO_MODE) {
					s_tTmr[i].Count = s_tTmr[i].PreLoad;
				}
			}
		}
	}

	g_InsideRunTime++;

#if (TIME_BASE_UNIT == TIME_UNIT_MS)
{
	g_RunTime++;
}
#elif (TIME_BASE_UNIT == TIME_UNIT_US)
{
	static __IO uint32_t cnt = 0;
	if (++cnt >= 10) {
		cnt = 0;
		/* 全局运行时间每1ms增1 */
		g_RunTime++;
	}
}
#endif
}

uint32_t HAL_GetTick(void)
{
	return g_RunTime;
}

extern uint32_t uwTickPrio;
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
	/* Configure the SysTick to have interrupt in 0.1ms time basis*/
	if (HAL_SYSTICK_Config(SystemCoreClock / 10000U) > 0U)
	{
		return HAL_ERROR;
	}

	/* Configure the SysTick IRQ priority */
	if (TickPriority < (1UL << __NVIC_PRIO_BITS))
	{
		HAL_NVIC_SetPriority(SysTick_IRQn, TickPriority, 0U);
		uwTickPrio = TickPriority;
	}
	else
	{
		return HAL_ERROR;
	}

	/* Return function status */
	return HAL_OK;
}

void HAL_Delay(uint32_t Delay)
{
	bsp_DelayMS(Delay);
}

/***************************** 杭州鑫高科技 www.hzxingao.com (END OF FILE) *********************************/
