/*
*********************************************************************************************************
* @file    	bsp_uart.c
* @author  	SY
* @version 	V1.0.3
* @date    	2016-10-17 13:05:46
* @IDE	 	Keil V4.72.10.0
* @Chip    	STM32F407VE
* @brief   	串口模块源文件
*********************************************************************************************************
* @attention
* -------------------------------------------------------------------------------------------------------
*	2018-9-4 14:07:35 		孙宇		V1.0.1
	1、	在串口初始化时，增加以下内容：
		__HAL_UART_CLEAR_FLAG(&this->handle, UART_FLAG_TC);
		while (!__HAL_UART_GET_FLAG(&this->handle, UART_FLAG_TC));
		__HAL_UART_CLEAR_FLAG(&this->handle, UART_FLAG_TC);
		解决串口第一个字节发送不出去的问题！
	2、	在每个串口中断服务函数中，增加 HAL_ReceiveITHandler()。
		原因：在 HAL_UART_IRQHandler()中，出现 USART_SR_ORE 溢出错误，导致HAL_UART_RxCpltCallback()
		不能执行，其他设备发送给本机的串口数据不能正常接收。
*	2018-10-26 08:47:36		孙宇		V1.0.2
	1、	在调试时遇到串口不能接收的情况，仿真时发现是串口被锁住__HAL_LOCK(huart);
	当串口进入发送中断时，会对串口加锁，如果此时进入串口接收中断，由于串口已经锁住，不能再打开接收
	中断，导致串口永远接收不到数据。
	因此增加 bsp_COM_Sync()，在外部循环调用。
*	2018-10-29 12:44:12 	孙宇		V1.0.3
	1、	屏蔽掉 HAL_UART_Receive_IT() 的 __HAL_LOCK(huart); 
* 
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private Includes
*********************************************************************************************************
*/
#include "bsp.h"


/*
*********************************************************************************************************
*                              				Private define
*********************************************************************************************************
*/
//配置哪些串口需要使用，使用的配置为1  
#define	UART1_FIFO_EN						(true)
#define	UART2_FIFO_EN						(true)
#define	UART3_FIFO_EN						(true)
#define	UART4_FIFO_EN						(false)
#define	UART5_FIFO_EN						(false)
#define	UART6_FIFO_EN						(false)

/* 定义串口波特率和FIFO缓冲区大小，分为发送缓冲区和接收缓冲区, 支持全双工 */
#if (UART1_FIFO_EN)
	#define UART1_BAUD			38400L
	#define UART1_TX_BUF_SIZE	1*512
	#define UART1_RX_BUF_SIZE	1*512
	#define UART1_RS485			false
		
#if (false)
	#define	USART1_GPIO_CLK_ENABLE()			do {\
													__HAL_RCC_GPIOA_CLK_ENABLE();\
												} while(0)
	#define USART1_CLK_ENABLE()					do {\
													__HAL_RCC_USART1_CLK_ENABLE();\
												} while(0)												
	#define USART1_TX_AF						GPIO_AF7_USART1
	#define USART1_RX_AF						GPIO_AF7_USART1
	#define	USART1_TX_GPIO_PORT 				GPIOA
	#define	USART1_TX_GPIO_PIN					GPIO_PIN_9
	#define	USART1_RX_GPIO_PORT 				GPIOA
	#define	USART1_RX_GPIO_PIN					GPIO_PIN_10
#else
	#define	USART1_GPIO_CLK_ENABLE()			do {\
													__HAL_RCC_GPIOB_CLK_ENABLE();\
												} while(0)		
	#define USART1_CLK_ENABLE()					do {\
													__HAL_RCC_USART1_CLK_ENABLE();\
												} while(0)
	#define USART1_TX_AF						GPIO_AF7_USART1
	#define USART1_RX_AF						GPIO_AF7_USART1
	#define	USART1_TX_GPIO_PORT 				GPIOB
	#define	USART1_TX_GPIO_PIN					GPIO_PIN_6
	#define	USART1_RX_GPIO_PORT 				GPIOB
	#define	USART1_RX_GPIO_PIN					GPIO_PIN_7
#endif
#endif
 
#if (UART2_FIFO_EN)
	#define UART2_BAUD			38400L
	#define UART2_TX_BUF_SIZE	1*512
	#define UART2_RX_BUF_SIZE	1*512
	#define UART2_RS485			false
		
#if (false)
	#define	USART2_GPIO_CLK_ENABLE()			do {\
													__HAL_RCC_GPIOA_CLK_ENABLE();\
												} while(0)
	#define USART2_CLK_ENABLE()					do {\
													__HAL_RCC_USART2_CLK_ENABLE();\
												} while(0)
	#define USART2_TX_AF						GPIO_AF7_USART2
	#define USART2_RX_AF						GPIO_AF7_USART2
	#define	USART2_TX_GPIO_PORT 				GPIOA
	#define	USART2_TX_GPIO_PIN					GPIO_PIN_2
	#define	USART2_RX_GPIO_PORT 				GPIOA
	#define	USART2_RX_GPIO_PIN					GPIO_PIN_3
#else
	#define	USART2_GPIO_CLK_ENABLE()			do {\
													__HAL_RCC_GPIOD_CLK_ENABLE();\
												} while(0)	
	#define USART2_CLK_ENABLE()					do {\
													__HAL_RCC_USART2_CLK_ENABLE();\
												} while(0)
	#define USART2_TX_AF						GPIO_AF7_USART2
	#define USART2_RX_AF						GPIO_AF7_USART2
	#define	USART2_TX_GPIO_PORT 				GPIOD
	#define	USART2_TX_GPIO_PIN					GPIO_PIN_5
	#define	USART2_RX_GPIO_PORT 				GPIOD
	#define	USART2_RX_GPIO_PIN					GPIO_PIN_6
#endif
#endif

#if (UART3_FIFO_EN)
	#define UART3_BAUD			38400L		
	#define UART3_TX_BUF_SIZE	1*512
	#define UART3_RX_BUF_SIZE	1*512
	#define UART3_RS485			true
		
#if (UART3_RS485)
	#define	USART3_RS485_GPIO_CLK_ENABLE()		do {\
													__HAL_RCC_GPIOE_CLK_ENABLE();\
												} while(0)
	#define	USART3_RS485_DIR_GPIO_PORT 			GPIOE
	#define	USART3_RS485_DIR_GPIO_PIN			GPIO_PIN_7											
#endif
#if (false)
	#define	USART3_GPIO_CLK_ENABLE()			do {\
													__HAL_RCC_GPIOC_CLK_ENABLE();\
												} while(0)
	#define USART3_CLK_ENABLE()					do {\
													__HAL_RCC_USART3_CLK_ENABLE();\
												} while(0)
	#define USART3_TX_AF						GPIO_AF7_USART3
	#define USART3_RX_AF						GPIO_AF7_USART3
	#define	USART3_TX_GPIO_PORT 				GPIOC
	#define	USART3_TX_GPIO_PIN					GPIO_PIN_10
	#define	USART3_RX_GPIO_PORT 				GPIOC
	#define	USART3_RX_GPIO_PIN					GPIO_PIN_11
#else
	#define	USART3_GPIO_CLK_ENABLE()			do {\
													__HAL_RCC_GPIOD_CLK_ENABLE();\
												} while(0)		
	#define USART3_CLK_ENABLE()					do {\
													__HAL_RCC_USART3_CLK_ENABLE();\
												} while(0)
	#define USART3_TX_AF						GPIO_AF7_USART3
	#define USART3_RX_AF						GPIO_AF7_USART3
	#define	USART3_TX_GPIO_PORT 				GPIOD
	#define	USART3_TX_GPIO_PIN					GPIO_PIN_8
	#define	USART3_RX_GPIO_PORT 				GPIOD
	#define	USART3_RX_GPIO_PIN					GPIO_PIN_9
#endif
#endif

#if (UART4_FIFO_EN)
	#define UART4_BAUD			115200L
	#define UART4_TX_BUF_SIZE	1*512
	#define UART4_RX_BUF_SIZE	1*512
	#define UART4_RS485			false
												
#if (true)
	#define	USART4_GPIO_CLK_ENABLE()			do {\
													__HAL_RCC_GPIOC_CLK_ENABLE();\
												} while(0)
	#define USART4_CLK_ENABLE()					do {\
													__HAL_RCC_UART4_CLK_ENABLE();\
												} while(0)
	#define USART4_TX_AF						GPIO_AF8_UART4
	#define USART4_RX_AF						GPIO_AF8_UART4
	#define	USART4_TX_GPIO_PORT 				GPIOC
	#define	USART4_TX_GPIO_PIN					GPIO_PIN_10
	#define	USART4_RX_GPIO_PORT 				GPIOC
	#define	USART4_RX_GPIO_PIN					GPIO_PIN_11
#else
	#define	USART4_GPIO_CLK_ENABLE()			do {\
													__HAL_RCC_GPIOC_CLK_ENABLE();\
												} while(0)
	#define USART4_CLK_ENABLE()					do {\
													__HAL_RCC_UART4_CLK_ENABLE();\
												} while(0)
	#define USART4_TX_AF						GPIO_AF8_UART4
	#define USART4_RX_AF						GPIO_AF8_UART4
	#define	USART4_TX_GPIO_PORT 				GPIOC
	#define	USART4_TX_GPIO_PIN					GPIO_PIN_10
	#define	USART4_RX_GPIO_PORT 				GPIOC
	#define	USART4_RX_GPIO_PIN					GPIO_PIN_11
#endif												
#endif

#if (UART5_FIFO_EN)
	#define UART5_BAUD			115200L
	#define UART5_TX_BUF_SIZE	1*1024
	#define UART5_RX_BUF_SIZE	1*1024
	#define UART5_RS485			false
	
#if (true)
	#define	USART5_GPIO_CLK_ENABLE()			do {\
													__HAL_RCC_GPIOC_CLK_ENABLE();\
													__HAL_RCC_GPIOD_CLK_ENABLE();\
												} while(0)
	#define USART5_CLK_ENABLE()					do {\
													__HAL_RCC_UART5_CLK_ENABLE();\
												} while(0)
	#define USART5_TX_AF						GPIO_AF8_UART5
	#define USART5_RX_AF						GPIO_AF8_UART5
	#define	USART5_TX_GPIO_PORT 				GPIOC
	#define	USART5_TX_GPIO_PIN					GPIO_PIN_12
	#define	USART5_RX_GPIO_PORT 				GPIOD
	#define	USART5_RX_GPIO_PIN					GPIO_PIN_2
#else
	#define	USART5_GPIO_CLK_ENABLE()			do {\
													__HAL_RCC_GPIOC_CLK_ENABLE();\
													__HAL_RCC_GPIOD_CLK_ENABLE();\
												} while(0)
	#define USART5_CLK_ENABLE()					do {\
													__HAL_RCC_UART5_CLK_ENABLE();\
												} while(0)
	#define USART5_TX_AF						GPIO_AF8_UART5
	#define USART5_RX_AF						GPIO_AF8_UART5
	#define	USART5_TX_GPIO_PORT 				GPIOC
	#define	USART5_TX_GPIO_PIN					GPIO_PIN_12
	#define	USART5_RX_GPIO_PORT 				GPIOD
	#define	USART5_RX_GPIO_PIN					GPIO_PIN_2
#endif
#endif

#if (UART6_FIFO_EN)
	#define UART6_BAUD			115200L
	#define UART6_TX_BUF_SIZE	1*1024
	#define UART6_RX_BUF_SIZE	1*1024
	#define UART6_RS485			false
	
#if (false)
	#define	USART6_GPIO_CLK_ENABLE()			do {\
													__HAL_RCC_GPIOG_CLK_ENABLE();\
												} while(0)
	#define USART6_CLK_ENABLE()					do {\
													__HAL_RCC_USART6_CLK_ENABLE();\
												} while(0)
	#define USART6_TX_AF						GPIO_AF8_USART6
	#define USART6_RX_AF						GPIO_AF8_USART6
	#define	USART6_TX_GPIO_PORT 				GPIOG
	#define	USART6_TX_GPIO_PIN					GPIO_PIN_14
	#define	USART6_RX_GPIO_PORT 				GPIOG
	#define	USART6_RX_GPIO_PIN					GPIO_PIN_9
#else
	#define	USART6_GPIO_CLK_ENABLE()			do {\
													__HAL_RCC_GPIOC_CLK_ENABLE();\
												} while(0)
	#define USART6_CLK_ENABLE()					do {\
													__HAL_RCC_USART6_CLK_ENABLE();\
												} while(0)
	#define USART6_TX_AF						GPIO_AF8_USART6
	#define USART6_RX_AF						GPIO_AF8_USART6
	#define	USART6_TX_GPIO_PORT 				GPIOC
	#define	USART6_TX_GPIO_PIN					GPIO_PIN_6
	#define	USART6_RX_GPIO_PORT 				GPIOC
	#define	USART6_RX_GPIO_PIN					GPIO_PIN_7
#endif
#endif

/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/
struct RS485_Drv {
	bool enabled;
	GPIO_TypeDef *GPIOx;
	uint16_t GPIO_Pin;
	void (*sendEnable)(struct RS485_Drv *this, bool enabled);
};

/* 串口设备结构体 */
typedef struct
{
	COM_DevEnum dev;
	UART_HandleTypeDef handle;
	uint32_t baudRate;
	
	SEQUEUE_TypeDef sendQueue;		
	uint8_t *rxBuffer;
	uint32_t rxBufferSize;
	
	SEQUEUE_TypeDef receiveQueue;	
	uint8_t *txBuffer;
	uint32_t txBufferSize;
	
	uint8_t byteRxBuffer;
	bool txBusy;				
	uint8_t byteTxBuffer;
	struct RS485_Drv rs485;
}UART_T;  
 


/*
*********************************************************************************************************
*                              				Private constants
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private macro
*********************************************************************************************************
*/
static void RS485_SendEnable(struct RS485_Drv *this, bool enabled)
{
	HAL_GPIO_WritePin(this->GPIOx, this->GPIO_Pin, enabled ? GPIO_PIN_SET : GPIO_PIN_RESET); 
}

/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
#if (UART1_FIFO_EN)
	static uint8_t g_TxBuf1[UART1_TX_BUF_SIZE];		/* 发送缓冲区 */
	static uint8_t g_RxBuf1[UART1_RX_BUF_SIZE];		/* 接收缓冲区 */
#endif

#if (UART2_FIFO_EN)
	static uint8_t g_TxBuf2[UART2_TX_BUF_SIZE];		/* 发送缓冲区 */
	static uint8_t g_RxBuf2[UART2_RX_BUF_SIZE];		/* 接收缓冲区 */
#endif

#if (UART3_FIFO_EN)
	static uint8_t g_TxBuf3[UART3_TX_BUF_SIZE];		/* 发送缓冲区 */
	static uint8_t g_RxBuf3[UART3_RX_BUF_SIZE];		/* 接收缓冲区 */
#endif

#if (UART4_FIFO_EN)
	static uint8_t g_TxBuf4[UART4_TX_BUF_SIZE];		/* 发送缓冲区 */
	static uint8_t g_RxBuf4[UART4_RX_BUF_SIZE];		/* 接收缓冲区 */
#endif

#if (UART5_FIFO_EN)
	static uint8_t g_TxBuf5[UART5_TX_BUF_SIZE];		/* 发送缓冲区 */
	static uint8_t g_RxBuf5[UART5_RX_BUF_SIZE];		/* 接收缓冲区 */
#endif

#if (UART6_FIFO_EN)
	static uint8_t g_TxBuf6[UART6_TX_BUF_SIZE];		/* 发送缓冲区 */
	static uint8_t g_RxBuf6[UART6_RX_BUF_SIZE];		/* 接收缓冲区 */
#endif

static UART_T UartDev[] = {
#if (UART1_FIFO_EN)
{
	.dev					= COM1,
	.handle					= {
		.Instance			= USART1,
	},
	.baudRate				= UART1_BAUD,
	.rxBuffer				= g_RxBuf1,
	.rxBufferSize			= ARRAY_SIZE(g_RxBuf1),
	.txBuffer				= g_TxBuf1,
	.txBufferSize			= ARRAY_SIZE(g_TxBuf1),
},
#endif

#if (UART2_FIFO_EN)
{
	.dev					= COM2,
	.handle					= {
		.Instance			= USART2,
	},
	.baudRate				= UART2_BAUD,
	.rxBuffer				= g_RxBuf2,
	.rxBufferSize			= ARRAY_SIZE(g_RxBuf2),
	.txBuffer				= g_TxBuf2,
	.txBufferSize			= ARRAY_SIZE(g_TxBuf2),
},
#endif

#if (UART3_FIFO_EN)
{
	.dev					= COM3,
	.handle					= {
		.Instance			= USART3,
	},
	.rs485					= {
		.enabled			= UART3_RS485,
		.GPIOx				= USART3_RS485_DIR_GPIO_PORT,
		.GPIO_Pin			= USART3_RS485_DIR_GPIO_PIN,
		.sendEnable			= RS485_SendEnable,
	},
	.baudRate				= UART3_BAUD,
	.rxBuffer				= g_RxBuf3,
	.rxBufferSize			= ARRAY_SIZE(g_RxBuf3),
	.txBuffer				= g_TxBuf3,
	.txBufferSize			= ARRAY_SIZE(g_TxBuf3),
},
#endif

#if (UART4_FIFO_EN)
{
	.dev					= COM4,
	.handle					= {
		.Instance			= UART4,
	},
	.baudRate				= UART4_BAUD,
	.rxBuffer				= g_RxBuf4,
	.rxBufferSize			= ARRAY_SIZE(g_RxBuf4),
	.txBuffer				= g_TxBuf4,
	.txBufferSize			= ARRAY_SIZE(g_TxBuf4),
},
#endif

#if (UART5_FIFO_EN)
{
	.dev					= COM5,
	.handle					= {
		.Instance			= UART5,
	},
	.baudRate				= UART5_BAUD,
	.rxBuffer				= g_RxBuf5,
	.rxBufferSize			= ARRAY_SIZE(g_RxBuf5),
	.txBuffer				= g_TxBuf5,
	.txBufferSize			= ARRAY_SIZE(g_TxBuf5),
},
#endif

#if (UART6_FIFO_EN)
{
	.dev					= COM6,
	.handle					= {
		.Instance			= USART6,
	},
	.rs485					= UART6_RS485,
	.baudRate				= UART6_BAUD,
	.rxBuffer				= g_RxBuf6,
	.rxBufferSize			= ARRAY_SIZE(g_RxBuf6),
	.txBuffer				= g_TxBuf6,
	.txBufferSize			= ARRAY_SIZE(g_TxBuf6),
},
#endif
};


/*
*********************************************************************************************************
*                              				Private function prototypes
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                              				Private functions
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : ComToUart
* Description	: 将COM端口号转换为UART指针
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static UART_T *ComToUart(COM_DevEnum dev)
{
	UART_T *handle = NULL;
	
	for (uint8_t i=0; i<ARRAY_SIZE(UartDev); ++i) {
		if (dev == UartDev[i].dev) {
			handle = &UartDev[i];
			break;
		}
	}
	
	return handle;
}

/*
*********************************************************************************************************
* Function Name : GetHandleByDevice
* Description	: 通过句柄获取设备地址
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static UART_T *GetHandleByDevice(UART_HandleTypeDef *handle)
{
	return container_of(handle, UART_T, handle);
}

/*
*********************************************************************************************************
* Function Name : USART_IRQHandler
* Description	: USART中断服务程序
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
#if UART1_FIFO_EN == 1
void USART1_IRQHandler(void)
{
	UART_T *pUart = ComToUart(COM1);
	if (pUart)
	{		
		HAL_UART_IRQHandler(&pUart->handle);
	}
}
#endif

#if UART2_FIFO_EN == 1
void USART2_IRQHandler(void)
{
	UART_T *pUart = ComToUart(COM2);
	if (pUart)
	{		
		HAL_UART_IRQHandler(&pUart->handle);
	}
}
#endif

#if UART3_FIFO_EN == 1
void USART3_IRQHandler(void)
{
	UART_T *pUart = ComToUart(COM3);
	if (pUart)
	{		
		HAL_UART_IRQHandler(&pUart->handle);
	}	
}
#endif

#if UART4_FIFO_EN == 1
void UART4_IRQHandler(void)
{
	UART_T *pUart = ComToUart(COM4);
	if (pUart)
	{		
		HAL_UART_IRQHandler(&pUart->handle);
	}
}
#endif

#if UART5_FIFO_EN == 1
void UART5_IRQHandler(void)
{
	UART_T *pUart = ComToUart(COM5);
	if (pUart)
	{		
		HAL_UART_IRQHandler(&pUart->handle);
	}
}
#endif

#if UART6_FIFO_EN == 1
void USART6_IRQHandler(void)
{
	UART_T *pUart = ComToUart(COM6);
	if (pUart)
	{		
		HAL_UART_IRQHandler(&pUart->handle);
	}
}
#endif

/*
*********************************************************************************************************
*                              				MSP
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : HAL_USART_MspInit
* Description	: 串口MSP初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_UART_MspInit(UART_HandleTypeDef *husart)
{
	GPIO_InitTypeDef GPIO_InitStructure = {0}; 
	const uint32_t PreemptPriority = 1;
	const uint32_t SubPriority = 0;
	
	if (husart->Instance == USART1)
	{
	#if (UART1_FIFO_EN)	
		/* 初始化时钟 */
		USART1_GPIO_CLK_ENABLE();
		USART1_CLK_ENABLE();
		
		/* 初始化GPIO-TX */
		GPIO_InitStructure.Pin = USART1_TX_GPIO_PIN;
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
		GPIO_InitStructure.Alternate = USART1_TX_AF;
		HAL_GPIO_Init(USART1_TX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 初始化GPIO-RX */
		GPIO_InitStructure.Pin = USART1_RX_GPIO_PIN;
		GPIO_InitStructure.Alternate = USART1_RX_AF;
		HAL_GPIO_Init(USART1_RX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 设置优先级 */
		HAL_NVIC_SetPriority(USART1_IRQn, PreemptPriority, SubPriority);
		HAL_NVIC_EnableIRQ(USART1_IRQn);
	#endif	
	}
	else if (husart->Instance == USART2)
	{
	#if (UART2_FIFO_EN)		
		/* 初始化时钟 */
		USART2_GPIO_CLK_ENABLE();
		USART2_CLK_ENABLE();
		
		/* 初始化GPIO-TX */
		GPIO_InitStructure.Pin = USART2_TX_GPIO_PIN;
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
		GPIO_InitStructure.Alternate = USART2_TX_AF;
		HAL_GPIO_Init(USART2_TX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 初始化GPIO-RX */
		GPIO_InitStructure.Pin = USART2_RX_GPIO_PIN;
		GPIO_InitStructure.Alternate = USART2_RX_AF;
		HAL_GPIO_Init(USART2_RX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 设置优先级 */
		HAL_NVIC_SetPriority(USART2_IRQn, PreemptPriority, SubPriority);
		HAL_NVIC_EnableIRQ(USART2_IRQn);
	#endif	
	}
	else if (husart->Instance == USART3)
	{
	#if (UART3_FIFO_EN)		
		/* 初始化时钟 */
		USART3_GPIO_CLK_ENABLE();
		USART3_CLK_ENABLE();
		
		/* 初始化GPIO-TX */
		GPIO_InitStructure.Pin = USART3_TX_GPIO_PIN;
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
		GPIO_InitStructure.Alternate = USART3_TX_AF;
		HAL_GPIO_Init(USART3_TX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 初始化GPIO-RX */
		GPIO_InitStructure.Pin = USART3_RX_GPIO_PIN;
		GPIO_InitStructure.Alternate = USART3_RX_AF;
		HAL_GPIO_Init(USART3_RX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 设置优先级 */
		HAL_NVIC_SetPriority(USART3_IRQn, PreemptPriority, SubPriority);
		HAL_NVIC_EnableIRQ(USART3_IRQn);
		
	#if (UART3_RS485)
		USART3_RS485_GPIO_CLK_ENABLE();
		
		GPIO_InitStructure.Pin = USART3_RS485_DIR_GPIO_PIN;
		GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
		GPIO_InitStructure.Alternate = 0;
		HAL_GPIO_Init(USART3_RS485_DIR_GPIO_PORT, &GPIO_InitStructure);
	#endif
	#endif
	}
	else if (husart->Instance == UART4)
	{
	#if (UART4_FIFO_EN)		
		/* 初始化时钟 */
		USART4_GPIO_CLK_ENABLE();
		USART4_CLK_ENABLE();
		
		/* 初始化GPIO-TX */
		GPIO_InitStructure.Pin = USART4_TX_GPIO_PIN;
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
		GPIO_InitStructure.Alternate = USART4_TX_AF;
		HAL_GPIO_Init(USART4_TX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 初始化GPIO-RX */
		GPIO_InitStructure.Pin = USART4_RX_GPIO_PIN;
		GPIO_InitStructure.Alternate = USART4_RX_AF;
		HAL_GPIO_Init(USART4_RX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 设置优先级 */
		HAL_NVIC_SetPriority(UART4_IRQn, PreemptPriority, SubPriority);
		HAL_NVIC_EnableIRQ(UART4_IRQn);
	#endif	
	}
	else if (husart->Instance == UART5)
	{
	#if (UART5_FIFO_EN)		
		/* 初始化时钟 */
		USART5_GPIO_CLK_ENABLE();
		USART5_CLK_ENABLE();
		
		/* 初始化GPIO-TX */
		GPIO_InitStructure.Pin = USART5_TX_GPIO_PIN;
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
		GPIO_InitStructure.Alternate = USART5_TX_AF;
		HAL_GPIO_Init(USART5_TX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 初始化GPIO-RX */
		GPIO_InitStructure.Pin = USART5_RX_GPIO_PIN;
		GPIO_InitStructure.Alternate = USART5_RX_AF;
		HAL_GPIO_Init(USART5_RX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 设置优先级 */
		HAL_NVIC_SetPriority(UART5_IRQn, PreemptPriority, SubPriority);
		HAL_NVIC_EnableIRQ(UART5_IRQn);
	#endif	
	}
	else if (husart->Instance == USART6)
	{
	#if (UART6_FIFO_EN)		
		/* 初始化时钟 */
		USART6_GPIO_CLK_ENABLE();
		USART6_CLK_ENABLE();
		
		/* 初始化GPIO-TX */
		GPIO_InitStructure.Pin = USART6_TX_GPIO_PIN;
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
		GPIO_InitStructure.Alternate = USART6_TX_AF;
		HAL_GPIO_Init(USART6_TX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 初始化GPIO-RX */
		GPIO_InitStructure.Pin = USART6_RX_GPIO_PIN;
		GPIO_InitStructure.Alternate = USART6_RX_AF;
		HAL_GPIO_Init(USART6_RX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 设置优先级 */
		HAL_NVIC_SetPriority(USART6_IRQn, PreemptPriority, SubPriority);
		HAL_NVIC_EnableIRQ(USART6_IRQn);
	#endif	
	}
}

/*
*********************************************************************************************************
* Function Name : HAL_UART_MspDeInit
* Description	: MSP DeInit
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1)
	{
	#if (UART1_FIFO_EN)	
		/*##-1- Reset peripherals ##################################################*/
		__HAL_RCC_USART1_FORCE_RESET();
		__HAL_RCC_USART1_RELEASE_RESET();
		
		/*##-2- Disable peripherals and GPIO Clocks #################################*/
		/* Configure UART Tx as alternate function  */
		HAL_GPIO_DeInit(USART1_TX_GPIO_PORT, USART1_TX_GPIO_PIN);
		/* Configure UART Rx as alternate function  */
		HAL_GPIO_DeInit(USART1_RX_GPIO_PORT, USART1_RX_GPIO_PIN);
		
		/*##-3- Disable the NVIC for UART ##########################################*/
		HAL_NVIC_DisableIRQ(USART1_IRQn);
	#endif	
	}
	else if (huart->Instance == USART2)
	{
	#if (UART2_FIFO_EN)		
		/*##-1- Reset peripherals ##################################################*/
		__HAL_RCC_USART2_FORCE_RESET();
		__HAL_RCC_USART2_RELEASE_RESET();
		
		/*##-2- Disable peripherals and GPIO Clocks #################################*/
		/* Configure UART Tx as alternate function  */
		HAL_GPIO_DeInit(USART2_TX_GPIO_PORT, USART2_TX_GPIO_PIN);
		/* Configure UART Rx as alternate function  */
		HAL_GPIO_DeInit(USART2_RX_GPIO_PORT, USART2_RX_GPIO_PIN);
		
		/*##-3- Disable the NVIC for UART ##########################################*/
		HAL_NVIC_DisableIRQ(USART2_IRQn);
	#endif	
	}
	else if (huart->Instance == USART3)
	{
	#if (UART3_FIFO_EN)		
		/*##-1- Reset peripherals ##################################################*/
		__HAL_RCC_USART3_FORCE_RESET();
		__HAL_RCC_USART3_RELEASE_RESET();
		
		/*##-2- Disable peripherals and GPIO Clocks #################################*/
		/* Configure UART Tx as alternate function  */
		HAL_GPIO_DeInit(USART3_TX_GPIO_PORT, USART3_TX_GPIO_PIN);
		/* Configure UART Rx as alternate function  */
		HAL_GPIO_DeInit(USART3_RX_GPIO_PORT, USART3_RX_GPIO_PIN);
		
		/*##-3- Disable the NVIC for UART ##########################################*/
		HAL_NVIC_DisableIRQ(USART3_IRQn);
	#endif	
	}
	else if (huart->Instance == UART4)
	{
	#if (UART4_FIFO_EN)		
		/*##-1- Reset peripherals ##################################################*/
		__HAL_RCC_UART4_FORCE_RESET();
		__HAL_RCC_UART4_RELEASE_RESET();
		
		/*##-2- Disable peripherals and GPIO Clocks #################################*/
		/* Configure UART Tx as alternate function  */
		HAL_GPIO_DeInit(USART4_TX_GPIO_PORT, USART4_TX_GPIO_PIN);
		/* Configure UART Rx as alternate function  */
		HAL_GPIO_DeInit(USART4_RX_GPIO_PORT, USART4_RX_GPIO_PIN);
		
		/*##-3- Disable the NVIC for UART ##########################################*/
		HAL_NVIC_DisableIRQ(UART4_IRQn);
	#endif	
	}
	else if (huart->Instance == UART5)
	{
	#if (UART5_FIFO_EN)		
		/*##-1- Reset peripherals ##################################################*/
		__HAL_RCC_UART5_FORCE_RESET();
		__HAL_RCC_UART5_RELEASE_RESET();
		
		/*##-2- Disable peripherals and GPIO Clocks #################################*/
		/* Configure UART Tx as alternate function  */
		HAL_GPIO_DeInit(USART5_TX_GPIO_PORT, USART5_TX_GPIO_PIN);
		/* Configure UART Rx as alternate function  */
		HAL_GPIO_DeInit(USART5_RX_GPIO_PORT, USART5_RX_GPIO_PIN);
		
		/*##-3- Disable the NVIC for UART ##########################################*/
		HAL_NVIC_DisableIRQ(UART5_IRQn);
	#endif	
	}
	else if (huart->Instance == USART6)
	{
	#if (UART6_FIFO_EN)		
		/*##-1- Reset peripherals ##################################################*/
		__HAL_RCC_USART6_FORCE_RESET();
		__HAL_RCC_USART6_RELEASE_RESET();
		
		/*##-2- Disable peripherals and GPIO Clocks #################################*/
		/* Configure UART Tx as alternate function  */
		HAL_GPIO_DeInit(USART6_TX_GPIO_PORT, USART6_TX_GPIO_PIN);
		/* Configure UART Rx as alternate function  */
		HAL_GPIO_DeInit(USART6_RX_GPIO_PORT, USART6_RX_GPIO_PIN);
		
		/*##-3- Disable the NVIC for UART ##########################################*/
		HAL_NVIC_DisableIRQ(USART6_IRQn);
	#endif	
	}
}

/*
*********************************************************************************************************
* Function Name : TxCoreHandler
* Description	: 发送核心处理程序
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void TxCoreHandler(UART_T *this)
{
	if (!this->txBusy) {
		if (PopSeqQueueByte(&this->sendQueue, &this->byteTxBuffer) == true) {
			this->txBusy = true;
			if (this->rs485.enabled) {
				this->rs485.sendEnable(&this->rs485, true);
			}
		}
	}
	if (this->txBusy) {
		if (HAL_UART_Transmit_IT(&this->handle, &this->byteTxBuffer, sizeof(this->byteTxBuffer)) == HAL_OK) {
			this->txBusy = false;
		}
	}
}

/*
*********************************************************************************************************
* Function Name : HAL_UART_TxCpltCallback
* Description	: 串口发送完成回调函数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
	UART_T *this = GetHandleByDevice(UartHandle);
	if (this->rs485.enabled) {
		this->rs485.sendEnable(&this->rs485, false);
	}
	TxCoreHandler(this);
}

/*
*********************************************************************************************************
* Function Name : HAL_UART_RxCpltCallback
* Description	: 串口接收完成回调函数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
	UART_T *this = GetHandleByDevice(UartHandle);
	PushSeqQueueByte(&this->receiveQueue, &this->byteRxBuffer);
	if (HAL_UART_Receive_IT(UartHandle, &this->byteRxBuffer, sizeof(this->byteRxBuffer)) != HAL_OK) {		
		ECHO(ECHO_DEBUG, "[BSP][COM] COM%d 中断接收异常！", this->dev + 1);
	}
}

/*
*********************************************************************************************************
* Function Name : HAL_UART_ErrorCallback
* Description	: 串口错误处理回调函数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	UART_T *this = GetHandleByDevice(huart);
	if (huart->ErrorCode) {
		ECHO(ECHO_DEBUG, "[BSP][COM] COM%d error, error code %d.", this->dev + 1, huart->ErrorCode);
	}
}

/*
*********************************************************************************************************
*                              				API
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_COM_SetBaudrate
* Description	: 设置串口波特率
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_COM_SetBaudrate(COM_DevEnum _ucPort, uint32_t baudRate)
{
	UART_T *pUart = ComToUart(_ucPort);
	if (pUart == NULL) {		
		return false;
	}
	
	__HAL_UART_DISABLE(&pUart->handle);
	
	pUart->handle.Init.BaudRate = baudRate;
	if(HAL_UART_Init(&pUart->handle) != HAL_OK) {
		return false;
	}
	
	__HAL_UART_ENABLE(&pUart->handle);	
	return true;
} 
	
/*
*********************************************************************************************************
* Function Name : bsp_COM_Write
* Description	: 向COM口发送一组数据。数据放到发送缓冲区后立即返回，由中断服务程序在后台完成发送
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_COM_Write(COM_DevEnum _ucPort, uint8_t *_ucaBuf, uint32_t _usLen)
{
	UART_T *this = ComToUart(_ucPort);
	if (this == NULL) {		
		return false;
	}	
	
	/**
	 *	使用 HAL_UART_Transmit_IT() 中断发送函数，使能<发送寄存器为空>中断。
	 *	进入中断服务函数后，如果发现还有数据未发送，则继续发送。
	 *	发送完成后，使能<发送完成>中断，然后调用回调函数通知用户，
	 *	用户可以选择在回调函数中继续发送数据。
	 */
	
	//填充数据
	for (uint32_t i=0; i<_usLen; ++i) {
		if (PushSeqQueueByte(&this->sendQueue, &_ucaBuf[i]) != true) {
			break;
		}
	}	
	return true;
}

/*
*********************************************************************************************************
* Function Name : bsp_COM_Read
* Description	: 从串口缓冲区读取1字节，非阻塞。无论有无数据均立即返回
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_COM_Read(COM_DevEnum _ucPort, uint8_t *_pByte)
{
	UART_T *this = ComToUart(_ucPort);
	if (this == NULL) {		
		return false;
	}
	return (PopSeqQueueByte(&this->receiveQueue, _pByte) == true);
}

/*
*********************************************************************************************************
* Function Name : bsp_COM_ClearRxBuff
* Description	: 清空串口缓存
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_COM_ClearRxBuff(COM_DevEnum _ucPort)
{
	UART_T *this = ComToUart(_ucPort);
	if (this == NULL) {		
		return false;
	}
	
	ClearSeqQueue(&this->receiveQueue);
	return true;
}

/*
*********************************************************************************************************
* Function Name : bsp_COM_HasData
* Description	: 串口有数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_COM_HasData(COM_DevEnum _ucPort)
{
	UART_T *this = ComToUart(_ucPort);
	if (this == NULL) {		
		return false;
	}
	return (!SeqQueueIsEmpty(&this->receiveQueue));
}


/*
*********************************************************************************************************
* Function Name : InitHardware
* Description	: 初始化硬件
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void InitHardware(UART_T *this)
{
	UART_InitTypeDef *initHandle 	= &this->handle.Init;
	initHandle->BaudRate     		= this->baudRate;
	initHandle->WordLength   		= UART_WORDLENGTH_8B;
	initHandle->StopBits     		= UART_STOPBITS_1;
	initHandle->Parity       		= UART_PARITY_NONE;
	initHandle->HwFlowCtl    		= UART_HWCONTROL_NONE;
	initHandle->Mode         		= UART_MODE_TX_RX;
	if(HAL_UART_DeInit(&this->handle) != HAL_OK)
	{
		BSP_ErrorHandler();
	} 
	if(HAL_UART_Init(&this->handle) != HAL_OK)
	{
		BSP_ErrorHandler();
	}
	
	HAL_UART_Receive_IT(&this->handle, &this->byteRxBuffer, sizeof(this->byteRxBuffer));
	/* CPU的小缺陷：串口配置好，如果直接Send，则第1个字节发送不出去
		如下语句解决第1个字节无法正确发送出去的问题 */
	__HAL_UART_CLEAR_FLAG(&this->handle, UART_FLAG_TC);
	while (!__HAL_UART_GET_FLAG(&this->handle, UART_FLAG_TC));
	__HAL_UART_CLEAR_FLAG(&this->handle, UART_FLAG_TC);
}

/*
*********************************************************************************************************
* Function Name : bsp_COM_Sync
* Description	: 串口同步
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_COM_Sync(void)
{
	for (uint8_t i=0; i<ARRAY_SIZE(UartDev); ++i) {
		UART_T *this = &UartDev[i];
		if (HAL_UART_Receive_IT(&this->handle, &this->byteRxBuffer, sizeof(this->byteRxBuffer)) == HAL_OK) {		
			ECHO(ECHO_DEBUG, "[BSP][COM] COM%d 接收手动打开！", this->dev + 1);
		}
		__HAL_UART_DISABLE_IT(&this->handle, UART_IT_TC);
		TxCoreHandler(this);
		__HAL_UART_ENABLE_IT(&this->handle, UART_IT_TC);
	}	
}

/*
*********************************************************************************************************
* Function Name : InitSoftware
* Description	: 初始化软件
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void InitSoftware(UART_T *this)
{
	CreateSeqQueue(&this->sendQueue, this->txBuffer, this->txBufferSize);
	CreateSeqQueue(&this->receiveQueue, this->rxBuffer, this->rxBufferSize);
	if (this->rs485.enabled) {
		this->rs485.sendEnable(&this->rs485, false);
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_InitCOM
* Description	: 串口初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitCOM(void)
{
	for (uint8_t i=0; i<ARRAY_SIZE(UartDev); ++i) {	
		InitHardware(&UartDev[i]);
		InitSoftware(&UartDev[i]);
	}
	ECHO(ECHO_DEBUG, "[BSP] BSP Init .......... Start!");
	ECHO(ECHO_DEBUG, "[BSP] COM 初始化 .......... OK");
}

/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
