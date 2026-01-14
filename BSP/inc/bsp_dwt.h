/*
*********************************************************************************************************
*
*	模块名称 : 数据观察点与跟踪(DWT)模块
*	文件名称 : bsp_dwt.c
*	版    本 : V1.0
*	说    明 : 头文件
*	Copyright (C), 2013-2014, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#ifndef __BSP_DWT_H
#define __BSP_DWT_H

/*
*********************************************************************************************************
*                                             寄存器
*********************************************************************************************************
*/

#define  DWT_CR      *(volatile unsigned int *)0xE0001000
#define  DEM_CR      *(volatile unsigned int *)0xE000EDFC
#define  DBGMCU_CR   *(volatile unsigned int *)0xE0042004
#define  DWT_CYCCNT  *(volatile unsigned int *)0xE0001004
	
void bsp_InitDWT(void);
void TS_StartTmr(void);
void TS_StopTmr(void);
void TS_PrintTmr(void);
void TS_PrintTmrClkCnt(void);
void TS_PrintPeriodTmr(void);

#endif

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
