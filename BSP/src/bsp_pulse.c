/*
*********************************************************************************************************
* @file    	bsp_pulse.c
* @author  	SY
* @version 	V1.0.4
* @date     2017-11-20 12:48:47
* @IDE	 	Keil V4.72.10.0
* @Chip    	STM32F407VE
* @brief   	发脉冲驱动源文件
*********************************************************************************************************
* @attention
*	本驱动支持2路脉冲发出，拥有脉冲频率和脉冲个数2种发脉冲方式。脉冲计数使用主从定时器的方式，效果上与
*	使用定时器ETR引脚计数类似。
*	1、主从定时器方式：一个主定时器可以与多个从定时器搭配使用，而且不需要外部硬件连接，理论上计数不会受到干扰。
*	2、ETR引脚计数方式：约束条件较多，硬件上发脉冲定时器引脚必须连接到计数定时器的ETR引脚，增加受到干扰的
*		可能性，计数定时器被固定死了，不能任意配对使用。
*	3、必须使用TIM_ARRPreloadConfig()，可以改善低频特性，在发生更新事件时，才写入定时器的影子寄存器。否则，
*	如果直接写入ARR的值小于CNT，导致计数器不能进入中断而持续发脉冲！
* =======================================================================================================
*	版本 		时间					作者					说明
* -------------------------------------------------------------------------------------------------------
*	Ver1.0.1 	2016-4-20 11:03:29 		SY		1、	有一个BUG，计数定时器(从定时器)不能配置比较输出，
													否则导致以太网不能工作，死在 while (ETH_GetSoftwareResetStatus() == SET);
													比较输出如下配置会导致以太网初始化失败：
													TIM_OCInitTypeDef 	 		TIM_OCInitStructure;
													TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
													TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
													TIM_OCInitStructure.TIM_Pulse = 0xffff;
													TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
													TIM_OC2Init(COUNT1_USE_TIMx, &TIM_OCInitStructure);
*	Ver1.0.2	2017-11-20 12:49:07		SY		1、	增加发送脉冲计数功能。
*	Ver1.0.3 	2018-5-9 09:28:06		SY		1、	优化代码结构。
*	Ver1.0.4 	2018-7-31 13:15:59		SY		1、	增加函数 bsp_PulseUpdate()，解决低频切换到高频时，需要执行该函数，
													否则导致影子寄存器不能及时更新。需要在 bsp_OutputPulse()后面使用！
	Ver1.0.5  	2018-11-5 10:45:09 		SY		1、	修改ParsePulseCount()，该函数会导致使能位清除。
												2、	增加bsp_SetPulseType()，用于切换脉冲输出类型。
	Ver1.0.6  	2019-1-10 14:59:00 		SY		1、	可以实时修改脉冲频率										
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
enum COUNT_TYPE {
	COUNT_TYPE_ETR = 0,			//使用ETR计数
	COUNT_TYPE_SLAVE_TIM,		//使用从定时器计数
};

enum DEVICE_TYPE {
	DEVICE_MASTER = 0,
	DEVICE_SLAVE,
};

/*
*********************************************************************************************************
*                              				配置方案
*	1. 脉冲频率方式(伺服电机)：
*		#define USE_PULSE1							0x01U
*		#define	PULSE1_OUTPUT_TYPE					PULSE_TYPE_FREQ
*		#define PULSE1_FREQ_COUNT_EN				0x00U
*		#define USE_COUNT1							0x00U
*	2. 脉冲频率计数：
*		#define USE_PULSE1							0x01U
*		#define	PULSE1_OUTPUT_TYPE					PULSE_TYPE_FREQ
*		#define PULSE1_FREQ_COUNT_EN				0x01U
*		#define USE_COUNT1							0x01U
*	3. 脉冲个数方式(数字阀)：
*		#define USE_PULSE1							0x01U
*		#define	PULSE1_OUTPUT_TYPE					PULSE_TYPE_NUMS
*		#define PULSE1_FREQ_COUNT_EN				0x00U
*		#define USE_COUNT1							0x01U
*	
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				脉冲1
*********************************************************************************************************
*/	
#define USE_PULSE1							(false)
#if (USE_PULSE1)
	#define	PULSE1_OUTPUT_TYPE				PULSE_TYPE_NUMS
	#define PULSE1_FREQ_COUNT_EN			(false)			//脉冲频率计数
	//发脉冲定时器(TIM1_CH2)
	#define PULSE1_USE_TIMx					TIM1
	#define PULSE1_TIM_CHANNEL				TIM_CHANNEL_2
	#define	PULSE1_TIM_RCC_PRESCALER		(1)				//时钟分频
	#define	PULSE1_TIM_CLK_ENABLE()			do {\
												__HAL_RCC_TIM1_CLK_ENABLE();\
											} while(0)
	
	#define	PULSE1_GPIO_CLK_ENABLE()		do {\
												__HAL_RCC_GPIOC_CLK_ENABLE();\
												__HAL_RCC_GPIOE_CLK_ENABLE();\
											} while(0)
	#define	PULSE1_GPIO_PULSE_PORT  		GPIOE
	#define	PULSE1_GPIO_PULSE_PIN			GPIO_PIN_11
	#define PULSE1_GPIO_PULSE_AF			GPIO_AF1_TIM1											
	#define	PULSE1_GPIO_DIR_PORT  			GPIOC
	#define	PULSE1_GPIO_DIR_PIN				GPIO_PIN_2										
#endif

#define USE_COUNT1							(false)																					
#if (USE_COUNT1)
	#define	COUNT1_TYPE						COUNT_TYPE_ETR										
											
#if true
	//ETR定时器
	#define COUNT1_USE_TIMx					TIM4
	#define COUNT1_TIMx_IRQn				TIM4_IRQn
	#define COUNT1_TIM_CHANNEL				TIM_CHANNEL_1									
	#define COUNT1_TIM_IT_CC				TIM_IT_CC1										
	#define	COUNT1_TIM_CLK_ENABLE()			do {\
												__HAL_RCC_TIM4_CLK_ENABLE();\
											} while(0)
										
	#define	COUNT1_GPIO_CLK_ENABLE()		do {\
												__HAL_RCC_GPIOE_CLK_ENABLE();\
											} while(0)

	#define	COUNT1_GPIO_ETR_PORT  			GPIOE
	#define	COUNT1_GPIO_ETR_PIN				GPIO_PIN_0
	#define COUNT1_GPIO_PULSE_AF			GPIO_AF2_TIM4										
#else
	//从定时器（TIM4_CH1 <==> ITR0 (TS = 000)）
	#define COUNT1_USE_TIMx					TIM4
	#define COUNT1_TIMx_IRQn				TIM4_IRQn
	#define COUNT1_TIM_CCR					&(TIM4->CCR1)
	#define COUNT1_TIM_IT_CC				TIM_IT_CC1
	#define COUNT1_TIM_ITR					TIM_TS_ITR0		//通过参考手册确认对应关系（表78 TIMx内部触发连接）							
	#define	COUNT1_TIM_CLK_ENABLE()			do {\
												__HAL_RCC_TIM4_CLK_ENABLE();\
											} while(0)
#endif											
#endif


/*
*********************************************************************************************************
*                              				脉冲2
*********************************************************************************************************
*/											
#define USE_PULSE2							(true)
#if (USE_PULSE2)			
	#define	PULSE2_OUTPUT_TYPE				PULSE_TYPE_FREQ										
	#define PULSE2_FREQ_COUNT_EN			(false)			//脉冲频率计数										
	//发脉冲定时器（TIM5_CH1）
	#define PULSE2_USE_TIMx					TIM5
	#define PULSE2_TIM_CHANNEL				TIM_CHANNEL_1
	#define	PULSE2_TIM_RCC_PRESCALER		(2)				//时钟分频									
	#define	PULSE2_TIM_CLK_ENABLE()			do {\
												__HAL_RCC_TIM5_CLK_ENABLE();\
											} while(0)
	
	#define	PULSE2_GPIO_CLK_ENABLE()		do {\
												__HAL_RCC_GPIOA_CLK_ENABLE();\
											} while(0)
	#define	PULSE2_GPIO_PULSE_PORT  		GPIOA
	#define	PULSE2_GPIO_PULSE_PIN			GPIO_PIN_0
	#define PULSE2_GPIO_PULSE_AF			GPIO_AF2_TIM5										
	#define	PULSE2_GPIO_DIR_PORT  			GPIOA
	#define	PULSE2_GPIO_DIR_PIN				GPIO_PIN_3											
#endif
											
#define USE_COUNT2							(false)
#if (USE_COUNT2)
	#define	COUNT2_TYPE						COUNT_TYPE_ETR	
											
#if true
	//ETR定时器
	#define COUNT2_USE_TIMx					TIM2
	#define COUNT2_TIMx_IRQn				TIM2_IRQn
	#define COUNT2_TIM_CHANNEL				TIM_CHANNEL_1						
	#define COUNT2_TIM_IT_CC				TIM_IT_CC1
	#define	COUNT2_TIM_CLK_ENABLE()			do {\
												__HAL_RCC_TIM2_CLK_ENABLE();\
											} while(0)
										
	#define	COUNT2_GPIO_CLK_ENABLE()		do {\
												__HAL_RCC_GPIOA_CLK_ENABLE();\
											} while(0)

	#define	COUNT2_GPIO_ETR_PORT  			GPIOA
	#define	COUNT2_GPIO_ETR_PIN				GPIO_PIN_0
	#define COUNT2_GPIO_PULSE_AF			GPIO_AF1_TIM2										
#else
	//从定时器(TIM4_CH1 <==> ITR0 (TS = 000))
	#define COUNT2_USE_TIMx					TIM4
	#define COUNT2_TIMx_IRQn				TIM4_IRQn
	#define COUNT2_TIM_CCR					&(TIM4->CCR1)
	#define COUNT2_TIM_IT_CC				TIM_IT_CC1
	#define COUNT2_TIM_ITR					TIM_TS_ITR0											
	#define	COUNT2_TIM_CLK_ENABLE()			do {\
												__HAL_RCC_TIM4_CLK_ENABLE();\
											} while(0)
#endif
#endif											
											
/*
*********************************************************************************************************
*                              				脉冲3
*********************************************************************************************************
*/											
#define USE_PULSE3							(true)
#if (USE_PULSE3)			
	#define	PULSE3_OUTPUT_TYPE				PULSE_TYPE_FREQ										
	#define PULSE3_FREQ_COUNT_EN			(false)			//脉冲频率计数										
	//发脉冲定时器（TIM5_CH1）
	#define PULSE3_USE_TIMx					TIM2
	#define PULSE3_TIM_CHANNEL				TIM_CHANNEL_1
	#define	PULSE3_TIM_RCC_PRESCALER		(2)				//时钟分频									
	#define	PULSE3_TIM_CLK_ENABLE()			do {\
												__HAL_RCC_TIM2_CLK_ENABLE();\
											} while(0)
	
	#define	PULSE3_GPIO_CLK_ENABLE()		do {\
												__HAL_RCC_GPIOA_CLK_ENABLE();\
											} while(0)
	#define	PULSE3_GPIO_PULSE_PORT  		GPIOA
	#define	PULSE3_GPIO_PULSE_PIN			GPIO_PIN_5
	#define PULSE3_GPIO_PULSE_AF			GPIO_AF1_TIM2										
	#define	PULSE3_GPIO_DIR_PORT  			GPIOA
	#define	PULSE3_GPIO_DIR_PIN				GPIO_PIN_6											
#endif
											
#define USE_COUNT3							(false)
#if (USE_COUNT3)
	#define	COUNT3_TYPE						COUNT_TYPE_ETR	
											
#if true
	//ETR定时器
	#define COUNT3_USE_TIMx					TIM2
	#define COUNT3_TIMx_IRQn				TIM2_IRQn
	#define COUNT3_TIM_CHANNEL				TIM_CHANNEL_1						
	#define COUNT3_TIM_IT_CC				TIM_IT_CC1
	#define	COUNT3_TIM_CLK_ENABLE()			do {\
												__HAL_RCC_TIM2_CLK_ENABLE();\
											} while(0)
										
	#define	COUNT3_GPIO_CLK_ENABLE()		do {\
												__HAL_RCC_GPIOA_CLK_ENABLE();\
											} while(0)

	#define	COUNT3_GPIO_ETR_PORT  			GPIOA
	#define	COUNT3_GPIO_ETR_PIN				GPIO_PIN_0
	#define COUNT3_GPIO_PULSE_AF			GPIO_AF1_TIM2										
#else
	//从定时器(TIM4_CH1 <==> ITR0 (TS = 000))
	#define COUNT3_USE_TIMx					TIM4
	#define COUNT3_TIMx_IRQn				TIM4_IRQn
	#define COUNT3_TIM_CCR					&(TIM4->CCR1)
	#define COUNT3_TIM_IT_CC				TIM_IT_CC1
	#define COUNT3_TIM_ITR					TIM_TS_ITR0											
	#define	COUNT3_TIM_CLK_ENABLE()			do {\
												__HAL_RCC_TIM4_CLK_ENABLE();\
											} while(0)
#endif
#endif	

#define STEP_MAX_PULSE_COUNTS				(0xFFFF)
#define DEFAULT_PULSE_FREQ					(16000)

/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/
struct PulseCount
{
	bool enable;			
	enum COUNT_TYPE type;			
	bool busy;								//脉冲输出忙标识
	__IO int32_t sumPulseNums;				//总脉冲个数
	__IO int32_t sendPulseNums;				//已发送的脉冲个数
	__IO int32_t onceMaxPulseNums;			//单次发送最大脉冲个数
};

struct PulseFreqCount
{
	bool enable;
	int32_t pulseNums;
	/* 从定时器强制初始化为16位，这里必须和定时器溢出值位数保持一致 */
	uint16_t prevTimerCnt;
};

enum PULSE_DIR {
	PULSE_FWD = 0,
	PULSE_REV,
};

struct __Reg {
	enum DEVICE_TYPE deviceType;
	TIM_HandleTypeDef handle;	
	TIM_TypeDef *timer;
	uint32_t channel;
	void *private_data;
};

struct MasterTimerPrivate {
	uint32_t rccPrescaler;
	uint32_t clockFreq;
	GPIO_TypeDef *GPIO_PortDir;
	uint16_t GPIO_PinDir;
};

struct SlaveTimerPrivate {
	IRQn_Type irqn;
	__IO uint32_t IT_CC;					//中断比较寄存器
	uint32_t InputTrigger;
};

struct PulseDrv {
	enum PULSE_Dev dev;						//设备
	enum PULSE_OUTPUT_TYPE type;			//脉冲输出类型
	uint32_t defaultFreq;					//缺省脉冲频率
	struct __Reg mReg;						//主寄存器
	struct __Reg sReg;						//从寄存器
	
	struct PulseCount pulseCount;			//脉冲计数
	struct PulseFreqCount pulseFreqCount;	//脉冲频率计数
	
	bool (*init)(struct PulseDrv *this);
	bool (*write)(struct PulseDrv *this, void *value);
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
static bool Pulse_Init(struct PulseDrv *this);

static bool WritePulse(struct PulseDrv *this, void *pulse);
uint32_t BSP_CPU_ClkFreq(void);
static void PulseFreqCountCycle(struct PulseDrv *this);
/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
#if (USE_PULSE1)
struct MasterTimerPrivate Pulse1MasterTimerPrivateData = {
	.rccPrescaler			= PULSE1_TIM_RCC_PRESCALER,
	.GPIO_PortDir 			= PULSE1_GPIO_DIR_PORT,
	.GPIO_PinDir 			= PULSE1_GPIO_DIR_PIN,
};
#endif

#if (USE_COUNT1)
struct SlaveTimerPrivate Count1SlaveTimerPrivateData = {
	.irqn					= COUNT1_TIMx_IRQn,
	.IT_CC					= COUNT1_TIM_IT_CC,
#if (COUNT1_TIM_ITR)
	.InputTrigger			= COUNT1_TIM_ITR,
#endif
};
#endif

#if (USE_PULSE2)
struct MasterTimerPrivate Pulse2MasterTimerPrivateData = {
	.rccPrescaler			= PULSE2_TIM_RCC_PRESCALER,
	.GPIO_PortDir 			= PULSE2_GPIO_DIR_PORT,
	.GPIO_PinDir 			= PULSE2_GPIO_DIR_PIN,
};
#endif

#if (USE_COUNT2)
struct SlaveTimerPrivate Count2SlaveTimerPrivateData = {
	.irqn					= COUNT2_TIMx_IRQn,
	.IT_CC					= COUNT2_TIM_IT_CC,
#if (COUNT2_TIM_ITR)
	.InputTrigger			= COUNT2_TIM_ITR,
#endif
};
#endif

#if (USE_PULSE3)
struct MasterTimerPrivate Pulse3MasterTimerPrivateData = {
	.rccPrescaler			= PULSE3_TIM_RCC_PRESCALER,
	.GPIO_PortDir 			= PULSE3_GPIO_DIR_PORT,
	.GPIO_PinDir 			= PULSE3_GPIO_DIR_PIN,
};
#endif

#if (USE_COUNT3)
struct SlaveTimerPrivate Count3SlaveTimerPrivateData = {
	.irqn					= COUNT3_TIMx_IRQn,
	.IT_CC					= COUNT3_TIM_IT_CC,
#if (COUNT3_TIM_ITR)
	.InputTrigger			= COUNT3_TIM_ITR,
#endif
};
#endif


static struct PulseDrv g_Pulse[] = {
#if (USE_PULSE1)
{
	.dev					= DEV_PULSE1,
	.type 					= PULSE1_OUTPUT_TYPE,
	.defaultFreq			= DEFAULT_PULSE_FREQ,
	.mReg 					= {
		.deviceType			= DEVICE_MASTER,
		.handle				= {
			.Instance		= PULSE1_USE_TIMx,
		},
		.timer				= PULSE1_USE_TIMx,
		.channel 			= PULSE1_TIM_CHANNEL,
		.private_data		= &Pulse1MasterTimerPrivateData,
	},
#if (USE_COUNT1)
	.sReg					= {
		.deviceType			= DEVICE_SLAVE,
		.handle				= {
			.Instance		= COUNT1_USE_TIMx,
		},
		.timer				= COUNT1_USE_TIMx,
		.channel 			= COUNT1_TIM_CHANNEL,
		.private_data		= &Count1SlaveTimerPrivateData,
	},
	.pulseCount				= {
		.enable				= true,
		.type				= COUNT1_TYPE,
	},
#endif
#if (PULSE1_FREQ_COUNT_EN)
	.pulseFreqCount			= {
		.enable				= true,
	},
#endif	
	.init					= Pulse_Init,
	.write					= WritePulse,
},
#endif

#if (USE_PULSE2)
{
	.dev					= DEV_PULSE2,
	.type 					= PULSE2_OUTPUT_TYPE,
	.defaultFreq			= DEFAULT_PULSE_FREQ,
	.mReg 					= {
		.deviceType			= DEVICE_MASTER,
		.handle				= {
			.Instance		= PULSE2_USE_TIMx,
		},
		.timer				= PULSE2_USE_TIMx,
		.channel 			= PULSE2_TIM_CHANNEL,
		.private_data		= &Pulse2MasterTimerPrivateData,
	},
#if (USE_COUNT2)
	.sReg					= {
		.deviceType			= DEVICE_SLAVE,
		.handle				= {
			.Instance		= COUNT2_USE_TIMx,
		},
		.timer				= COUNT2_USE_TIMx,
		.channel 			= COUNT2_TIM_CHANNEL,
		.private_data		= &Count2SlaveTimerPrivateData,
	},
	.pulseCount				= {
		.enable				= true,
		.type				= COUNT2_TYPE,
	},
#endif
#if (PULSE2_FREQ_COUNT_EN)
	.pulseFreqCount			= {
		.enable				= true,
	},
#endif
	.init					= Pulse_Init,
	.write					= WritePulse,
},
#endif

#if (USE_PULSE3)
{
	.dev					= DEV_PULSE3,
	.type 					= PULSE3_OUTPUT_TYPE,
	.defaultFreq			= DEFAULT_PULSE_FREQ,
	.mReg 					= {
		.deviceType			= DEVICE_MASTER,
		.handle				= {
			.Instance		= PULSE3_USE_TIMx,
		},
		.timer				= PULSE3_USE_TIMx,
		.channel 			= PULSE3_TIM_CHANNEL,
		.private_data		= &Pulse3MasterTimerPrivateData,
	},
#if (USE_COUNT3)
	.sReg					= {
		.deviceType			= DEVICE_SLAVE,
		.handle				= {
			.Instance		= COUNT3_USE_TIMx,
		},
		.timer				= COUNT3_USE_TIMx,
		.channel 			= COUNT3_TIM_CHANNEL,
		.private_data			= &Count3SlaveTimerPrivateData,
	},
	.pulseCount				= {
		.enable				= true,
		.type				= COUNT3_TYPE,
	},
#endif
#if (PULSE3_FREQ_COUNT_EN)
	.pulseFreqCount			= {
		.enable				= true,
	},
#endif
	.init					= Pulse_Init,
	.write					= WritePulse,
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
*                              				UTILS
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : GetHandleByDevice
* Description	: 通过句柄获取设备地址
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static struct __Reg *GetRegHandleByDevice(TIM_HandleTypeDef *handle)
{
	return container_of(handle, struct __Reg, handle);
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
static struct PulseDrv *GetPulseHandleByDevice(TIM_HandleTypeDef *handle, enum DEVICE_TYPE deviceType)
{
	struct __Reg *regHandle = GetRegHandleByDevice(handle);
	
	struct PulseDrv *deviceHandle = NULL;
	switch (deviceType) {
		case DEVICE_MASTER:
			deviceHandle = container_of(regHandle, struct PulseDrv, mReg);
			break;
		case DEVICE_SLAVE:
			deviceHandle = container_of(regHandle, struct PulseDrv, sReg);
			break;
	}
	return deviceHandle;
}

/*
*********************************************************************************************************
* Function Name : GetPulseDevHandle
* Description	: 获取脉冲设备句柄
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static struct PulseDrv *GetPulseDevHandle(enum PULSE_Dev dev)
{
	struct PulseDrv *handle = NULL;
	for (uint8_t i=0; i<ARRAY_SIZE(g_Pulse); ++i) {
		if (dev == g_Pulse[i].dev) {
			handle = &g_Pulse[i];
			break;
		}
	}
	return handle;
}

/*
*********************************************************************************************************
* Function Name : SetDir
* Description	: 设置方向
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void SetDir(struct PulseDrv *this, enum PULSE_DIR dir)
{
	struct MasterTimerPrivate *private_data = this->mReg.private_data;
	if (dir == PULSE_FWD) {
		HAL_GPIO_WritePin(private_data->GPIO_PortDir, private_data->GPIO_PinDir, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(private_data->GPIO_PortDir, private_data->GPIO_PinDir, GPIO_PIN_RESET);
	}
}

/*
*********************************************************************************************************
* Function Name : SetPWM_Mode
* Description	: 设置PWM模式
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void SetPWM_Mode(struct PulseDrv *this, bool enable)
{	
	switch (this->mReg.channel)
	{
		case TIM_CHANNEL_1:
		{
			CLEAR_BIT(this->mReg.handle.Instance->CCMR1, TIM_CCMR1_OC1M);
			uint32_t value = (enable) ? TIM_CCMR1_OC1M : TIM_OCMODE_TIMING;
			SET_BIT(this->mReg.handle.Instance->CCMR1, value);
			break;
		}
		case TIM_CHANNEL_2:
		{
			CLEAR_BIT(this->mReg.handle.Instance->CCMR1, TIM_CCMR1_OC2M);
			uint32_t value = (enable) ? TIM_CCMR1_OC2M : TIM_OCMODE_TIMING;
			SET_BIT(this->mReg.handle.Instance->CCMR1, value);
			break;
		}
		case TIM_CHANNEL_3:
		{
			CLEAR_BIT(this->mReg.handle.Instance->CCMR2, TIM_CCMR2_OC3M);
			uint32_t value = (enable) ? TIM_CCMR2_OC3M : TIM_OCMODE_TIMING;
			SET_BIT(this->mReg.handle.Instance->CCMR2, value);
			break;
		}
		case TIM_CHANNEL_4:
		{
			CLEAR_BIT(this->mReg.handle.Instance->CCMR2, TIM_CCMR2_OC4M);
			uint32_t value = (enable) ? TIM_CCMR2_OC4M : TIM_OCMODE_TIMING;
			SET_BIT(this->mReg.handle.Instance->CCMR2, value);
			break;
		}
	}
}

/*
*********************************************************************************************************
* Function Name : SetPulseFreq
* Description	: 设置脉冲频率
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool SetPulseFreq(struct PulseDrv *this, float pulseFreq)
{
	enum PULSE_DIR dir = PULSE_FWD;
	if (pulseFreq < 0) {
		dir = PULSE_REV;				
	}
	pulseFreq = fabs(pulseFreq);		
	__HAL_TIM_DISABLE(&this->mReg.handle);
	SetPWM_Mode(this, false);
	SetDir(this, dir);

	const float MIN_FREQ = RCC_MAX_FREQUENCY / 65535.0f / 65535.0f + 0.001f;
	if (pulseFreq < MIN_FREQ)
	{			
		__HAL_TIM_SET_COUNTER(&this->mReg.handle, 0);
		__HAL_TIM_SET_AUTORELOAD(&this->mReg.handle, 0);
	}
	else
	{	
		struct MasterTimerPrivate *private_data = this->mReg.private_data;		
		uint32_t arr = private_data->clockFreq / pulseFreq + 0.500001f;	
	 	uint16_t psc = 0x0001;
	 	while (arr & 0xFFFF0000)
	 	{
	 		arr >>= 1;
	 		psc <<= 1;
	 	}
		
		uint32_t _cnt = __HAL_TIM_GET_COUNTER(&this->mReg.handle);		
		uint32_t prev_arr = __HAL_TIM_GET_AUTORELOAD(&this->mReg.handle);				
		__HAL_TIM_SET_AUTORELOAD(&this->mReg.handle, arr - 1);
		uint32_t now_arr = __HAL_TIM_GET_AUTORELOAD(&this->mReg.handle);		
		__HAL_TIM_SET_PRESCALER(&this->mReg.handle, psc - 1);
		__HAL_TIM_SET_COMPARE(&this->mReg.handle, this->mReg.channel, arr >> 1);			
		HAL_TIM_GenerateEvent(&this->mReg.handle, TIM_EVENTSOURCE_UPDATE);		
		if (prev_arr) {
			uint32_t nowCnt = (float)_cnt / prev_arr * now_arr;
			if (nowCnt > now_arr) {
				ECHO(ECHO_DEBUG, "[BSP] 计数器溢出！");
				nowCnt = 0;
			}
			__HAL_TIM_SET_COUNTER(&this->mReg.handle, nowCnt);
		}		
		SetPWM_Mode(this, true);
		HAL_TIM_PWM_Start(&this->mReg.handle, this->mReg.channel);
	}
	return true;
}

/*
*********************************************************************************************************
* Function Name : ParsePulseCount
* Description	: 解析脉冲个数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void ParsePulseCount(struct PulseDrv *this, int32_t count)
{	
	this->pulseCount.sendPulseNums = 0;	
	this->pulseCount.sumPulseNums = count;
	if (count >= 0) {
		this->pulseCount.onceMaxPulseNums = STEP_MAX_PULSE_COUNTS;
	} else {
		this->pulseCount.onceMaxPulseNums = -STEP_MAX_PULSE_COUNTS;
	}
}

/*
*********************************************************************************************************
* Function Name : DoPulseOutput
* Description	: 脉冲输出
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static int32_t DoPulseOutput(struct PulseDrv *this, int32_t pulseNum)
{	
	if (!pulseNum) {
		return 0;
	}
	
	this->pulseCount.busy = true;
	enum PULSE_DIR dir = PULSE_FWD;
	if (pulseNum < 0) {
		dir = PULSE_REV;				
	}
	pulseNum = abs(pulseNum);
	
	HAL_TIM_PWM_Stop(&this->mReg.handle, this->mReg.channel);
	SetDir(this, dir);
	__HAL_TIM_SET_COUNTER(&this->sReg.handle, 0);
	__HAL_TIM_SET_COMPARE(&this->sReg.handle, this->sReg.channel, pulseNum);
	HAL_TIM_PWM_Start(&this->mReg.handle, this->mReg.channel);
	
	return pulseNum;
} 

/*
*********************************************************************************************************
* Function Name : SetPulseOutput
* Description	: 设置脉冲输出
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static int32_t SetPulseOutput(struct PulseDrv *this)
{
	int32_t remindCount = this->pulseCount.sumPulseNums - this->pulseCount.sendPulseNums;
	if (abs(remindCount) > abs(this->pulseCount.onceMaxPulseNums)) {
		remindCount = this->pulseCount.onceMaxPulseNums;
	}
	this->pulseCount.sendPulseNums += remindCount;
	return DoPulseOutput(this, remindCount);
}

/*
*********************************************************************************************************
* Function Name : SetPulseNums
* Description	: 设置脉冲个数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool SetPulseNums(struct PulseDrv *this, int32_t count)
{
	if (this->pulseCount.busy == true) {
		return false;
	}
	ParsePulseCount(this, count);
	SetPulseOutput(this);
	return true;
}

/*
*********************************************************************************************************
* Function Name : WritePulse
* Description	: 写入脉冲
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool WritePulse(struct PulseDrv *this, void *pulse)
{
	bool ret;
	
	if (this->type == PULSE_TYPE_FREQ) {
		float pulseFreq = *(float *)pulse;
		ret = SetPulseFreq(this, pulseFreq);
	} else {
		int32_t pulseNums = *(int32_t *)pulse;
		ret = SetPulseNums(this, pulseNums);
	}
	
	return ret;
}

/*
*********************************************************************************************************
* Function Name : PulseFreqCountCycle
* Description	: 脉冲频率计数循环体(周期性执行,对于定时器脉冲计数，关键在于：计数器只会增加，不会减少。如果反向
*					发脉冲，则方向引脚改变，不会引起计数器的方向改变。)
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void PulseFreqCountCycle(struct PulseDrv *this)
{
	if (!this->pulseFreqCount.enable) {
		return;
	}
	
	uint16_t nowTimerCnt = __HAL_TIM_GET_COUNTER(&this->sReg.handle);
	bool posiDir = false;
	struct MasterTimerPrivate *private_data = this->mReg.private_data;
	if (HAL_GPIO_ReadPin(private_data->GPIO_PortDir, private_data->GPIO_PinDir))
	{
		posiDir = true;
	}
	uint16_t diff = nowTimerCnt - this->pulseFreqCount.prevTimerCnt;
	if (posiDir)
	{
		this->pulseFreqCount.pulseNums += diff;
	}
	else
	{
		this->pulseFreqCount.pulseNums -= diff;
	}
	this->pulseFreqCount.prevTimerCnt = nowTimerCnt;
}

/*
*********************************************************************************************************
* Function Name : PulseMasterTimer_Init
* Description	: 主定时器脉冲初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool PulseMasterTimer_Init(struct PulseDrv *this)
{
	struct MasterTimerPrivate *private_data = this->mReg.private_data;
	if (!private_data->rccPrescaler) {
		return false;
	}
	private_data->clockFreq = HAL_RCC_GetHCLKFreq() / private_data->rccPrescaler;
	
	if (!this->defaultFreq) {
		return false;
	}
	const uint32_t Prescaler = 21;
	uint32_t Period = private_data->clockFreq / Prescaler / this->defaultFreq;	
	
	/********** 配置基本定时器 **********/
	this->mReg.handle.Init.Period = Period - 1;
	this->mReg.handle.Init.Prescaler = Prescaler - 1;
	this->mReg.handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	this->mReg.handle.Init.CounterMode = TIM_COUNTERMODE_UP;
	this->mReg.handle.Init.RepetitionCounter = 0;
	if (HAL_TIM_PWM_Init(&this->mReg.handle) != HAL_OK)
	{
		BSP_ErrorHandler();
	}
	
	/********** 配置基本PWM输出 **********/
	TIM_OC_InitTypeDef TIM_OCInitStructure = {0};
	uint16_t CCR_Val = Period >> 1;
	
	TIM_OCInitStructure.OCMode = TIM_OCMODE_PWM2; 						
	TIM_OCInitStructure.Pulse = CCR_Val; 									
	TIM_OCInitStructure.OCPolarity = TIM_OCPOLARITY_LOW; 	
	TIM_OCInitStructure.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	TIM_OCInitStructure.OCFastMode = TIM_OCFAST_DISABLE;
	TIM_OCInitStructure.OCIdleState = TIM_OCIDLESTATE_SET;
	TIM_OCInitStructure.OCNIdleState= TIM_OCNIDLESTATE_RESET;
	HAL_TIM_PWM_ConfigChannel(&this->mReg.handle, &TIM_OCInitStructure, this->mReg.channel);
	
	/* Set the ARR Preload Bit */
    this->mReg.handle.Instance->CR1 |= TIM_CR1_ARPE;					//重要！！！只有发生更新事件，ARR寄存器的值才会更新到影子寄存器
	
	__HAL_TIM_CLEAR_FLAG(&this->mReg.handle, TIM_FLAG_UPDATE);
	HAL_TIM_PWM_Stop(&this->mReg.handle, this->mReg.channel);
	__HAL_TIM_SET_COUNTER(&this->mReg.handle, 0);
	__HAL_TIM_SET_AUTORELOAD(&this->mReg.handle, 0);
	return true;
}

/*
*********************************************************************************************************
* Function Name : PulseSlaveTimer_Init
* Description	: 从定时器脉冲初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool PulseSlaveTimer_Init(struct PulseDrv *this)
{
	/********** 配置基本定时器 **********/
	this->sReg.handle.Init.Period = 0xffff;
	this->sReg.handle.Init.Prescaler = 0;
	this->sReg.handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	this->sReg.handle.Init.CounterMode = TIM_COUNTERMODE_UP;
	this->sReg.handle.Init.RepetitionCounter = 0;
	if (HAL_TIM_PWM_Init(&this->sReg.handle) != HAL_OK)
	{
		BSP_ErrorHandler();
	}
	
	struct SlaveTimerPrivate *private_data = this->sReg.private_data;
	switch (this->pulseCount.type) 
	{
		case COUNT_TYPE_ETR:
		{
			TIM_ClockConfigTypeDef sClockSourceConfig = {0};
			sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_ETRMODE2;
			sClockSourceConfig.ClockPolarity = TIM_CLOCKPOLARITY_INVERTED;
			sClockSourceConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
			sClockSourceConfig.ClockFilter = 0x00;
			HAL_TIM_ConfigClockSource(&this->sReg.handle, &sClockSourceConfig);	
			break;
		}			
		case COUNT_TYPE_SLAVE_TIM:
		{
			/* 主定时器脉冲输出作为从定时器的时钟输入 */
			TIM_MasterConfigTypeDef sMasterConfig = {0};
			sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_ENABLE;
			sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;			
			HAL_TIMEx_MasterConfigSynchronization(&this->sReg.handle, &sMasterConfig);
			
			TIM_SlaveConfigTypeDef sSlaveConfig = {0};
			sSlaveConfig.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;
			sSlaveConfig.InputTrigger = private_data->InputTrigger;
			HAL_TIM_SlaveConfigSynchronization(&this->sReg.handle, &sSlaveConfig);
			break;
		}
		default:
			return false;
	}
	
	__HAL_TIM_SET_COUNTER(&this->sReg.handle, 0);
	__HAL_TIM_CLEAR_FLAG(&this->sReg.handle, TIM_FLAG_UPDATE);
	HAL_NVIC_SetPriority(private_data->irqn, 0, 0);
	if (this->pulseFreqCount.enable) {
		HAL_NVIC_DisableIRQ(private_data->irqn);
		__HAL_TIM_DISABLE_IT(&this->sReg.handle, private_data->IT_CC);
	} else {
		HAL_NVIC_EnableIRQ(private_data->irqn);
		__HAL_TIM_ENABLE_IT(&this->sReg.handle, private_data->IT_CC);
	}
	__HAL_TIM_ENABLE(&this->sReg.handle);
	
	return true;
}

/*
*********************************************************************************************************
* Function Name : Pulse_Init
* Description	: 脉冲初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool Pulse_Init(struct PulseDrv *this)
{
	bool ret = PulseMasterTimer_Init(this);
	if (ret == false) {
		return false;
	}
	if (this->pulseCount.enable) {
		ret = PulseSlaveTimer_Init(this);
	}
	return ret;
}

/*
*********************************************************************************************************
*                              				中断服务函数
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : COUNTx_TIM_IRQHandler
* Description	: 计数器中断服务函数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void COUNTx_TIM_IRQHandler(struct PulseDrv *this)
{
	HAL_TIM_PWM_Stop(&this->mReg.handle, this->mReg.channel);
	if (!SetPulseOutput(this)) {
		this->pulseCount.busy = false;
	}
}

/*
*********************************************************************************************************
* Function Name : HAL_TIM_PWM_PulseFinishedCallback
* Description	: PWM脉冲发送完成
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
	struct __Reg *regHandle = GetRegHandleByDevice(htim);
	struct PulseDrv *this = GetPulseHandleByDevice(htim, regHandle->deviceType);
	COUNTx_TIM_IRQHandler(this);
}

#if (USE_COUNT1)
/*
*********************************************************************************************************
* Function Name : TIM4_IRQHandler
* Description	: 计数器中断服务函数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void TIM4_IRQHandler(void)
{
	struct PulseDrv *this = GetPulseDevHandle(DEV_PULSE1);
	HAL_TIM_IRQHandler(&this->sReg.handle);
}
#endif

#if (USE_COUNT2)
/*
*********************************************************************************************************
* Function Name : TIM2_IRQHandler
* Description	: 计数器中断服务函数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void TIM5_IRQHandler(void)
{
	struct PulseDrv *this = GetPulseDevHandle(DEV_PULSE2);
	HAL_TIM_IRQHandler(&this->sReg.handle);
}
#endif

#if (USE_COUNT3)
/*
*********************************************************************************************************
* Function Name : TIM2_IRQHandler
* Description	: 计数器中断服务函数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void TIM2_IRQHandler(void)
{
	struct PulseDrv *this = GetPulseDevHandle(DEV_PULSE3);
	HAL_TIM_IRQHandler(&this->sReg.handle);
}
#endif

/*
*********************************************************************************************************
*                              				MSP
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : HAL_TIM_PWM_MspInit
* Description	: 定时器MSP初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
	struct __Reg *regHandle = GetRegHandleByDevice(htim);
	struct PulseDrv *this = GetPulseHandleByDevice(htim, regHandle->deviceType);
	
	switch (this->dev) {
		case DEV_PULSE1:
		{
			if (regHandle->deviceType == DEVICE_MASTER) {
			#if (USE_PULSE1)	
				/* GPIO Clock */
				PULSE1_GPIO_CLK_ENABLE();

				/* Config GPIO */
				GPIO_InitTypeDef GPIO_InitStructure = {0}; 
				GPIO_InitStructure.Pin = PULSE1_GPIO_PULSE_PIN;
				GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
				GPIO_InitStructure.Pull = GPIO_NOPULL;
				GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
				GPIO_InitStructure.Alternate = PULSE1_GPIO_PULSE_AF;
				HAL_GPIO_Init(PULSE1_GPIO_PULSE_PORT, &GPIO_InitStructure);
				
				GPIO_InitStructure.Pin 	= PULSE1_GPIO_DIR_PIN;
				GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP; 	
				HAL_GPIO_Init(PULSE1_GPIO_DIR_PORT, &GPIO_InitStructure);	
				
				/* TIM Clock */
				PULSE1_TIM_CLK_ENABLE();
			#endif	
			} else if (regHandle->deviceType == DEVICE_SLAVE) {
			#if (USE_COUNT1)		
				if (this->pulseCount.type == COUNT_TYPE_ETR) {
					/* GPIO Clock */
					COUNT1_GPIO_CLK_ENABLE();
					GPIO_InitTypeDef GPIO_InitStructure = {0}; 
					GPIO_InitStructure.Pin = COUNT1_GPIO_ETR_PIN;
					GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
					GPIO_InitStructure.Pull = GPIO_NOPULL;
					GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
					GPIO_InitStructure.Alternate = COUNT1_GPIO_PULSE_AF;
					HAL_GPIO_Init(COUNT1_GPIO_ETR_PORT, &GPIO_InitStructure);
				}

				/* TIM Clock */
				COUNT1_TIM_CLK_ENABLE();
			#endif
			}
			break;
		}
		case DEV_PULSE2:
		{
			if (regHandle->deviceType == DEVICE_MASTER) {
			#if (USE_PULSE2)
				/* GPIO Clock */
				PULSE2_GPIO_CLK_ENABLE();

				/* Config GPIO */
				GPIO_InitTypeDef GPIO_InitStructure = {0}; 
				GPIO_InitStructure.Pin = PULSE2_GPIO_PULSE_PIN;
				GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
				GPIO_InitStructure.Pull = GPIO_NOPULL;
				GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
				GPIO_InitStructure.Alternate = PULSE2_GPIO_PULSE_AF;
				HAL_GPIO_Init(PULSE2_GPIO_PULSE_PORT, &GPIO_InitStructure);
				
				GPIO_InitStructure.Pin 	= PULSE2_GPIO_DIR_PIN;
				GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP; 	
				HAL_GPIO_Init(PULSE2_GPIO_DIR_PORT, &GPIO_InitStructure);	
				
				/* TIM Clock */
				PULSE2_TIM_CLK_ENABLE();
			#endif	
			} else if (regHandle->deviceType == DEVICE_SLAVE) {
			#if (USE_COUNT2)	
				/* ETR GPIO */
				if (this->pulseCount.type == COUNT_TYPE_ETR) {
					/* GPIO Clock */
					COUNT2_GPIO_CLK_ENABLE();
					
					GPIO_InitTypeDef GPIO_InitStructure = {0}; 
					GPIO_InitStructure.Pin = COUNT2_GPIO_ETR_PIN;
					GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
					GPIO_InitStructure.Pull = GPIO_NOPULL;
					GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
					GPIO_InitStructure.Alternate = COUNT2_GPIO_PULSE_AF;
					HAL_GPIO_Init(COUNT2_GPIO_ETR_PORT, &GPIO_InitStructure);
				}

				/* TIM Clock */
				COUNT2_TIM_CLK_ENABLE();
			#endif	
			}
			break;
		}
		case DEV_PULSE3:
		{
			if (regHandle->deviceType == DEVICE_MASTER) {
			#if (USE_PULSE3)
				/* GPIO Clock */
				PULSE3_GPIO_CLK_ENABLE();

				/* Config GPIO */
				GPIO_InitTypeDef GPIO_InitStructure = {0}; 
				GPIO_InitStructure.Pin = PULSE3_GPIO_PULSE_PIN;
				GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
				GPIO_InitStructure.Pull = GPIO_NOPULL;
				GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
				GPIO_InitStructure.Alternate = PULSE3_GPIO_PULSE_AF;
				HAL_GPIO_Init(PULSE3_GPIO_PULSE_PORT, &GPIO_InitStructure);
				
				GPIO_InitStructure.Pin 	= PULSE3_GPIO_DIR_PIN;
				GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP; 	
				HAL_GPIO_Init(PULSE3_GPIO_DIR_PORT, &GPIO_InitStructure);	
				
				/* TIM Clock */
				PULSE3_TIM_CLK_ENABLE();
			#endif	
			} else if (regHandle->deviceType == DEVICE_SLAVE) {
			#if (USE_COUNT3)	
				/* ETR GPIO */
				if (this->pulseCount.type == COUNT_TYPE_ETR) {
					/* GPIO Clock */
					COUNT3_GPIO_CLK_ENABLE();
					
					GPIO_InitTypeDef GPIO_InitStructure = {0}; 
					GPIO_InitStructure.Pin = COUNT3_GPIO_ETR_PIN;
					GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
					GPIO_InitStructure.Pull = GPIO_NOPULL;
					GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
					GPIO_InitStructure.Alternate = COUNT3_GPIO_PULSE_AF;
					HAL_GPIO_Init(COUNT3_GPIO_ETR_PORT, &GPIO_InitStructure);
				}

				/* TIM Clock */
				COUNT3_TIM_CLK_ENABLE();
			#endif	
			}
			break;
		}
		default:
			break;
	}
}

/*
*********************************************************************************************************
*                              				API
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_InitPulse
* Description	: 初始化脉冲输出
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitPulse(void)
{
	for (uint8_t i=0; i<ARRAY_SIZE(g_Pulse); ++i) {
		if (g_Pulse[i].init) {
			bool ret = g_Pulse[i].init(&g_Pulse[i]);
			ECHO(ECHO_DEBUG, "[BSP] PULSE %d 初始化 .......... %s", g_Pulse[i].dev+1, (ret == true) ? "OK" : "ERROR");
		}
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_OutputPulse
* Description	: 脉冲输出
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_OutputPulse(enum PULSE_Dev dev, void *value)
{
	struct PulseDrv *this = GetPulseDevHandle(dev);
	if (!this) {
		return false;
	}
	
	return this->write(this, value);
}

/*
*********************************************************************************************************
* Function Name : bsp_PulseIsBusy
* Description	: 脉冲输出忙
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_PulseIsBusy(enum PULSE_Dev dev)
{
	struct PulseDrv *this = GetPulseDevHandle(dev);
	if (!this) {
		return false;
	}
	
	return this->pulseCount.busy;
}

/*
*********************************************************************************************************
* Function Name : bsp_PulseCountCycle
* Description	: 脉冲计数循环
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_PulseCountCycle(enum PULSE_Dev dev)
{
	struct PulseDrv *this = GetPulseDevHandle(dev);
	if (!this) {
		return;
	}
	PulseFreqCountCycle(this);
}

/*
*********************************************************************************************************
* Function Name : bsp_GetPulseCount
* Description	: 获取脉冲个数
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
int32_t bsp_GetPulseCount(enum PULSE_Dev dev)
{
	struct PulseDrv *this = GetPulseDevHandle(dev);
	if (!this) {
		return 0;
	}
	return this->pulseFreqCount.pulseNums;
}

/*
*********************************************************************************************************
* Function Name : bsp_PulseUpdate
* Description	: 脉冲更新（低频切换到高频时，需要执行该函数，否则导致影子寄存器不能及时更新）
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_PulseUpdate(enum PULSE_Dev dev)
{
	struct PulseDrv *this = GetPulseDevHandle(dev);
	if (!this) {
		return;
	}
	/* 当定时器在发送低频脉冲时，如果切换到发送高频脉冲，执行本函数可以快速响应 */	
	HAL_TIM_GenerateEvent(&this->mReg.handle, TIM_EVENTSOURCE_UPDATE);
}

/*
*********************************************************************************************************
* Function Name : bsp_SetPulseType
* Description	: 设置脉冲类型
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_SetPulseType(enum PULSE_Dev dev, enum PULSE_OUTPUT_TYPE type)
{
	struct PulseDrv *this = GetPulseDevHandle(dev);
	if (!this) {
		return;
	}
	
	this->type = type;
	this->pulseFreqCount.enable = (type == PULSE_TYPE_FREQ);
	Pulse_Init(this);
}

/*
*********************************************************************************************************
* Function Name : bsp_GetPulseType
* Description	: 获取脉冲类型
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
enum PULSE_OUTPUT_TYPE bsp_GetPulseType(enum PULSE_Dev dev)
{
	enum PULSE_OUTPUT_TYPE type = PULSE_TYPE_FREQ;
	struct PulseDrv *this = GetPulseDevHandle(dev);
	if (!this) {
		return type;
	}	
	return this->type;
}

/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
