/*
*********************************************************************************************************
* @file    	bsp_io.c
* @author  	SY
* @version 	V1.0.1
* @date    	2016-4-18 11:24:33
* @IDE	 	Keil V4.72.10.0
* @Chip    	STM32F407VE
* @brief   	IO驱动源文件
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


/*
*********************************************************************************************************
*                              				IO 输入
*********************************************************************************************************
*/
#define INPUT_REV							(BSP_DI_1 | BSP_DI_2 | BSP_DI_3 | \
											BSP_DI_4 | BSP_DI_5 | BSP_DI_6 | \
											BSP_DI_7 | BSP_DI_10 | BSP_DI_11)
#define USE_GPIO_IO_INPUT					(true)				
#if (USE_GPIO_IO_INPUT)
#define IO_INPUT_GPIO_CLK_ENABLE()			do {\
													__HAL_RCC_GPIOA_CLK_ENABLE();\
													__HAL_RCC_GPIOB_CLK_ENABLE();\
													__HAL_RCC_GPIOC_CLK_ENABLE();\
													__HAL_RCC_GPIOE_CLK_ENABLE();\
													__HAL_RCC_GPIOG_CLK_ENABLE();\
													__HAL_RCC_GPIOF_CLK_ENABLE();\
											} while (0)
#define	IO1_INPUT_GPIO_PORT					GPIOC
#define	IO1_INPUT_GPIO_PIN					GPIO_PIN_13
#define	IO2_INPUT_GPIO_PORT					GPIOE
#define	IO2_INPUT_GPIO_PIN					GPIO_PIN_9
#define	IO3_INPUT_GPIO_PORT					GPIOE
#define	IO3_INPUT_GPIO_PIN					GPIO_PIN_8
#define	IO4_INPUT_GPIO_PORT					GPIOE
#define	IO4_INPUT_GPIO_PIN					GPIO_PIN_5
#define	IO5_INPUT_GPIO_PORT					GPIOF
#define	IO5_INPUT_GPIO_PIN					GPIO_PIN_6
#define	IO6_INPUT_GPIO_PORT					GPIOF
#define	IO6_INPUT_GPIO_PIN					GPIO_PIN_7
#define	IO7_INPUT_GPIO_PORT					GPIOB
#define	IO7_INPUT_GPIO_PIN					GPIO_PIN_14
#define	IO8_INPUT_GPIO_PORT					GPIOA
#define	IO8_INPUT_GPIO_PIN					GPIO_PIN_10
#define	IO9_INPUT_GPIO_PORT					GPIOE
#define	IO9_INPUT_GPIO_PIN					GPIO_PIN_0
#define	IO10_INPUT_GPIO_PORT				GPIOG
#define	IO10_INPUT_GPIO_PIN					GPIO_PIN_3
#define	IO11_INPUT_GPIO_PORT				GPIOG
#define	IO11_INPUT_GPIO_PIN					GPIO_PIN_1
											
/*
*********************************************************************************************************
*                              				74HC165
*********************************************************************************************************
*/
#define USE_CHIP_HC165							(false)	
#if (USE_CHIP_HC165)
	#define	HC165_GPIO_CLK						(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOE)
	#define	HC165_GPIO_CLK_ENABLE()				do {\
													RCC_APB2PeriphClockCmd(HC165_GPIO_CLK, ENABLE);\
												} while (0)

	#define	HC165_CLK_GPIO_PIN					GPIO_Pin_7
	#define	HC165_CLK_GPIO_PORT					GPIOA

	#define	HC165_LOCK_GPIO_PIN					GPIO_Pin_4
	#define	HC165_LOCK_GPIO_PORT				GPIOC

	#define	HC165_DAT_GPIO_PIN					GPIO_Pin_6
	#define	HC165_DAT_GPIO_PORT					GPIOA
		
	#define	HC165_GPIO_CLK_SET()				GPIO_SetBits(HC165_CLK_GPIO_PORT, HC165_CLK_GPIO_PIN)
	#define	HC165_GPIO_LOCK_SET() 				GPIO_SetBits(HC165_LOCK_GPIO_PORT, HC165_LOCK_GPIO_PIN)
		
	#define	HC165_GPIO_CLK_CLEAR()				GPIO_ResetBits(HC165_CLK_GPIO_PORT, HC165_CLK_GPIO_PIN)	
	#define	HC165_GPIO_LOCK_CLEAR() 			GPIO_ResetBits(HC165_LOCK_GPIO_PORT, HC165_LOCK_GPIO_PIN)

	#define	HC165_GPIO_DAT_READ()   			GPIO_ReadInputDataBit(HC165_DAT_GPIO_PORT, HC165_DAT_GPIO_PIN)
#endif
#endif

/*
*********************************************************************************************************
*                              				IO 输出
*********************************************************************************************************
*/
#define USE_GPIO_IO_OUTPUT					(false)
#if (USE_GPIO_IO_OUTPUT)
#define IO_OUTPUT_GPIO_CLK_ENABLE()			do {\
													__HAL_RCC_GPIOE_CLK_ENABLE();\
											} while (0)
#define	IO1_OUTPUT_GPIO_PORT				GPIOE
#define	IO1_OUTPUT_GPIO_PIN					GPIO_PIN_6
#define	IO2_OUTPUT_GPIO_PORT				GPIOE
#define	IO2_OUTPUT_GPIO_PIN					GPIO_PIN_5
#endif

/*
*********************************************************************************************************
*                              				74HC595
*********************************************************************************************************
*/
#define USE_CHIP_HC595						(false)
#if (USE_CHIP_HC595)
#define USE_HC595_EN_GPIO					0x01U	//使用芯片使能配置引脚

#define	HC595_GPIO_CLK_ENABLE()				do {\
													__HAL_RCC_GPIOB_CLK_ENABLE();\
													__HAL_RCC_GPIOE_CLK_ENABLE();\
											} while (0)

#define	HC595_RCK_GPIO_PORT					GPIOE
#define	HC595_RCK_GPIO_PIN					GPIO_PIN_4

#define	HC595_SCK_GPIO_PORT					GPIOE
#define	HC595_SCK_GPIO_PIN					GPIO_PIN_0

#define	HC595_SDA_GPIO_PORT					GPIOE
#define	HC595_SDA_GPIO_PIN					GPIO_PIN_3

#define	HC595_SE_GPIO_PORT					GPIOB
#define	HC595_SE_GPIO_PIN					GPIO_PIN_9
	
#define	HC595_GPIO_RCK_SET()				HAL_GPIO_WritePin(HC595_RCK_GPIO_PORT,HC595_RCK_GPIO_PIN,GPIO_PIN_SET)
#define	HC595_GPIO_SCK_SET() 				HAL_GPIO_WritePin(HC595_SCK_GPIO_PORT,HC595_SCK_GPIO_PIN,GPIO_PIN_SET)
#define	HC595_GPIO_SDA_SET() 				HAL_GPIO_WritePin(HC595_SDA_GPIO_PORT,HC595_SDA_GPIO_PIN,GPIO_PIN_SET)	
#define	HC595_GPIO_SE_SET() 				HAL_GPIO_WritePin(HC595_SE_GPIO_PORT,HC595_SE_GPIO_PIN,GPIO_PIN_SET)
 
#define	HC595_GPIO_RCK_CLEAR()				HAL_GPIO_WritePin(HC595_RCK_GPIO_PORT,HC595_RCK_GPIO_PIN,GPIO_PIN_RESET)
#define	HC595_GPIO_SCK_CLEAR() 				HAL_GPIO_WritePin(HC595_SCK_GPIO_PORT,HC595_SCK_GPIO_PIN,GPIO_PIN_RESET)
#define	HC595_GPIO_SDA_CLEAR() 				HAL_GPIO_WritePin(HC595_SDA_GPIO_PORT,HC595_SDA_GPIO_PIN,GPIO_PIN_RESET)	
#define	HC595_GPIO_SE_CLEAR() 				HAL_GPIO_WritePin(HC595_SE_GPIO_PORT,HC595_SE_GPIO_PIN,GPIO_PIN_RESET)
#endif

/*
*********************************************************************************************************
*                              				继电器
*********************************************************************************************************
*/
#define USE_GPIO_IO_RELAY					(true)
#if (USE_GPIO_IO_RELAY)
#define	RELAY_GPIO_CLK_ENABLE()				do {\
												__HAL_RCC_GPIOE_CLK_ENABLE();\
											} while (0)
#define	RELAY1_GPIO_PORT					GPIOE
#define	RELAY1_GPIO_PIN						GPIO_PIN_15
#define	RELAY2_GPIO_PORT					GPIOE
#define	RELAY2_GPIO_PIN						GPIO_PIN_14
#define	RELAY3_GPIO_PORT					GPIOE
#define	RELAY3_GPIO_PIN						GPIO_PIN_13
#define	RELAY4_GPIO_PORT					GPIOE
#define	RELAY4_GPIO_PIN						GPIO_PIN_12
#define	RELAY5_GPIO_PORT					GPIOE
#define	RELAY5_GPIO_PIN						GPIO_PIN_11
#define	RELAY6_GPIO_PORT					GPIOE
#define	RELAY6_GPIO_PIN						GPIO_PIN_10	
#endif

/*
*********************************************************************************************************
*                              				LAMP
*********************************************************************************************************
*/
#define USE_GPIO_IO_LAMP					(true)
#if (USE_GPIO_IO_LAMP)
#define	BOARD_LED_GPIO_CLK_ENABLE()			do {\
												__HAL_RCC_GPIOD_CLK_ENABLE();\
											} while (0)
#define	BOARD_LED1_GPIO_PORT				GPIOD
#define	BOARD_LED1_GPIO_PIN					GPIO_PIN_3
#define	BOARD_LED2_GPIO_PORT				GPIOD
#define	BOARD_LED2_GPIO_PIN					GPIO_PIN_4
#endif
											
#define USE_CHIP_CH455G						(true)
/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/
struct BSP_IODrv {
	BspIO_DevEnum dev;
	uint32_t shadowReg;										//影子寄存器
	uint32_t dataReg;										//数据寄存器
	
	//初始化
	void (*init)(struct BSP_IODrv *this);		//初始化
	
	//读操作
	uint32_t (*read)(struct BSP_IODrv *this);	//读取数据
	void (*sync)(struct BSP_IODrv *this);		//同步
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
static void bsp_InitInport(struct BSP_IODrv *this);
static uint32_t bsp_InportRead(struct BSP_IODrv *this);
static void bsp_InportCycleTask(struct BSP_IODrv *this);

static void bsp_InitOutport(struct BSP_IODrv *this);
static uint32_t bsp_GetOutport(struct BSP_IODrv *this);
static void bsp_OutportCycleTask(struct BSP_IODrv *this);

static void bsp_InitLamp(struct BSP_IODrv *this);
static void bsp_LampCycleTask(struct BSP_IODrv *this);


/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
static struct BSP_IODrv g_BSP_IODev[] = {
{
	.dev						= DEV_BSP_IO_1,
	.init						= bsp_InitInport,
	.read						= bsp_InportRead,
	.sync						= bsp_InportCycleTask,
},
{
	.dev						= DEV_BSP_IO_2,
	.init						= bsp_InitOutport,
	.read						= bsp_GetOutport,
	.sync						= bsp_OutportCycleTask,
},
{
	.dev						= DEV_BSP_IO_3,
	.init						= bsp_InitLamp,
	.read						= bsp_GetOutport,	
	.sync						= bsp_LampCycleTask,
},
};

/*
*********************************************************************************************************
*                              				Private functions
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : GetIOHandle
* Description	: 获取IO句柄
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static struct BSP_IODrv *GetIOHandle(BspIO_DevEnum dev)
{
	struct BSP_IODrv *handle = NULL;
	for (uint8_t i=0; i<ARRAY_SIZE(g_BSP_IODev); ++i) {
		if (dev == g_BSP_IODev[i].dev) {
			handle = &g_BSP_IODev[i];
			break;
		}
	}
	return handle;
}

/*
*********************************************************************************************************
*                              				输入端口
*********************************************************************************************************
*/
#if (USE_CHIP_HC165)
/*
*********************************************************************************************************
* Function Name : Init_74HC165
* Description	: 初始化芯片74HC165
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void Init_74HC165(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; 
	
	HC165_GPIO_CLK_ENABLE();
	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	
	GPIO_InitStructure.GPIO_Pin = HC165_CLK_GPIO_PIN;
	GPIO_Init(HC165_CLK_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = HC165_LOCK_GPIO_PIN;
	GPIO_Init(HC165_LOCK_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	
	GPIO_InitStructure.GPIO_Pin = HC165_DAT_GPIO_PIN;
	GPIO_Init(HC165_DAT_GPIO_PORT, &GPIO_InitStructure);
	
	HC165_GPIO_CLK_CLEAR();
	HC165_GPIO_LOCK_SET();
}

/*
*********************************************************************************************************
* Function Name : __bsp_74HC165_Read
* Description	: 从74HC165读取数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static uint32_t __bsp_74HC165_Read(uint8_t bit)
{
	uint8_t i;
	uint32_t value = 0;
	
	HC165_GPIO_LOCK_CLEAR();
	bsp_DelayUS(1);
	HC165_GPIO_LOCK_SET(); 
	
	for (i=0; i<bit; i++)
	{		  			
		value <<= 1;
		if( HC165_GPIO_DAT_READ() )
		{
			value |= 0x01;
		} 
		
		HC165_GPIO_CLK_SET();
		bsp_DelayUS(1);
		HC165_GPIO_CLK_CLEAR();	
	}
	
	return value;
}

/*
*********************************************************************************************************
* Function Name : bsp_74HC165_Read
* Description	: 从74HC165读取数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
uint32_t bsp_74HC165_Read( void )
{
	return (~__bsp_74HC165_Read(8));
}
#endif

/*
*********************************************************************************************************
* Function Name : bsp_InitInport
* Description	: 初始化输入
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void bsp_InitInport(struct BSP_IODrv *this)
{
	GPIO_InitTypeDef GPIO_InitStructure = {0}; 
#if (USE_GPIO_IO_INPUT)	
	IO_INPUT_GPIO_CLK_ENABLE();	
	GPIO_InitStructure.Pin = IO1_INPUT_GPIO_PIN; 
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(IO1_INPUT_GPIO_PORT, &GPIO_InitStructure); 
	
	GPIO_InitStructure.Pin = IO2_INPUT_GPIO_PIN;
	HAL_GPIO_Init(IO2_INPUT_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = IO3_INPUT_GPIO_PIN;
	HAL_GPIO_Init(IO3_INPUT_GPIO_PORT, &GPIO_InitStructure); 
	
	GPIO_InitStructure.Pin = IO4_INPUT_GPIO_PIN;
	HAL_GPIO_Init(IO4_INPUT_GPIO_PORT, &GPIO_InitStructure); 
	
	GPIO_InitStructure.Pin = IO5_INPUT_GPIO_PIN;
	HAL_GPIO_Init(IO5_INPUT_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = IO6_INPUT_GPIO_PIN;
	HAL_GPIO_Init(IO6_INPUT_GPIO_PORT, &GPIO_InitStructure); 
	
	GPIO_InitStructure.Pin = IO7_INPUT_GPIO_PIN;
	HAL_GPIO_Init(IO7_INPUT_GPIO_PORT, &GPIO_InitStructure); 
	
	GPIO_InitStructure.Pin = IO8_INPUT_GPIO_PIN;
	HAL_GPIO_Init(IO8_INPUT_GPIO_PORT, &GPIO_InitStructure); 
	
	GPIO_InitStructure.Pin = IO9_INPUT_GPIO_PIN;
	HAL_GPIO_Init(IO9_INPUT_GPIO_PORT, &GPIO_InitStructure); 
	
	GPIO_InitStructure.Pin = IO10_INPUT_GPIO_PIN;
	HAL_GPIO_Init(IO10_INPUT_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = IO11_INPUT_GPIO_PIN;
	HAL_GPIO_Init(IO11_INPUT_GPIO_PORT, &GPIO_InitStructure); 
#endif	
#if (USE_CHIP_HC165)
	Init_74HC165();
#endif	

	this->shadowReg = 0x00000000 ^ INPUT_REV;
	this->dataReg   = 0x00000000 ^ INPUT_REV;
	if (this->sync)
	{
		this->sync(this);
	}	
} 
 
/*
*********************************************************************************************************
* Function Name : bsp_InportRead
* Description	: 读取输入端口
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static uint32_t bsp_InportRead(struct BSP_IODrv *this)
{	
	return (this->dataReg ^ INPUT_REV);
}

/*
*********************************************************************************************************
* Function Name : bsp_InportCycleTask
* Description	: 端口输入任务(需要循环执行，已实现防抖)
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void bsp_InportCycleTask(struct BSP_IODrv *this)
{ 	
	uint32_t tempu32 = 0;
	if ( HAL_GPIO_ReadPin(IO1_INPUT_GPIO_PORT, IO1_INPUT_GPIO_PIN) ) {
		tempu32 |= BSP_DI_1;
	}
	if ( HAL_GPIO_ReadPin(IO2_INPUT_GPIO_PORT, IO2_INPUT_GPIO_PIN) ) {
		tempu32 |= BSP_DI_2;
	}	
	if ( HAL_GPIO_ReadPin(IO3_INPUT_GPIO_PORT, IO3_INPUT_GPIO_PIN) ) {
		tempu32 |= BSP_DI_3;
	}	
	if ( HAL_GPIO_ReadPin(IO4_INPUT_GPIO_PORT, IO4_INPUT_GPIO_PIN) ) {
		tempu32 |= BSP_DI_4;
	}	
	if ( HAL_GPIO_ReadPin(IO5_INPUT_GPIO_PORT, IO5_INPUT_GPIO_PIN) ) {
		tempu32 |= BSP_DI_5;
	}	
	if ( HAL_GPIO_ReadPin(IO6_INPUT_GPIO_PORT, IO6_INPUT_GPIO_PIN) ) {
		tempu32 |= BSP_DI_6;
	}	
	if ( HAL_GPIO_ReadPin(IO7_INPUT_GPIO_PORT, IO7_INPUT_GPIO_PIN) ) {
		tempu32 |= BSP_DI_7;
	}
	if ( HAL_GPIO_ReadPin(IO8_INPUT_GPIO_PORT, IO8_INPUT_GPIO_PIN) ) {
		tempu32 |= BSP_DI_8;
	}
	if ( HAL_GPIO_ReadPin(IO9_INPUT_GPIO_PORT, IO9_INPUT_GPIO_PIN) ) {
		tempu32 |= BSP_DI_9;
	}
	if ( HAL_GPIO_ReadPin(IO10_INPUT_GPIO_PORT, IO10_INPUT_GPIO_PIN) ) {
		tempu32 |= BSP_DI_10;
	}
	if ( HAL_GPIO_ReadPin(IO11_INPUT_GPIO_PORT, IO11_INPUT_GPIO_PIN) ) {
		tempu32 |= BSP_DI_11;
	}
#if (USE_CHIP_HC165)	
	uint32_t reg = bsp_74HC165_Read();	
	if (READ_BIT(reg, 0x01)) {
		tempu32 |= BSP_DI_1;
	}
	if (READ_BIT(reg, 0x02)) {
		tempu32 |= BSP_DI_2;
	}
	if (READ_BIT(reg, 0x04)) {
		tempu32 |= BSP_DI_3;
	}
	if (READ_BIT(reg, 0x08)) {
		tempu32 |= BSP_DI_4;
	}
	if (READ_BIT(reg, 0x10)) {
		tempu32 |= BSP_DI_5;
	}
	if (READ_BIT(reg, 0x20)) {
		tempu32 |= BSP_DI_6;
	}
	if (READ_BIT(reg, 0x40)) {
		tempu32 |= BSP_DI_7;
	}
#endif	

	if (this->dataReg != this->shadowReg) {
		/* 连续两次采样数值一致，认为采样是稳定的 */
		if (tempu32 == this->shadowReg) {
			this->dataReg = this->shadowReg;
		}
	}
	this->shadowReg = tempu32;
} 

/*
*********************************************************************************************************
*                              				输出端口
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_InitOutport
* Description	: 端口输出初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/ 
static void bsp_InitOutport(struct BSP_IODrv *this)
{
	GPIO_InitTypeDef GPIO_InitStructure = {0}; 
#if (USE_GPIO_IO_OUTPUT)	
	/* 初始化IO输出 */
	IO_OUTPUT_GPIO_CLK_ENABLE();
	
	GPIO_InitStructure.Pin = IO1_OUTPUT_GPIO_PIN; 
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(IO1_OUTPUT_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = IO2_OUTPUT_GPIO_PIN; 
	HAL_GPIO_Init(IO2_OUTPUT_GPIO_PORT, &GPIO_InitStructure);
#endif
	
#if (USE_GPIO_IO_RELAY)
	/* 初始化继电器输出 */	
	RELAY_GPIO_CLK_ENABLE();
	
	GPIO_InitStructure.Pin = RELAY1_GPIO_PIN; 
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(RELAY1_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = RELAY2_GPIO_PIN;
	HAL_GPIO_Init(RELAY2_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = RELAY3_GPIO_PIN;
	HAL_GPIO_Init(RELAY3_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = RELAY4_GPIO_PIN;
	HAL_GPIO_Init(RELAY4_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = RELAY5_GPIO_PIN;
	HAL_GPIO_Init(RELAY5_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = RELAY6_GPIO_PIN;
	HAL_GPIO_Init(RELAY6_GPIO_PORT, &GPIO_InitStructure);
#endif	
	
	this->shadowReg = 0x00000000;
	this->dataReg = ~this->shadowReg;
	if (this->sync)
	{
		this->sync(this);
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_OutportCycleTask
* Description	: 端口输出任务(需要循环执行)
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void bsp_OutportCycleTask(struct BSP_IODrv *this)
{
	uint32_t flag = this->dataReg ^ this->shadowReg; 
	
	//如果有输出需要改变，刷新输出
	if (flag)
	{
		#if (USE_GPIO_IO_OUTPUT)
		{
			if (flag & BSP_DO_1) {
				if (this->shadowReg & BSP_DO_1) {
					 HAL_GPIO_WritePin(IO1_OUTPUT_GPIO_PORT, IO1_OUTPUT_GPIO_PIN, GPIO_PIN_RESET);
				} else {				 
					HAL_GPIO_WritePin(IO1_OUTPUT_GPIO_PORT, IO1_OUTPUT_GPIO_PIN, GPIO_PIN_SET);
				}	
			}
			if (flag & BSP_DO_2) {
				if (this->shadowReg & BSP_DO_2) {
					 HAL_GPIO_WritePin(IO2_OUTPUT_GPIO_PORT, IO2_OUTPUT_GPIO_PIN, GPIO_PIN_RESET);
				} else {				 
					HAL_GPIO_WritePin(IO2_OUTPUT_GPIO_PORT, IO2_OUTPUT_GPIO_PIN, GPIO_PIN_SET);
				}	
			}
		}
		#endif
			
		#if (USE_GPIO_IO_RELAY)
		{			
			if (flag & BSP_DO_1) {	
				if (this->shadowReg & BSP_DO_1) {
					HAL_GPIO_WritePin(RELAY1_GPIO_PORT, RELAY1_GPIO_PIN, GPIO_PIN_SET); 					
				} else {
					HAL_GPIO_WritePin(RELAY1_GPIO_PORT, RELAY1_GPIO_PIN, GPIO_PIN_RESET); 
				}	 
			}
			if (flag & BSP_DO_2) {	
				if (this->shadowReg & BSP_DO_2) {
					HAL_GPIO_WritePin(RELAY2_GPIO_PORT, RELAY2_GPIO_PIN, GPIO_PIN_SET); 					
				} else {
					HAL_GPIO_WritePin(RELAY2_GPIO_PORT, RELAY2_GPIO_PIN, GPIO_PIN_RESET); 					
				}	 
			}
			if (flag & BSP_DO_3) {	
				if (this->shadowReg & BSP_DO_3) {
					HAL_GPIO_WritePin(RELAY3_GPIO_PORT, RELAY3_GPIO_PIN, GPIO_PIN_SET); 					
				} else {
					HAL_GPIO_WritePin(RELAY3_GPIO_PORT, RELAY3_GPIO_PIN, GPIO_PIN_RESET); 					
				}	 
			}
			if (flag & BSP_DO_4) {	
				if (this->shadowReg & BSP_DO_4) {
					HAL_GPIO_WritePin(RELAY4_GPIO_PORT, RELAY4_GPIO_PIN, GPIO_PIN_SET); 					
				} else {
					HAL_GPIO_WritePin(RELAY4_GPIO_PORT, RELAY4_GPIO_PIN, GPIO_PIN_RESET); 
				}	 
			}
			if (flag & BSP_DO_5) {	
				if (this->shadowReg & BSP_DO_5) {
					HAL_GPIO_WritePin(RELAY5_GPIO_PORT, RELAY5_GPIO_PIN, GPIO_PIN_SET); 					
				} else {
					HAL_GPIO_WritePin(RELAY5_GPIO_PORT, RELAY5_GPIO_PIN, GPIO_PIN_RESET); 					
				}	 
			}
			if (flag & BSP_DO_6) {	
				if (this->shadowReg & BSP_DO_6) {
					HAL_GPIO_WritePin(RELAY6_GPIO_PORT, RELAY6_GPIO_PIN, GPIO_PIN_SET); 					
				} else {
					HAL_GPIO_WritePin(RELAY6_GPIO_PORT, RELAY6_GPIO_PIN, GPIO_PIN_RESET); 					
				}
			}
		}
		#endif	
		this->dataReg = this->shadowReg;
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_SetOutportBit
* Description	: 设置输出位
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void bsp_SetOutportBit(struct BSP_IODrv *this, uint32_t bit)
{ 
	SET_BIT(this->shadowReg, bit);
} 

/*
*********************************************************************************************************
* Function Name : bsp_ClrOutportBit
* Description	: 清除输出位
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void bsp_ClrOutportBit(struct BSP_IODrv *this, uint32_t bit)
{ 
	CLEAR_BIT(this->shadowReg, bit);
} 
  
/*
*********************************************************************************************************
* Function Name : bsp_ToggleOutportBit
* Description	: 翻转输出位
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void bsp_RevOutportBit(struct BSP_IODrv *this, uint32_t bit)
{ 
	XOR_BIT(this->shadowReg, bit); 
} 
    
/*
*********************************************************************************************************
* Function Name : bsp_SetOutport
* Description	: 设置输出值
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void bsp_SetOutport(struct BSP_IODrv *this, uint32_t value)
{ 
	WRITE_REG(this->shadowReg, value);
	
	this->dataReg = ~this->shadowReg;
	
	/**
		需要立即执行同步，因为此时数据寄存器的值为临时值,
		如果此时读取数据寄存器将出现错误。
	*/
	if (this->sync)
	{
		this->sync(this);
	}
}
 
/*
*********************************************************************************************************
* Function Name : bsp_GetOutport
* Description	: 获取输出值
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static uint32_t bsp_GetOutport(struct BSP_IODrv *this)
{
	return this->dataReg;
}

/*
*********************************************************************************************************
*                              				LAMP
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_InitLamp
* Description	: 指示灯初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void bsp_InitLamp(struct BSP_IODrv *this)
{
#if (USE_GPIO_IO_LAMP)
{
	GPIO_InitTypeDef GPIO_InitStructure = {0}; 
	
	BOARD_LED_GPIO_CLK_ENABLE();	
	GPIO_InitStructure.Pin = BOARD_LED1_GPIO_PIN; 
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BOARD_LED1_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = BOARD_LED2_GPIO_PIN;
	HAL_GPIO_Init(BOARD_LED2_GPIO_PORT, &GPIO_InitStructure);
}
#endif	
	this->shadowReg = 0x00000000;
	this->dataReg = ~this->shadowReg;
	if (this->sync)
	{
		this->sync(this);
	}
}  

/*
*********************************************************************************************************
* Function Name : bsp_LampCycleTask
* Description	: 指示灯任务，需要循环执行
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void bsp_LampCycleTask(struct BSP_IODrv *this)
{
	uint32_t flag = this->dataReg ^ this->shadowReg; 	
	
	if (flag)		//如果有输出需要改变，刷新输出
	{
	#if (USE_GPIO_IO_LAMP)
		if (flag & BSP_LAMP_1) {	
			if (this->shadowReg & BSP_LAMP_1) {
				HAL_GPIO_WritePin(BOARD_LED1_GPIO_PORT, BOARD_LED1_GPIO_PIN, GPIO_PIN_RESET); 
			} else {			
				HAL_GPIO_WritePin(BOARD_LED1_GPIO_PORT, BOARD_LED1_GPIO_PIN, GPIO_PIN_SET); 
			}	 
		}
		
		if (flag & BSP_LAMP_2) {
			if (this->shadowReg & BSP_LAMP_2) {
				HAL_GPIO_WritePin(BOARD_LED2_GPIO_PORT, BOARD_LED2_GPIO_PIN, GPIO_PIN_RESET); 
			} else {			
				HAL_GPIO_WritePin(BOARD_LED2_GPIO_PORT, BOARD_LED2_GPIO_PIN, GPIO_PIN_SET); 
			}	 
		}
	#endif
		
	#if (USE_CHIP_CH455G)
	{
		uint32_t value = 0;
		bool refresh = false;
		uint32_t bit = 0;
		uint32_t start = BSP_LAMP_3;
		for (uint32_t i=0; i<26; ++i) {
			if (flag & start) {
				refresh = true;
			}
			if (this->shadowReg & start) {
				SET_BIT(value, 1 << bit);
			}
			start <<= 1;
			bit++;
			if (bit == 2) {
				bit = 8;
			}
		}
		if (refresh) {
			bsp_CH455G_WriteSeg(DEV_CH455G_1, value);
		}
	}
	#endif	
		this->dataReg = this->shadowReg;
	}
}

/*
*********************************************************************************************************
*                              				API
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_IORead
* Description	: 读取IO
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
uint32_t bsp_IORead(BspIO_DevEnum dev)
{
	struct BSP_IODrv *this = GetIOHandle(dev);
	if (this == NULL) {
		return 0;
	}
	return this->read(this);
}

/*
*********************************************************************************************************
* Function Name : bsp_IOSync
* Description	: 同步IO
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_IOSync(BspIO_DevEnum dev)
{
	struct BSP_IODrv *this = GetIOHandle(dev);
	if (this == NULL) {
		return;
	}
	if (this->sync) {
		this->sync(this);
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_IOReadSync
* Description	: 同步读取IO
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
uint32_t bsp_IOReadSync(BspIO_DevEnum dev)
{
	uint32_t in = 0;
	struct BSP_IODrv *this = GetIOHandle(dev);
	if (this == NULL) {
		return in;
	}
	if (this->sync) {
		this->sync(this);
		if (this->read) {
			in = this->read(this);
		}
	}
	return in;
}

/*
*********************************************************************************************************
* Function Name : bsp_IOWrite
* Description	: 写入IO
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_IOWrite(BspIO_DevEnum dev, uint32_t out)
{
	struct BSP_IODrv *this = GetIOHandle(dev);
	if (this == NULL) {
		return;
	}
	bsp_SetOutport(this, out);
}

/*
*********************************************************************************************************
* Function Name : bsp_IOSetBit
* Description	: 设置IO位
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_IOSetBit(BspIO_DevEnum dev, uint32_t bit)
{
	struct BSP_IODrv *this = GetIOHandle(dev);
	if (this == NULL) {
		return;
	}
	bsp_SetOutportBit(this, bit);
}

/*
*********************************************************************************************************
* Function Name : bsp_IOClrBit
* Description	: 清除IO位
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_IOClrBit(BspIO_DevEnum dev, uint32_t bit)
{
	struct BSP_IODrv *this = GetIOHandle(dev);
	if (this == NULL) {
		return;
	}
	bsp_ClrOutportBit(this, bit);
}

/*
*********************************************************************************************************
* Function Name : bsp_IORevBit
* Description	: 取反IO位
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_IORevBit(BspIO_DevEnum dev, uint32_t bit)
{
	struct BSP_IODrv *this = GetIOHandle(dev);
	if (this == NULL) {
		return;
	}
	bsp_RevOutportBit(this, bit);
}

/*
*********************************************************************************************************
* Function Name : bsp_InitIO
* Description	: 初始化IO
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitIO(void)
{
	for (uint8_t i=0; i<ARRAY_SIZE(g_BSP_IODev); ++i) {
		if (g_BSP_IODev[i].init) {
			g_BSP_IODev[i].init(&g_BSP_IODev[i]);
		}
	}
	ECHO(ECHO_DEBUG, "[BSP] IO 初始化 .......... OK");
}

/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
