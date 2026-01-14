/*
*********************************************************************************************************
* @file    	bsp_spi_bus.c
* @author  	SY
* @version 	V1.0.0
* @date    	2016-7-10 09:42:53
* @IDE	 	Keil V4.72.10.0
* @Chip    	STM32F407VE
* @brief   	SPI总线源文件
*********************************************************************************************************
* @attention
*		对SPI总线进行统一管理。
* 		注意芯片SETUP的选择。
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
#define USE_REG						(true)

#define SPI1_EN						(false)
#if (SPI1_EN)
#define SPI1_CLK_ENABLE()			do {\
										__HAL_RCC_SPI1_CLK_ENABLE();\
									} while (0)
#define SPI1_BAUD					SPI_BAUDRATEPRESCALER_2
#define SPI1_AF						GPIO_AF5_SPI1
#define SPI1_RW_TIMEOUT_MS			(5)
#define SPI1_GPIO_CLK_ENABLE() 		do {\
										__HAL_RCC_GPIOA_CLK_ENABLE();\
										__HAL_RCC_GPIOB_CLK_ENABLE();\
									} while (0)
#define	SPI1_MISO_GPIO_PORT 		GPIOA
#define	SPI1_MISO_GPIO_PIN			GPIO_PIN_6
#define	SPI1_MOSI_GPIO_PORT 		GPIOA
#define	SPI1_MOSI_GPIO_PIN			GPIO_PIN_7
#define	SPI1_SCK_GPIO_PORT 			GPIOA
#define	SPI1_SCK_GPIO_PIN			GPIO_PIN_5
#endif							
									
#define SPI2_EN						(true)
#if (SPI2_EN)
#define SPI2_CLK_ENABLE()			do {\
										__HAL_RCC_SPI2_CLK_ENABLE();\
									} while (0)
#define SPI2_BAUD					SPI_BAUDRATEPRESCALER_32
#define SPI2_AF						GPIO_AF5_SPI2
#define SPI2_RW_TIMEOUT_MS			(5)
#define SPI2_GPIO_CLK_ENABLE() 		do {\
										__HAL_RCC_GPIOB_CLK_ENABLE();\
										__HAL_RCC_GPIOC_CLK_ENABLE();\
									} while (0)
#define	SPI2_MISO_GPIO_PORT 		GPIOC
#define	SPI2_MISO_GPIO_PIN			GPIO_PIN_2
#define	SPI2_MOSI_GPIO_PORT 		GPIOC
#define	SPI2_MOSI_GPIO_PIN			GPIO_PIN_3
#define	SPI2_SCK_GPIO_PORT 			GPIOB
#define	SPI2_SCK_GPIO_PIN			GPIO_PIN_10
#endif
									
#define SPI3_EN						(true)
#if (SPI3_EN)
#define SPI3_CLK_ENABLE()			do {\
										__HAL_RCC_SPI3_CLK_ENABLE();\
									} while (0)
#define SPI3_BAUD					SPI_BAUDRATEPRESCALER_2
#define SPI3_AF						GPIO_AF6_SPI3
#define SPI3_RW_TIMEOUT_MS			(5)
#define SPI3_GPIO_CLK_ENABLE() 		do {\
										__HAL_RCC_GPIOC_CLK_ENABLE();\
									} while (0)
#define	SPI3_MISO_GPIO_PORT 		GPIOC
#define	SPI3_MISO_GPIO_PIN			GPIO_PIN_11
#define	SPI3_MOSI_GPIO_PORT 		GPIOC
#define	SPI3_MOSI_GPIO_PIN			GPIO_PIN_12
#define	SPI3_SCK_GPIO_PORT 			GPIOC
#define	SPI3_SCK_GPIO_PIN			GPIO_PIN_10
#endif
									
/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/
struct SPIDrv {
	SPI_DevEnum dev;					//设备
	SPI_HandleTypeDef handle;
	uint32_t rwTimeout;
	uint32_t BaudRatePrescaler;
	DMA_HandleTypeDef dma_tx;
	DMA_HandleTypeDef dma_rx;
	
	bool (*init)(struct SPIDrv *this);
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
*                              				Private function prototypes
*********************************************************************************************************
*/
static bool SPI_Init(struct SPIDrv *this);
static bool SPI_Init_Flash(struct SPIDrv *this);

/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
static struct SPIDrv g_SPIDev[] = {
#if (SPI1_EN)
{
	.dev					= DEV_SPI1,
	.handle					= {
		.Instance			= SPI1,
	},
	.rwTimeout				= SPI1_RW_TIMEOUT_MS,
	.BaudRatePrescaler		= SPI1_BAUD,
	.init					= SPI_Init,
},
#endif

#if (SPI2_EN)
{
	.dev					= DEV_SPI2,
	.handle					= {
		.Instance			= SPI2,
	},
	.rwTimeout				= SPI2_RW_TIMEOUT_MS,
	.BaudRatePrescaler		= SPI2_BAUD,
	.init					= SPI_Init,
},
#endif

#if (SPI3_EN)
{
	.dev					= DEV_SPI3,
	.handle					= {
		.Instance			= SPI3,
	},
	.rwTimeout				= SPI3_RW_TIMEOUT_MS,
	.BaudRatePrescaler		= SPI3_BAUD,
	.init					= SPI_Init_Flash,
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
static __INLINE struct SPIDrv *GetDevHandle(SPI_DevEnum dev)
{
	struct SPIDrv *handle = NULL;	
	for (uint8_t i=0; i<ARRAY_SIZE(g_SPIDev); ++i) {		
		if (dev == g_SPIDev[i].dev) {
			handle = &g_SPIDev[i];
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
static struct SPIDrv *GetHandleByDevice(SPI_HandleTypeDef *handle)
{
	return container_of(handle, struct SPIDrv, handle);
}

/*
*********************************************************************************************************
* Function Name : SPI_Init
* Description	: SPI初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool SPI_Init(struct SPIDrv *this)
{
	SPI_InitTypeDef *initHandle = &this->handle.Init;
	initHandle->Mode = SPI_MODE_MASTER;
	initHandle->Direction = SPI_DIRECTION_2LINES;
	initHandle->DataSize = SPI_DATASIZE_8BIT;
	initHandle->CLKPolarity = SPI_POLARITY_LOW;
	initHandle->CLKPhase = SPI_PHASE_1EDGE;
	initHandle->NSS = SPI_NSS_SOFT;
	initHandle->BaudRatePrescaler = this->BaudRatePrescaler;
	initHandle->FirstBit = SPI_FIRSTBIT_MSB;
	initHandle->TIMode = SPI_TIMODE_DISABLE;
	initHandle->CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	initHandle->CRCPolynomial = 7;
	if (HAL_SPI_DeInit(&this->handle) != HAL_OK) {
		BSP_ErrorHandler();
	}
	if (HAL_SPI_Init(&this->handle) != HAL_OK) {
		BSP_ErrorHandler();
	}
	__HAL_SPI_ENABLE(&this->handle);
	return true;
}

/*
*********************************************************************************************************
* Function Name : SPI_Init_Flash
* Description	: SPI初始化FLASH
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool SPI_Init_Flash(struct SPIDrv *this)
{
	SPI_InitTypeDef *initHandle = &this->handle.Init;
	initHandle->Mode = SPI_MODE_MASTER;
	initHandle->Direction = SPI_DIRECTION_2LINES;
	initHandle->DataSize = SPI_DATASIZE_8BIT;
	initHandle->CLKPolarity = SPI_POLARITY_HIGH;
	initHandle->CLKPhase = SPI_PHASE_2EDGE;
	initHandle->NSS = SPI_NSS_SOFT;
	initHandle->BaudRatePrescaler = this->BaudRatePrescaler;
	initHandle->FirstBit = SPI_FIRSTBIT_MSB;
	initHandle->TIMode = SPI_TIMODE_DISABLE;
	initHandle->CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	initHandle->CRCPolynomial = 7;
	if (HAL_SPI_DeInit(&this->handle) != HAL_OK) {
		BSP_ErrorHandler();
	}
	if (HAL_SPI_Init(&this->handle) != HAL_OK) {
		BSP_ErrorHandler();
	}
	__HAL_SPI_ENABLE(&this->handle);
	return true;
}

/*
*********************************************************************************************************
*                              				MSP
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : HAL_SPI_MspInit
* Description	: SPI MSP初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
	struct SPIDrv *this = GetHandleByDevice(hspi);
	
	if (this->dev == DEV_SPI1)
	{
	#if (SPI1_EN)
		SPI1_GPIO_CLK_ENABLE();
		SPI1_CLK_ENABLE();
		
		GPIO_InitTypeDef GPIO_InitStructure;
		GPIO_InitStructure.Pin = SPI1_MISO_GPIO_PIN;
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Alternate = SPI1_AF;		
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
		HAL_GPIO_Init(SPI1_MISO_GPIO_PORT, &GPIO_InitStructure);
		
		GPIO_InitStructure.Pin = SPI1_MOSI_GPIO_PIN;
		HAL_GPIO_Init(SPI1_MOSI_GPIO_PORT, &GPIO_InitStructure);
		
		GPIO_InitStructure.Pin = SPI1_SCK_GPIO_PIN;
		HAL_GPIO_Init(SPI1_SCK_GPIO_PORT, &GPIO_InitStructure);
		
		{
			__HAL_RCC_DMA2_CLK_ENABLE();
			HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
			HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
	
			this->dma_tx.Instance = DMA2_Stream3;
			this->dma_tx.Init.Channel = DMA_CHANNEL_3;
			this->dma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
			this->dma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
			this->dma_tx.Init.MemInc = DMA_MINC_ENABLE;
			this->dma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
			this->dma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
			this->dma_tx.Init.Mode = DMA_NORMAL;
			this->dma_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
			this->dma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
			
			if(HAL_DMA_Init(&this->dma_tx) != HAL_OK)
			{
				/* Initialization Error */
				BSP_ErrorHandler();  
			}	
			__HAL_LINKDMA(hspi, hdmatx, this->dma_tx);	
		}
		{
			__HAL_RCC_DMA2_CLK_ENABLE();
			HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
			HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
	
			this->dma_rx.Instance = DMA2_Stream2;
			this->dma_rx.Init.Channel = DMA_CHANNEL_3;
			this->dma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
			this->dma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
			this->dma_rx.Init.MemInc = DMA_MINC_ENABLE;
			this->dma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
			this->dma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
			this->dma_rx.Init.Mode = DMA_NORMAL;
			this->dma_rx.Init.Priority = DMA_PRIORITY_HIGH;
			this->dma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
			if(HAL_DMA_Init(&this->dma_rx) != HAL_OK)
			{
				/* Initialization Error */
				BSP_ErrorHandler();  
			}	
			__HAL_LINKDMA(hspi, hdmarx, this->dma_rx);
		}
	#endif	
	}
	else if (this->dev == DEV_SPI2)
	{
	#if (SPI2_EN)
		SPI2_GPIO_CLK_ENABLE();
		SPI2_CLK_ENABLE();
		
		GPIO_InitTypeDef GPIO_InitStructure;
		GPIO_InitStructure.Pin = SPI2_MISO_GPIO_PIN;
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Alternate = SPI2_AF;		
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
		HAL_GPIO_Init(SPI2_MISO_GPIO_PORT, &GPIO_InitStructure);
		
		GPIO_InitStructure.Pin = SPI2_MOSI_GPIO_PIN;
		HAL_GPIO_Init(SPI2_MOSI_GPIO_PORT, &GPIO_InitStructure);
		
		GPIO_InitStructure.Pin = SPI2_SCK_GPIO_PIN;
		HAL_GPIO_Init(SPI2_SCK_GPIO_PORT, &GPIO_InitStructure);
	#endif
	}
	else if (this->dev == DEV_SPI3)
	{
	#if (SPI3_EN)
		SPI3_GPIO_CLK_ENABLE();
		SPI3_CLK_ENABLE();
		
		GPIO_InitTypeDef GPIO_InitStructure;
		GPIO_InitStructure.Pin = SPI3_MISO_GPIO_PIN;
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Alternate = SPI3_AF;		
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
		HAL_GPIO_Init(SPI3_MISO_GPIO_PORT, &GPIO_InitStructure);
		
		GPIO_InitStructure.Pin = SPI3_MOSI_GPIO_PIN;
		HAL_GPIO_Init(SPI3_MOSI_GPIO_PORT, &GPIO_InitStructure);
		
		GPIO_InitStructure.Pin = SPI3_SCK_GPIO_PIN;
		HAL_GPIO_Init(SPI3_SCK_GPIO_PORT, &GPIO_InitStructure);
	#endif
	}
}
#if (SPI1_EN)
/**
* @brief This function handles DMA2 stream2 global interrupt.
*/
void DMA2_Stream2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream2_IRQn 0 */

  /* USER CODE END DMA2_Stream2_IRQn 0 */
  HAL_DMA_IRQHandler(&g_SPIDev[0].dma_rx);
  /* USER CODE BEGIN DMA2_Stream2_IRQn 1 */

  /* USER CODE END DMA2_Stream2_IRQn 1 */
}

/**
* @brief This function handles DMA2 Stream3 global interrupt.
*/
void DMA2_Stream3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA2_Stream3_IRQn 0 */
  /* USER CODE END DMA2_Stream3_IRQn 0 */
  
  HAL_DMA_IRQHandler(&g_SPIDev[0].dma_tx);
 
  /* USER CODE BEGIN DMA2_Stream3_IRQn 1 */
  /* USER CODE END DMA2_Stream3_IRQn 1 */
}
#endif
/*
*********************************************************************************************************
*                              				API
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_InitSPIBus
* Description	: 初始化SPI总线
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitSPIBus(void)
{
	for (uint8_t i=0; i<ARRAY_SIZE(g_SPIDev); ++i) {
		if (g_SPIDev[i].init) {
			bool ret = g_SPIDev[i].init(&g_SPIDev[i]);
			ECHO(ECHO_DEBUG, "[BSP] SPI %d 初始化 .......... %s", g_SPIDev[i].dev+1, (ret == true) ? "OK" : "ERROR");
		}
	}
}

/*
*********************************************************************************************************
* Function Name : SPI_ReadByte
* Description	: SPI读取数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
uint8_t SPI_ReadByte(SPI_DevEnum dev)
{
	struct SPIDrv *this = GetDevHandle(dev);	
#if (USE_REG)
	while ((this->handle.Instance->SR & SPI_SR_TXE) == RESET);
	this->handle.Instance->DR = 0xFF;
	while ((this->handle.Instance->SR & SPI_SR_RXNE) == RESET);
	return this->handle.Instance->DR;
#else
	uint8_t rdData = 0;
	if (HAL_SPI_Receive(&this->handle, &rdData, sizeof(rdData), this->rwTimeout) != HAL_OK) {
		BSP_ErrorHandler();
	}
	return rdData;
#endif	
}

/*
*********************************************************************************************************
* Function Name : SPI_WriteByte
* Description	: SPI写数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void SPI_WriteByte(SPI_DevEnum dev, uint8_t _ucValue)
{
	struct SPIDrv *this = GetDevHandle(dev);
#if (USE_REG)	
	while ((this->handle.Instance->SR & SPI_SR_TXE) == RESET);
	this->handle.Instance->DR = _ucValue;
	while ((this->handle.Instance->SR & SPI_SR_RXNE) == RESET);
	this->handle.Instance->DR;
#else
	if (HAL_SPI_Transmit(&this->handle, &_ucValue, sizeof(_ucValue), this->rwTimeout) != HAL_OK) {
		BSP_ErrorHandler();
	}
#endif	
}

/*
*********************************************************************************************************
* Function Name : SPI_WriteByte1
* Description	: SPI写数据1
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
uint8_t SPI_WriteByte1(SPI_DevEnum dev, uint8_t _ucValue)
{
	uint8_t value = 0xFF;
	struct SPIDrv *this = GetDevHandle(dev);
#if (USE_REG)	
	while ((this->handle.Instance->SR & SPI_SR_TXE) == RESET);
	this->handle.Instance->DR = _ucValue;
	while ((this->handle.Instance->SR & SPI_SR_RXNE) == RESET);
	value = this->handle.Instance->DR;
#else
	if (HAL_SPI_Transmit(&this->handle, &_ucValue, sizeof(_ucValue), this->rwTimeout) != HAL_OK) {
		BSP_ErrorHandler();
	}
#endif	
	return value;
}

/*
*********************************************************************************************************
* Function Name : SPI_WriteBytes
* Description	: SPI写数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void SPI_WriteBytes(SPI_DevEnum dev, uint8_t *data, uint32_t length)
{
	struct SPIDrv *this = GetDevHandle(dev);
#if (USE_REG)
	if ((dev == DEV_SPI1) && (length >= 10)) {
		HAL_SPI_Transmit_DMA(&this->handle, data, length);	
		while (HAL_SPI_GetState(&this->handle) != HAL_SPI_STATE_READY);			
	} else {	
		for (uint32_t i=0; i<length; ++i) {
			while ((this->handle.Instance->SR & SPI_SR_TXE) == RESET);
			this->handle.Instance->DR = *(data + i);
			while ((this->handle.Instance->SR & SPI_SR_RXNE) == RESET);
			this->handle.Instance->DR;
		}
	}
#else
	if (HAL_SPI_Transmit(&this->handle, data, length, this->rwTimeout) != HAL_OK) {
		BSP_ErrorHandler();
	}
#endif	
}

/*
*********************************************************************************************************
* Function Name : SPI_ReadByte
* Description	: SPI读取数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void SPI_ReadBytes(SPI_DevEnum dev, uint8_t *data, uint32_t length)
{
	struct SPIDrv *this = GetDevHandle(dev);
#if (USE_REG)
	if ((dev == DEV_SPI1) && (length >= 10)) {		
		HAL_SPI_Receive_DMA(&this->handle, data, length);
		while (HAL_SPI_GetState(&this->handle) != HAL_SPI_STATE_READY);
	} else {
		for (uint32_t i=0; i<length; ++i) {
			while ((this->handle.Instance->SR & SPI_SR_TXE) == RESET);
			this->handle.Instance->DR = 0xFF;
			while ((this->handle.Instance->SR & SPI_SR_RXNE) == RESET);
			uint8_t in = this->handle.Instance->DR;
			*(data + i) = in;
		}
	}
#else
	if (HAL_SPI_Receive(&this->handle, data, length, this->rwTimeout) != HAL_OK) {
		BSP_ErrorHandler();
	}
#endif	
}

/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
