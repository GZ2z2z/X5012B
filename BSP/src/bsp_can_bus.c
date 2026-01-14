/*
*********************************************************************************************************
* @file    	bsp_can_bus.c
* @author  	SY
* @version 	V1.0.1
* @date    	2016-12-13 16:37:27
* @IDE	 	Keil V4.72.10.0
* @Chip    	STM32F407VE
* @brief   	CAN总线驱动源文件
*********************************************************************************************************
* @attention
*	如果是来自FIFO0的接收中断，则用CAN1_RX0_IRQn中断来处理。
*	如果是来自FIFO1的接收中断，则用CAN1_RX1_IRQn中断来处理。
* 
* =======================================================================================================
*	版本 		时间					作者					说明
* -------------------------------------------------------------------------------------------------------
*	Ver1.0.1 	2018-5-2 15:11:10 		SY		1、	修改CAN设备句柄
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
#define CAN1_EN								(true)
#define CAN1_GPIO_REMAP						(false)
#define CAN1_TX_PACKAGE_BUFF_SIZE			64
#define CAN1_RX_PACKAGE_BUFF_SIZE			64
#if (CAN1_GPIO_REMAP)
	#define CAN1_GPIO_CLK_ENABLE()          do {\
												__HAL_RCC_GPIOB_CLK_ENABLE();\
											} while(0)
	#define CAN1_CLK_ENABLE()				do {\
												__HAL_RCC_CAN1_CLK_ENABLE();\
											} while(0)									
	#define	CAN1_TX_GPIO_PORT				GPIOB
	#define CAN1_TX_GPIO_PIN				GPIO_PIN_9
	#define	CAN1_RX_GPIO_PORT				GPIOB
	#define CAN1_RX_GPIO_PIN				GPIO_PIN_8
	#define CAN1_TX_AF						GPIO_AF9_CAN1
	#define CAN1_RX_AF						GPIO_AF9_CAN1										
#else
	#define CAN1_GPIO_CLK_ENABLE()          do {\
												__HAL_RCC_GPIOA_CLK_ENABLE();\
											} while(0)
	#define CAN1_CLK_ENABLE()				do {\
												__HAL_RCC_CAN1_CLK_ENABLE();\
											} while(0)										
	#define	CAN1_TX_GPIO_PORT				GPIOA
	#define CAN1_TX_GPIO_PIN				GPIO_PIN_12
	#define	CAN1_RX_GPIO_PORT				GPIOA
	#define CAN1_RX_GPIO_PIN				GPIO_PIN_11
	#define CAN1_TX_AF						GPIO_AF9_CAN1
	#define CAN1_RX_AF						GPIO_AF9_CAN1										
#endif	

#define CAN2_EN								(true)
#define CAN2_GPIO_REMAP						(false)
#define CAN2_TX_PACKAGE_BUFF_SIZE			64
#define CAN2_RX_PACKAGE_BUFF_SIZE			64
#if (CAN2_GPIO_REMAP)
	#define CAN2_GPIO_CLK_ENABLE()          do {\
												__HAL_RCC_GPIOB_CLK_ENABLE();\
											} while(0)
	#define CAN2_CLK_ENABLE()				do {\
												__HAL_RCC_CAN2_CLK_ENABLE();\
											} while(0)										
	#define	CAN2_TX_GPIO_PORT				GPIOB
	#define CAN2_TX_GPIO_PIN				GPIO_PIN_6
	#define	CAN2_RX_GPIO_PORT				GPIOB
	#define CAN2_RX_GPIO_PIN				GPIO_PIN_5
	#define CAN2_TX_AF						GPIO_AF9_CAN2
	#define CAN2_RX_AF						GPIO_AF9_CAN2		
#else
	#define CAN2_GPIO_CLK_ENABLE()          do {\
												__HAL_RCC_GPIOB_CLK_ENABLE();\
											} while(0)
	#define CAN2_CLK_ENABLE()				do {\
												__HAL_RCC_CAN2_CLK_ENABLE();\
											} while(0)										
	#define	CAN2_TX_GPIO_PORT				GPIOB
	#define CAN2_TX_GPIO_PIN				GPIO_PIN_13
	#define	CAN2_RX_GPIO_PORT				GPIOB
	#define CAN2_RX_GPIO_PIN				GPIO_PIN_12
	#define CAN2_TX_AF						GPIO_AF9_CAN2
	#define CAN2_RX_AF						GPIO_AF9_CAN2										
#endif	



/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/
struct CanDrv {
	CAN_DevEnum dev;					//设备
	CAN_HandleTypeDef handle;
	SEQUEUE_TypeDef sendQueue;		
	struct CanRxMsg *rxBuffer;
	uint32_t rxBufferSize;
	
	SEQUEUE_TypeDef receiveQueue;	
	struct CanTxMsg *txBuffer;
	uint32_t txBufferSize;
	
	bool (*init)(struct CanDrv *this);
	bool (*initDone)(struct CanDrv *this);
	bool (*send)(struct CanDrv *this, struct CanTxMsg *TxMessage);
	bool (*receive)(struct CanDrv *this, struct CanRxMsg *RxMessage);
};

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
/*
*********************************************************************************************************
* Function Name : PushSeqQueueTxMsg_CallBack
* Description	: 顺序队列发送消息数据入队
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static __INLINE void PushSeqQueueTxMsg_CallBack( void *base, uint32_t offset, void *dataIn )
{
	struct CanTxMsg *dataPtr = dataIn;
	struct CanTxMsg *basePtr = base;
	
	basePtr[offset] = *dataPtr;
}

/*
*********************************************************************************************************
* Function Name : PopSeqQueueTxMsg_CallBack
* Description	: 顺序队列发送消息数据出队
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static __INLINE void PopSeqQueueTxMsg_CallBack( void *base, uint32_t offset, void *dataOut )
{
	struct CanTxMsg *dataPtr = dataOut;
	struct CanTxMsg *basePtr = base;
	
	*dataPtr = basePtr[offset];
}

/*
*********************************************************************************************************
* Function Name : PushSeqQueueTxMsg_CallBack
* Description	: 顺序队列发送消息数据入队
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static __INLINE void PushSeqQueueRxMsg_CallBack( void *base, uint32_t offset, void *dataIn )
{
	struct CanRxMsg *dataPtr = dataIn;
	struct CanRxMsg *basePtr = base;
	
	basePtr[offset] = *dataPtr;
}

/*
*********************************************************************************************************
* Function Name : PopSeqQueueTxMsg_CallBack
* Description	: 顺序队列发送消息数据出队
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static __INLINE void PopSeqQueueRxMsg_CallBack( void *base, uint32_t offset, void *dataOut )
{
	struct CanRxMsg *dataPtr = dataOut;
	struct CanRxMsg *basePtr = base;
	
	*dataPtr = basePtr[offset];
}

/*
*********************************************************************************************************
*                              				Private function prototypes
*********************************************************************************************************
*/
static bool CAN_Init(struct CanDrv *this);
static bool SendMessage(struct CanDrv *this, struct CanTxMsg *TxMessage);
static bool ReceiveMessage(struct CanDrv *this, struct CanRxMsg *RxMessage);
#if (CAN1_EN)
static bool CAN1_InitDone(struct CanDrv *this);
#endif
#if (CAN2_EN)
static bool CAN2_InitDone(struct CanDrv *this);
#endif

/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
#if (CAN1_EN)
static struct CanTxMsg g_CAN1_TxBuff[CAN1_TX_PACKAGE_BUFF_SIZE];
static struct CanRxMsg g_CAN1_RxBuff[CAN1_RX_PACKAGE_BUFF_SIZE];
#endif

#if (CAN2_EN)
static struct CanTxMsg g_CAN2_TxBuff[CAN2_TX_PACKAGE_BUFF_SIZE];
static struct CanRxMsg g_CAN2_RxBuff[CAN2_RX_PACKAGE_BUFF_SIZE];
#endif

static struct CanDrv g_CANDev[] = {
#if (CAN1_EN)
{
	.dev				= DEV_CAN1,
	.handle				= {
		.Instance		= CAN1,
	},
	.txBuffer			= g_CAN1_TxBuff,
	.txBufferSize		= ARRAY_SIZE(g_CAN1_TxBuff),
	.rxBuffer			= g_CAN1_RxBuff,
	.rxBufferSize		= ARRAY_SIZE(g_CAN1_RxBuff),
	.init 				= CAN_Init,
	.initDone			= CAN1_InitDone,
	.send				= SendMessage,
	.receive			= ReceiveMessage,
},
#endif
#if (CAN2_EN)
{
	.dev				= DEV_CAN2,
	.handle				= {
		.Instance		= CAN2,
	},
	.txBuffer			= g_CAN2_TxBuff,
	.txBufferSize		= ARRAY_SIZE(g_CAN2_TxBuff),
	.rxBuffer			= g_CAN2_RxBuff,
	.rxBufferSize		= ARRAY_SIZE(g_CAN2_RxBuff),
	.init 				= CAN_Init,
	.initDone			= CAN2_InitDone,
	.send				= SendMessage,
	.receive			= ReceiveMessage,
},
#endif
};

/*
*********************************************************************************************************
*                              				Private functions
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : GetDevHandle
* Description	: 获取设备句柄
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static struct CanDrv *GetDevHandle(CAN_DevEnum dev)
{
	struct CanDrv *handle = NULL;
	for (uint8_t i=0; i<ARRAY_SIZE(g_CANDev); ++i) {		
		if (dev == g_CANDev[i].dev) {
			handle = &g_CANDev[i];
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
static struct CanDrv *GetHandleByDevice(CAN_HandleTypeDef *handle)
{
	return container_of(handle, struct CanDrv, handle);
}

/*
*********************************************************************************************************
* Function Name : SendMessage
* Description	: 发送消息
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool SendMessage(struct CanDrv *this, struct CanTxMsg *TxMessage)
{
	uint32_t TxMailbox = 0;
	HAL_StatusTypeDef ret = HAL_CAN_AddTxMessage(&this->handle, &TxMessage->txHead, TxMessage->dataBody, &TxMailbox);	
	if (ret != HAL_OK) {
		if (!PushSeqQueue(&this->sendQueue, TxMessage, PushSeqQueueTxMsg_CallBack))
		{
//			ECHO(ECHO_DEBUG, "%s Overflow", __func__);
			return false;
		}
	}
	return true;
}

/*
*********************************************************************************************************
* Function Name : ReceiveMessage
* Description	: 接收消息
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ReceiveMessage(struct CanDrv *this, struct CanRxMsg *RxMessage)
{
	bool status = false;
	if (PopSeqQueue(&this->receiveQueue, RxMessage, PopSeqQueueRxMsg_CallBack) == true)
	{
		status = true;
	}	
	return status;
}

/*
*********************************************************************************************************
* Function Name : CAN_Init
* Description	: CAN初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool CAN_Init(struct CanDrv *this)
{
	CreateSeqQueue(&this->sendQueue, this->txBuffer, this->txBufferSize);
	CreateSeqQueue(&this->receiveQueue, this->rxBuffer, this->rxBufferSize);
	
	CAN_InitTypeDef *initHandle 		= &this->handle.Init; 
	initHandle->TimeTriggeredMode		= DISABLE;				//非时间触发通信模式
	initHandle->AutoBusOff				= ENABLE;				//硬件自动离线管理[ENABLE：总线短路后，解除短路状态仍然可以正常发送]
	initHandle->AutoWakeUp				= ENABLE;				//睡眠模式通过软件唤醒
	initHandle->AutoRetransmission		= ENABLE;				//报文自动重传
	initHandle->ReceiveFifoLocked		= DISABLE;				//报文不锁定,新的覆盖旧的
	initHandle->TransmitFifoPriority	= ENABLE;				//优先级由发送请求的顺序来决定
	initHandle->Mode					= CAN_MODE_NORMAL;
	/** 
	 *	CAN Baudrate = 250KBps
	 *	CAN波特率 = 42M / CAN_Prescaler / (CAN_BS1+CAN_BS2+CAN_SJW)
	 *	根据小软件 <STM32_CANBaudRate.exe> 计算波特率
	 */
	initHandle->SyncJumpWidth			= CAN_SJW_1TQ;
	initHandle->TimeSeg1				= CAN_BS1_12TQ;
	initHandle->TimeSeg2				= CAN_BS2_1TQ;
	initHandle->Prescaler				= 12;
	if (HAL_CAN_DeInit(&this->handle) != HAL_OK) {
		BSP_ErrorHandler();
	}
	if (HAL_CAN_Init(&this->handle) != HAL_OK) {
		BSP_ErrorHandler();
	}
	
	if (this->initDone) {
		bool ret = this->initDone(this);
		if (ret == false) {
			return false;
		}
	}
	
	HAL_StatusTypeDef ret = HAL_CAN_Start(&this->handle);
	return (ret == HAL_OK);
}

#if (CAN1_EN)
/*
*********************************************************************************************************
* Function Name : CAN1_InitDone
* Description	: CAN1初始化完成
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool CAN1_InitDone(struct CanDrv *this)
{	
	/** 
	 *	屏蔽位模式：指定要过滤的位（参考寄存器 (CAN_RIxR ），
	 *	设置为1表示对指定位进行监控，如果不符合则过滤掉。
	 */	
	CAN_FilterTypeDef sFilterConfig = {0};
	sFilterConfig.FilterBank			= 0;
	sFilterConfig.SlaveStartFilterBank	= 14;
	sFilterConfig.FilterMode			= CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale			= CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh			= 0;		
	sFilterConfig.FilterIdLow			= 0;
	sFilterConfig.FilterMaskIdHigh		= 0;	
	sFilterConfig.FilterMaskIdLow		= 0;
	sFilterConfig.FilterFIFOAssignment	= CAN_FILTER_FIFO0;
	sFilterConfig.FilterActivation		= ENABLE;
	HAL_StatusTypeDef ret = HAL_CAN_ConfigFilter(&this->handle, &sFilterConfig);
	
	/* 用于CAN2 */
	sFilterConfig.FilterBank			= 14;
	sFilterConfig.SlaveStartFilterBank	= 14;
	sFilterConfig.FilterFIFOAssignment	= CAN_FILTER_FIFO1;
	ret = HAL_CAN_ConfigFilter(&this->handle, &sFilterConfig);
	
	return (ret == HAL_OK);
}
#endif

#if (CAN2_EN)
/*
*********************************************************************************************************
* Function Name : CAN2_InitDone
* Description	: CAN2初始化完成
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool CAN2_InitDone(struct CanDrv *this)
{
	/** 
	 *	屏蔽位模式：指定要过滤的位（参考寄存器 (CAN_RIxR ），
	 *	设置为1表示对指定位进行监控，如果不符合则过滤掉。
	 */
//	CAN_FilterTypeDef sFilterConfig = {0};
//	sFilterConfig.FilterBank			= 14;
//	sFilterConfig.SlaveStartFilterBank	= 14;
//	sFilterConfig.FilterMode			= CAN_FILTERMODE_IDMASK;
//	sFilterConfig.FilterScale			= CAN_FILTERSCALE_32BIT;
//	sFilterConfig.FilterIdHigh			= 0;		
//	sFilterConfig.FilterIdLow			= 0;
//	sFilterConfig.FilterMaskIdHigh		= 0;	
//	sFilterConfig.FilterMaskIdLow		= 0;
//	sFilterConfig.FilterFIFOAssignment	= CAN_FILTER_FIFO1;
//	sFilterConfig.FilterActivation		= ENABLE;
//	HAL_StatusTypeDef ret = HAL_CAN_ConfigFilter(&this->handle, &sFilterConfig);
//	return (ret == HAL_OK);
	return true;
}
#endif

/*
*********************************************************************************************************
*                              				MSP
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : HAL_CAN_MspInit
* Description	: CAN MSP初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan)
{
	GPIO_InitTypeDef GPIO_InitStructure; 
	
	struct CanDrv *this = GetHandleByDevice(hcan);
	if (hcan->Instance == CAN1)
	{
	#if (CAN1_EN)	
		/* 初始化时钟 */
		CAN1_GPIO_CLK_ENABLE();
		CAN1_CLK_ENABLE();
		
		/* 初始化GPIO-TX */
		GPIO_InitStructure.Pin = CAN1_TX_GPIO_PIN;
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
		GPIO_InitStructure.Alternate = CAN1_TX_AF;
		HAL_GPIO_Init(CAN1_TX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 初始化GPIO-RX */
		GPIO_InitStructure.Pin = CAN1_RX_GPIO_PIN;
		GPIO_InitStructure.Alternate = CAN1_RX_AF;
		HAL_GPIO_Init(CAN1_RX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 设置优先级 */
		HAL_NVIC_SetPriority(CAN1_TX_IRQn, 2, 2);
		HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
		
		HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 2, 1);
		HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
			
		__HAL_CAN_ENABLE_IT(&this->handle, CAN_IT_TX_MAILBOX_EMPTY);
		__HAL_CAN_ENABLE_IT(&this->handle, CAN_IT_RX_FIFO0_MSG_PENDING);
	#endif	
	}
	else if (hcan->Instance == CAN2)
	{
	#if (CAN2_EN)	
		/* 初始化时钟 */
		CAN2_GPIO_CLK_ENABLE();
		CAN2_CLK_ENABLE();
		
		/* 初始化GPIO-TX */
		GPIO_InitStructure.Pin = CAN2_TX_GPIO_PIN;
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
		GPIO_InitStructure.Alternate = CAN2_TX_AF;
		HAL_GPIO_Init(CAN2_TX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 初始化GPIO-RX */
		GPIO_InitStructure.Pin = CAN2_RX_GPIO_PIN;
		GPIO_InitStructure.Alternate = CAN2_RX_AF;
		HAL_GPIO_Init(CAN2_RX_GPIO_PORT, &GPIO_InitStructure);
		
		/* 设置优先级 */
		HAL_NVIC_SetPriority(CAN2_TX_IRQn, 2, 2);
		HAL_NVIC_EnableIRQ(CAN2_TX_IRQn);
		
		HAL_NVIC_SetPriority(CAN2_RX1_IRQn, 2, 1);
		HAL_NVIC_EnableIRQ(CAN2_RX1_IRQn);
				
		__HAL_CAN_ENABLE_IT(&this->handle, CAN_IT_TX_MAILBOX_EMPTY);
		__HAL_CAN_ENABLE_IT(&this->handle, CAN_IT_RX_FIFO1_MSG_PENDING);
	#endif	
	}
}

/*
*********************************************************************************************************
*                              				中断服务函数
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : SendMsgHandler
* Description	: 发送消息处理
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void SendMsgHandler(CAN_HandleTypeDef *hcan)
{
	struct CanDrv *this = GetHandleByDevice(hcan);
	
	struct CanTxMsg msg = {0};
	if (PopSeqQueue(&this->sendQueue, &msg, PopSeqQueueTxMsg_CallBack)) {
		SendMessage(this, &msg);
	}
}

/*
*********************************************************************************************************
* Function Name : HAL_CAN_TxMailbox0CompleteCallback
* Description	: CAN发送邮箱0完成
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
	SendMsgHandler(hcan);
}

/*
*********************************************************************************************************
* Function Name : HAL_CAN_TxMailbox1CompleteCallback
* Description	: CAN发送邮箱1完成
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
	SendMsgHandler(hcan);
}

/*
*********************************************************************************************************
* Function Name : HAL_CAN_TxMailbox2CompleteCallback
* Description	: CAN发送邮箱2完成
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
	SendMsgHandler(hcan);
}

/*
*********************************************************************************************************
* Function Name : ReceiveMsgHandler
* Description	: 接收消息处理
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void ReceiveMsgHandler(CAN_HandleTypeDef *hcan, uint32_t RxFifo)
{
	struct CanDrv *this = GetHandleByDevice(hcan);
	struct CanRxMsg rxMsg = {0};
	if (HAL_CAN_GetRxMessage(&this->handle, RxFifo, &rxMsg.rxHead, rxMsg.dataBody) == HAL_OK) {
		if (PushSeqQueue(&this->receiveQueue, &rxMsg, PushSeqQueueRxMsg_CallBack) == false)
		{
			ECHO(ECHO_DEBUG, "%s Overflow", __func__);
		}
	}
}
	
/*
*********************************************************************************************************
* Function Name : HAL_CAN_RxFifo0MsgPendingCallback
* Description	: CAN接收邮箱挂起
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	ReceiveMsgHandler(hcan, CAN_RX_FIFO0);
}

/*
*********************************************************************************************************
* Function Name : HAL_CAN_RxFifo1MsgPendingCallback
* Description	: CAN接收邮箱挂起
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	ReceiveMsgHandler(hcan, CAN_RX_FIFO1);
}

/*
*********************************************************************************************************
* Function Name : HAL_CAN_ErrorCallback
* Description	: CAN错误处理程序
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
//	ECHO(ECHO_DEBUG, "%s [Err] %08X", __func__, hcan->ErrorCode);
	HAL_CAN_ResetError(hcan);
}

#if (CAN1_EN)
/*
*********************************************************************************************************
* Function Name : CAN1_TX_IRQHandler
* Description	: CAN1发送中断服务函数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void CAN1_TX_IRQHandler(void)
{
	struct CanDrv *this = GetDevHandle(DEV_CAN1);
	HAL_CAN_IRQHandler(&this->handle);	
}

/*
*********************************************************************************************************
* Function Name : CAN1_RX0_IRQHandler
* Description	: CAN1接收中断服务函数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void CAN1_RX0_IRQHandler(void)
{		
	struct CanDrv *this = GetDevHandle(DEV_CAN1);
	HAL_CAN_IRQHandler(&this->handle);	
}
#endif

#if (CAN2_EN)
/*
*********************************************************************************************************
* Function Name : CAN2_TX_IRQHandler
* Description	: CAN2发送中断服务函数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void CAN2_TX_IRQHandler(void)
{		
	struct CanDrv *this = GetDevHandle(DEV_CAN2);
	HAL_CAN_IRQHandler(&this->handle);	
}

/*
*********************************************************************************************************
* Function Name : CAN2_RX0_IRQHandler
* Description	: CAN2接收中断服务函数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void CAN2_RX0_IRQHandler(void)
{		
	struct CanDrv *this = GetDevHandle(DEV_CAN2);
	HAL_CAN_IRQHandler(&this->handle);
}

/*
*********************************************************************************************************
* Function Name : CAN2_RX1_IRQHandler
* Description	: CAN2接收中断服务函数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void CAN2_RX1_IRQHandler(void)
{		
	struct CanDrv *this = GetDevHandle(DEV_CAN2);
	HAL_CAN_IRQHandler(&this->handle);	
}
#endif

/*
*********************************************************************************************************
*                              				API
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_InitCANBus
* Description	: 初始化CAN总线
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitCANBus(void)
{
	for (uint8_t i=0; i<ARRAY_SIZE(g_CANDev); ++i) {
		if (g_CANDev[i].init) {
			bool ret = g_CANDev[i].init(&g_CANDev[i]);
			ECHO(ECHO_DEBUG, "[BSP] CAN %d 初始化 .......... %s", g_CANDev[i].dev+1, (ret == true) ? "OK" : "ERROR");
		}
	}
}

/*
*********************************************************************************************************
* Function Name : CAN_SendMessage
* Description	: 发送消息
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool CAN_SendMessage(CAN_DevEnum dev, struct CanTxMsg *txMessage)
{
	struct  CanDrv *handle = GetDevHandle(dev);
	if (handle == NULL) {
		return false;
	}
	
	return handle->send(handle, txMessage);
}

/*
*********************************************************************************************************
* Function Name : CAN_ReceiveMessage
* Description	: 接收消息
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool CAN_ReceiveMessage(CAN_DevEnum dev, struct CanRxMsg *rxMessage)
{
	struct CanDrv *handle = GetDevHandle(dev);
	if (handle == NULL) {
		return false;
	}
	return handle->receive(handle, rxMessage);
}

/*
*********************************************************************************************************
* Function Name : CAN_HasData
* Description	: CAN有数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool CAN_HasData(CAN_DevEnum dev)
{
	struct CanDrv *this = GetDevHandle(dev);
	if (this == NULL) {
		return false;
	}
	return (!SeqQueueIsEmpty(&this->receiveQueue));
}


/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
