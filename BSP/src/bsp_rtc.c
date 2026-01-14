/*
*********************************************************************************************************
* @file    	bsp_rtc.c
* @author  	SY
* @version 	V1.0.0
* @date    	2018-5-16 13:03:14
* @IDE	 	Keil V4.72.10.0
* @Chip    	STM32F407VE
* @brief   	访问RTC备份域源文件
*********************************************************************************************************
* @attention
*	适用于芯片STM32F407VE
* 
*********************************************************************************************************
*/

#include "bsp.h"

static void RTC_Config(void);


/* 20个RTC备份数据寄存器 */
#define RTC_BKP_DR_NUMBER              20    

/* RTC 备份数据寄存器 */
uint32_t aRTC_BKP_DR[RTC_BKP_DR_NUMBER] =
{
	RTC_BKP_DR0,  RTC_BKP_DR1,  RTC_BKP_DR2, 
	RTC_BKP_DR3,  RTC_BKP_DR4,  RTC_BKP_DR5,
	RTC_BKP_DR6,  RTC_BKP_DR7,  RTC_BKP_DR8, 
	RTC_BKP_DR9,  RTC_BKP_DR10, RTC_BKP_DR11, 
	RTC_BKP_DR12, RTC_BKP_DR13, RTC_BKP_DR14, 
	RTC_BKP_DR15, RTC_BKP_DR16, RTC_BKP_DR17, 
	RTC_BKP_DR18, RTC_BKP_DR19,
};

static RTC_HandleTypeDef RTC_Handle;

/*
*********************************************************************************************************
*	函 数 名: bsp_InitRTC
*	功能说明: 初始化RTC
*	形    参：无
*	返 回 值: 无		        
*********************************************************************************************************
*/
void bsp_InitRTC(void)
{
	RTC_Config();
}
	
/*
*********************************************************************************************************
*	函 数 名: bsp_DeInitRTC
*	功能说明: 恢复默认RTC
*	形    参：无
*	返 回 值: 无		        
*********************************************************************************************************
*/
void bsp_DeInitRTC(void)
{
	/* 使能PWR时钟 */
	__HAL_RCC_PWR_CLK_DISABLE();

	/* 允许访问RTC */
	HAL_PWR_DisableBkUpAccess();
}
	
/*
*********************************************************************************************************
*	函 数 名: RTC_Config
*	功能说明: 配置RTC用于跑表
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
static void RTC_Config(void)
{
	/* 使能PWR时钟 */
	__HAL_RCC_PWR_CLK_ENABLE();

	/* 允许访问RTC */
	HAL_PWR_EnableBkUpAccess();
	
	RTC_Handle.Instance = RTC;
}

/*
*********************************************************************************************************
*	函 数 名: WriteToRTC_BKP_DR
*	功能说明: 写数据到RTC的备份数据寄存器
*	形    参：无FirstRTCBackupData 写入的数据
*	返 回 值: 无
*********************************************************************************************************
*/
void WriteToRTC_BKP_DR(uint8_t RTCBackupIndex, uint16_t RTCBackupData)
{
	if (RTCBackupIndex < RTC_BKP_DR_NUMBER) {
		/* 写数据备份数据寄存器 */
		HAL_RTCEx_BKUPWrite(&RTC_Handle, aRTC_BKP_DR[RTCBackupIndex], RTCBackupData);
	}
}

/*
*********************************************************************************************************
*	函 数 名: ReadRTC_BKP_DR
*	功能说明: 读取备份域数据
*	形    参：RTCBackupIndex 寄存器索引值
*	返 回 值: 
*********************************************************************************************************
*/
uint16_t ReadRTC_BKP_DR(uint8_t RTCBackupIndex)
{
	if (RTCBackupIndex < RTC_BKP_DR_NUMBER) {
		return HAL_RTCEx_BKUPRead(&RTC_Handle, aRTC_BKP_DR[RTCBackupIndex]);		
	}
	
	return 0xFFFF;
}


/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
