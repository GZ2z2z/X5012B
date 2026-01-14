/****************************************Copyright (c)**************************************************
**                                杭州鑫高科技有限公司
**                                     
**                                
** 文   件   名: bsp_uart.h
** 最后修改日期: 2015/6/23
** 描        述: 
** 版	     本: V1.1
** 主  控  芯 片:  晶振频率: 
** IDE: 
**********************************************************************************************************/
#ifndef __BSP_UART_H
#define	__BSP_UART_H

 
/* 定义端口号 */
typedef enum {
	COM1 = 0,	/* USART1  PA9, PA10 或  PB6, PB7 */
	COM2,		/* USART2, PA2, PA3或 PD5,PD6	  */
	COM3,		/* USART3, PB10, PB11 			  */
	COM4,		/* UART4, PC10, PC11 			  */
	COM5,		/* UART5, PC12, PD2 			  */
	COM6		/* USART6, PC6, PC7 STM32F107无此串口*/
} COM_DevEnum;
  
//用户实际使用只需要这三个函数
void bsp_InitCOM(void);														//BSP串口初始化
bool bsp_COM_Write(COM_DevEnum _ucPort, uint8_t *_ucaBuf, uint32_t _usLen);		//COM口发送函数 
bool bsp_COM_Read(COM_DevEnum _ucPort, uint8_t *_pByte);							//COM口接收函数
void bsp_COM_Sync(void);
bool bsp_COM_SetBaudrate(COM_DevEnum _ucPort, uint32_t baudRate);
bool bsp_COM_ClearRxBuff(COM_DevEnum _ucPort);
bool bsp_COM_HasData(COM_DevEnum _ucPort);


#endif




	

	

	












