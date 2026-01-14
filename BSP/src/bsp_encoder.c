/*
*********************************************************************************************************
* @file    	bsp_encoder.c
* @author  	SY
* @version 	V1.0.0
* @date    	2016-12-15 20:08:56
* @IDE	 	Keil V5.22.0.0
* @Chip    	STM32F407VE
* @brief   	采集编码器位移驱动源文件
*********************************************************************************************************
* @attention
*
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
#define USE_ENCODER1						(true)
#if (USE_ENCODER1)
	#define ENCODER1_TIMER					TIM1
	#define	ENCODER1_TIM_CLK_ENABLE()			do {\
												__HAL_RCC_TIM1_CLK_ENABLE();\
											} while(0)
	
	#define	ENCODER1_GPIO_CLK_ENABLE()		do {\
												__HAL_RCC_GPIOA_CLK_ENABLE();\
											} while(0)
	#define	ENCODER1_GPIO_PA_PULSE_PORT  	GPIOA
	#define	ENCODER1_GPIO_PA_PULSE_PIN		GPIO_PIN_8
	#define	ENCODER1_GPIO_PB_PULSE_PORT  	GPIOA
	#define	ENCODER1_GPIO_PB_PULSE_PIN		GPIO_PIN_9
	#define ENCODER1_GPIO_AF				GPIO_AF1_TIM1
#endif

#define USE_ENCODER2						(true)
#if (USE_ENCODER2)
	#define ENCODER2_TIMER					TIM3
	#define	ENCODER2_TIM_CLK_ENABLE()			do {\
												__HAL_RCC_TIM3_CLK_ENABLE();\
											} while(0)
	
	#define	ENCODER2_GPIO_CLK_ENABLE()		do {\
												__HAL_RCC_GPIOB_CLK_ENABLE();\
											} while(0)
	#define	ENCODER2_GPIO_PA_PULSE_PORT  	GPIOB
	#define	ENCODER2_GPIO_PA_PULSE_PIN		GPIO_PIN_4
	#define	ENCODER2_GPIO_PB_PULSE_PORT  	GPIOB
	#define	ENCODER2_GPIO_PB_PULSE_PIN		GPIO_PIN_5
	#define ENCODER2_GPIO_AF				GPIO_AF2_TIM3
#endif
											
#define USE_ENCODER3						(true)
#if (USE_ENCODER3)
	#define ENCODER3_TIMER					TIM4
	#define	ENCODER3_TIM_CLK_ENABLE()			do {\
												__HAL_RCC_TIM4_CLK_ENABLE();\
											} while(0)
	
	#define	ENCODER3_GPIO_CLK_ENABLE()		do {\
												__HAL_RCC_GPIOD_CLK_ENABLE();\
											} while(0)
	#define	ENCODER3_GPIO_PA_PULSE_PORT  	GPIOD
	#define	ENCODER3_GPIO_PA_PULSE_PIN		GPIO_PIN_12
	#define	ENCODER3_GPIO_PB_PULSE_PORT  	GPIOD
	#define	ENCODER3_GPIO_PB_PULSE_PIN		GPIO_PIN_13
	#define ENCODER3_GPIO_AF				GPIO_AF2_TIM4
#endif		

#define USE_ENCODER4						(true)
#if (USE_ENCODER4)
	#define ENCODER4_TIMER					TIM8
	#define	ENCODER4_TIM_CLK_ENABLE()			do {\
												__HAL_RCC_TIM8_CLK_ENABLE();\
											} while(0)
	
	#define	ENCODER4_GPIO_CLK_ENABLE()		do {\
												__HAL_RCC_GPIOC_CLK_ENABLE();\
											} while(0)
	#define	ENCODER4_GPIO_PA_PULSE_PORT  	GPIOC
	#define	ENCODER4_GPIO_PA_PULSE_PIN		GPIO_PIN_6
	#define	ENCODER4_GPIO_PB_PULSE_PORT  	GPIOC
	#define	ENCODER4_GPIO_PB_PULSE_PIN		GPIO_PIN_7
	#define ENCODER4_GPIO_AF				GPIO_AF3_TIM8
#endif											

/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/
struct Encoding_Drv {
	ENCODER_DevEnum dev;
	TIM_HandleTypeDef handle;
	int32_t value;
	int16_t reg;
	
	bool (*init)(struct Encoding_Drv *this);
	int32_t (*read)(struct Encoding_Drv *this);
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
static bool __Init(struct Encoding_Drv *this);
static int32_t __Read(struct Encoding_Drv *this);

/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
struct Encoding_Drv g_EncoderDev[] = {
#if (USE_ENCODER1)
{
	.dev				= DEV_ENCODER1,
	.handle				= {
		.Instance		= ENCODER1_TIMER,
	},
	.init				= __Init,
	.read				= __Read,
},
#endif

#if (USE_ENCODER2)
{
	.dev				= DEV_ENCODER2,
	.handle				= {
		.Instance		= ENCODER2_TIMER,
	},
	.init				= __Init,
	.read				= __Read,
},
#endif

#if (USE_ENCODER3)
{
	.dev				= DEV_ENCODER3,
	.handle				= {
		.Instance		= ENCODER3_TIMER,
	},
	.init				= __Init,
	.read				= __Read,
},
#endif

#if (USE_ENCODER4)
{
	.dev				= DEV_ENCODER4,
	.handle				= {
		.Instance		= ENCODER4_TIMER,
	},
	.init				= __Init,
	.read				= __Read,
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
static struct Encoding_Drv *GetDevHandle(ENCODER_DevEnum dev)
{
	struct Encoding_Drv *handle = NULL;
	
	for (uint8_t i=0; i<ARRAY_SIZE(g_EncoderDev); ++i) {		
		if (dev == g_EncoderDev[i].dev) {
			handle = &g_EncoderDev[i];
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
static struct Encoding_Drv *GetEncodeHandleByDevice(TIM_HandleTypeDef *handle)
{
	return container_of(handle, struct Encoding_Drv, handle);
}

/*
*********************************************************************************************************
* Function Name : __Init
* Description	: 硬件初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool __Init(struct Encoding_Drv *this)
{
	/* 基本定时器 */
	TIM_Base_InitTypeDef *timBaseInit = &this->handle.Init;
	timBaseInit->Period = 0xffff;
	timBaseInit->Prescaler = 0;
	timBaseInit->ClockDivision = TIM_CLOCKDIVISION_DIV1;
	timBaseInit->CounterMode = TIM_COUNTERMODE_UP;
	timBaseInit->RepetitionCounter = 0;
	
	TIM_Encoder_InitTypeDef sConfig = {0};
	sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
	sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
	sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
	sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
	sConfig.IC1Filter = 0;
	sConfig.IC2Polarity = TIM_ICPOLARITY_FALLING;
	sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
	sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
	sConfig.IC2Filter = 0;	
	if (HAL_TIM_Encoder_DeInit(&this->handle) != HAL_OK) {
		BSP_ErrorHandler();
	}
	if (HAL_TIM_Encoder_Init(&this->handle, &sConfig) != HAL_OK) {
		BSP_ErrorHandler();
	}
		
	//标志位上电后置位
	__HAL_TIM_CLEAR_FLAG(&this->handle, TIM_FLAG_UPDATE);
	__HAL_TIM_SET_COUNTER(&this->handle, 0);
	__HAL_TIM_ENABLE(&this->handle);
	return true;
}

/*
*********************************************************************************************************
*                              				MSP
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : HAL_TIM_Encoder_MspInit
* Description	: 编码器 MSP初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_TIM_Encoder_MspInit(TIM_HandleTypeDef *htim)
{
	struct Encoding_Drv *encodeHandle = GetEncodeHandleByDevice(htim);
	switch (encodeHandle->dev)
	{
		case DEV_ENCODER1:
		{
		#if (USE_ENCODER1)
			/* GPIO Clock */
			ENCODER1_GPIO_CLK_ENABLE();

			/* Config GPIO */
			GPIO_InitTypeDef GPIO_InitStructure = {0}; 
			GPIO_InitStructure.Pin = ENCODER1_GPIO_PA_PULSE_PIN;
			GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
			GPIO_InitStructure.Pull = GPIO_NOPULL;
			GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
			GPIO_InitStructure.Alternate = ENCODER1_GPIO_AF;
			HAL_GPIO_Init(ENCODER1_GPIO_PA_PULSE_PORT, &GPIO_InitStructure);
			
			GPIO_InitStructure.Pin 	= ENCODER1_GPIO_PB_PULSE_PIN;
			HAL_GPIO_Init(ENCODER1_GPIO_PB_PULSE_PORT, &GPIO_InitStructure);	
			
			/* TIM Clock */
			ENCODER1_TIM_CLK_ENABLE();
		#endif	
			break;
		}
		case DEV_ENCODER2:
		{
		#if (USE_ENCODER2)
			/* GPIO Clock */
			ENCODER2_GPIO_CLK_ENABLE();

			/* Config GPIO */
			GPIO_InitTypeDef GPIO_InitStructure = {0}; 
			GPIO_InitStructure.Pin = ENCODER2_GPIO_PA_PULSE_PIN;
			GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
			GPIO_InitStructure.Pull = GPIO_NOPULL;
			GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
			GPIO_InitStructure.Alternate = ENCODER2_GPIO_AF;
			HAL_GPIO_Init(ENCODER2_GPIO_PA_PULSE_PORT, &GPIO_InitStructure);
			
			GPIO_InitStructure.Pin 	= ENCODER2_GPIO_PB_PULSE_PIN;
			HAL_GPIO_Init(ENCODER2_GPIO_PB_PULSE_PORT, &GPIO_InitStructure);	
			
			/* TIM Clock */
			ENCODER2_TIM_CLK_ENABLE();
		#endif	
			break;
		}
		case DEV_ENCODER3:
		{
		#if (USE_ENCODER3)
			/* GPIO Clock */
			ENCODER3_GPIO_CLK_ENABLE();

			/* Config GPIO */
			GPIO_InitTypeDef GPIO_InitStructure = {0}; 
			GPIO_InitStructure.Pin = ENCODER3_GPIO_PA_PULSE_PIN;
			GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
			GPIO_InitStructure.Pull = GPIO_NOPULL;
			GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
			GPIO_InitStructure.Alternate = ENCODER3_GPIO_AF;
			HAL_GPIO_Init(ENCODER3_GPIO_PA_PULSE_PORT, &GPIO_InitStructure);
			
			GPIO_InitStructure.Pin 	= ENCODER3_GPIO_PB_PULSE_PIN;
			HAL_GPIO_Init(ENCODER3_GPIO_PB_PULSE_PORT, &GPIO_InitStructure);	
			
			/* TIM Clock */
			ENCODER3_TIM_CLK_ENABLE();
		#endif	
			break;
		}
		case DEV_ENCODER4:
		{
		#if (USE_ENCODER4)
			/* GPIO Clock */
			ENCODER4_GPIO_CLK_ENABLE();

			/* Config GPIO */
			GPIO_InitTypeDef GPIO_InitStructure = {0}; 
			GPIO_InitStructure.Pin = ENCODER4_GPIO_PA_PULSE_PIN;
			GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
			GPIO_InitStructure.Pull = GPIO_NOPULL;
			GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
			GPIO_InitStructure.Alternate = ENCODER4_GPIO_AF;
			HAL_GPIO_Init(ENCODER4_GPIO_PA_PULSE_PORT, &GPIO_InitStructure);
			
			GPIO_InitStructure.Pin 	= ENCODER4_GPIO_PB_PULSE_PIN;
			HAL_GPIO_Init(ENCODER4_GPIO_PB_PULSE_PORT, &GPIO_InitStructure);	
			
			/* TIM Clock */
			ENCODER4_TIM_CLK_ENABLE();
		#endif	
			break;
		}
		default:
			break;
	}
}

/*
*********************************************************************************************************
* Function Name : __Read
* Description	: 读取编码器
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static int32_t __Read(struct Encoding_Drv *this)
{  
	int16_t cnt = __HAL_TIM_GET_COUNTER(&this->handle); 
	int16_t diff = cnt - this->reg;
	this->reg = cnt;
	this->value += diff;
	return this->value;
}

/*
*********************************************************************************************************
*                              				API
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_InitEncoder
* Description	: 编码器采集初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitEncoder(void)
{
	for (uint8_t i=0; i<ARRAY_SIZE(g_EncoderDev); ++i) {
		if (g_EncoderDev[i].init) {
			bool ret = g_EncoderDev[i].init(&g_EncoderDev[i]);
			ECHO(ECHO_DEBUG, "[BSP] ENCODER %d 初始化 .......... %s", g_EncoderDev[i].dev+1, (ret == true) ? "OK" : "ERROR");
		}
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_EncoderHandler
* Description	: 编码器处理
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_EncoderHandler(ENCODER_DevEnum dev)
{
	struct Encoding_Drv *handle = GetDevHandle(dev);
	if (handle == NULL) {
		return false;
	}
	handle->read(handle);
	return true;
}

/*
*********************************************************************************************************
* Function Name : bsp_EncoderRead
* Description	: 读取编码器值
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_EncoderRead(ENCODER_DevEnum dev, int32_t *value)
{
	struct Encoding_Drv *handle = GetDevHandle(dev);
	if (handle == NULL) {
		return false;
	}
	
	*value = handle->value;
	return true;
}

/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
