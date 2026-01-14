/*
*********************************************************************************************************
* @file    	bsp_i2c_bus.c
* @author  	SY
* @version 	V1.0.0
* @date    	2016-7-10 09:42:53
* @IDE	 	Keil V4.72.10.0
* @Chip    	STM32F407VE
* @brief   	I2C总线源文件
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
	CPU主频168MHz时，在内部Flash运行, MDK工程不优化。用台式示波器观测波形。
	循环次数为5时，SCL频率 = 1.78MHz (读耗时: 92ms, 读写正常，但是用示波器探头碰上就读写失败。时序接近临界)
	循环次数为10时，SCL频率 = 1.1MHz (读耗时: 138ms, 读速度: 118724B/s)
	循环次数为30时，SCL频率 = 440KHz， SCL高电平时间1.0us，SCL低电平时间1.2us

	上拉电阻选择2.2K欧时，SCL上升沿时间约0.5us，如果选4.7K欧，则上升沿约1us

	实际应用选择400KHz左右的速率即可
*/

#define I2C_EEP_DELAY_TIME			(30)	//EEP延时
#define I2C_FRAM_DELAY_TIME			(10)	//铁电延时
#define I2C_CH455G_DELAY_TIME		(5)		//CH455G延时

#define I2C1_EN						(true)
#if (I2C1_EN)
#define I2C1_BUS_DELAY				(I2C_EEP_DELAY_TIME)
static __INLINE void SetI2C1_GpioClk(void) {
	__HAL_RCC_GPIOB_CLK_ENABLE();\
}
#define	I2C1_SCL_GPIO_PORT 			GPIOB
#define	I2C1_SCL_GPIO_PIN			GPIO_PIN_8
#define	I2C1_SDA_GPIO_PORT 			GPIOB
#define	I2C1_SDA_GPIO_PIN			GPIO_PIN_9
#endif

#define I2C2_EN						(true)
#if (I2C2_EN)
#define I2C2_BUS_DELAY				(I2C_EEP_DELAY_TIME)
static __INLINE void SetI2C2_GpioClk(void) {
	__HAL_RCC_GPIOD_CLK_ENABLE();\
}
#define	I2C2_SCL_GPIO_PORT 			GPIOD
#define	I2C2_SCL_GPIO_PIN			GPIO_PIN_11
#define	I2C2_SDA_GPIO_PORT 			GPIOD
#define	I2C2_SDA_GPIO_PIN			GPIO_PIN_10
#endif

#define I2C3_EN						(true)
#if (I2C3_EN)
#define I2C3_BUS_DELAY				(I2C_CH455G_DELAY_TIME)
static __INLINE void SetI2C3_GpioClk(void) {
	__HAL_RCC_GPIOF_CLK_ENABLE();\
}
#define	I2C3_SCL_GPIO_PORT 			GPIOF
#define	I2C3_SCL_GPIO_PIN			GPIO_PIN_1
#define	I2C3_SDA_GPIO_PORT 			GPIOF
#define	I2C3_SDA_GPIO_PIN			GPIO_PIN_0
#endif

#define I2C4_EN						(false)
#if (I2C4_EN)
#define I2C4_BUS_DELAY				(I2C_EEP_DELAY_TIME)
static __INLINE void SetI2C4_GpioClk(void) {
	__HAL_RCC_GPIOD_CLK_ENABLE();\
}
#define	I2C4_SCL_GPIO_PORT 			GPIOD
#define	I2C4_SCL_GPIO_PIN			GPIO_PIN_13
#define	I2C4_SDA_GPIO_PORT 			GPIOD
#define	I2C4_SDA_GPIO_PIN			GPIO_PIN_12
#endif

#define I2C5_EN						(false)
#if (I2C5_EN)
#define I2C5_BUS_DELAY				(I2C_EEP_DELAY_TIME)
static __INLINE void SetI2C5_GpioClk(void) {
	__HAL_RCC_GPIOE_CLK_ENABLE();\
}
#define	I2C5_SCL_GPIO_PORT 			GPIOE
#define	I2C5_SCL_GPIO_PIN			GPIO_PIN_12
#define	I2C5_SDA_GPIO_PORT 			GPIOE
#define	I2C5_SDA_GPIO_PIN			GPIO_PIN_10
#endif

#define I2C6_EN						(false)
#if (I2C6_EN)
#define I2C6_BUS_DELAY				(I2C_EEP_DELAY_TIME)
static __INLINE void SetI2C6_GpioClk(void) {
	__HAL_RCC_GPIOD_CLK_ENABLE();\
}
#define	I2C6_SCL_GPIO_PORT 			GPIOD
#define	I2C6_SCL_GPIO_PIN			GPIO_PIN_3
#define	I2C6_SDA_GPIO_PORT 			GPIOD
#define	I2C6_SDA_GPIO_PIN			GPIO_PIN_2
#endif

#define I2C7_EN						(false)
#if (I2C7_EN)
#define I2C7_BUS_DELAY				(I2C_EEP_DELAY_TIME)
static __INLINE void SetI2C7_GpioClk(void) {
	__HAL_RCC_GPIOB_CLK_ENABLE();\
}
#define	I2C7_SCL_GPIO_PORT 			GPIOB
#define	I2C7_SCL_GPIO_PIN			GPIO_PIN_11
#define	I2C7_SDA_GPIO_PORT 			GPIOB
#define	I2C7_SDA_GPIO_PIN			GPIO_PIN_10
#endif


/* 定义读写SCL和SDA的宏 */
#define I2C_SCL_1(this)  			do {\
										HAL_GPIO_WritePin(this->sclPort, this->sclPin, GPIO_PIN_SET);\
									} while(0)										/* SCL = 1 */
#define I2C_SCL_0(this)  			do {\
										HAL_GPIO_WritePin(this->sclPort, this->sclPin, GPIO_PIN_RESET);\
									} while(0)										/* SCL = 0 */

#define I2C_SDA_1(this)  			do {\
										HAL_GPIO_WritePin(this->sdaPort, this->sdaPin, GPIO_PIN_SET);\
									} while(0)										/* SDA = 1 */		
#define I2C_SDA_0(this)  			do {\
										HAL_GPIO_WritePin(this->sdaPort, this->sdaPin, GPIO_PIN_RESET);\
									} while(0)										/* SDA = 0 */

#define I2C_SDA_READ(this)  		HAL_GPIO_ReadPin(this->sdaPort, this->sdaPin)	/* 读SDA口线状态 */
#define I2C_SCL_READ(this)  		HAL_GPIO_ReadPin(this->sclPort, this->sclPin)	/* 读SCL口线状态 */
									
									
/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/
struct I2CDrv {
	I2C_DevEnum dev;					//设备
	uint32_t delayTime;
	GPIO_TypeDef *sclPort;
	uint16_t sclPin;
	GPIO_TypeDef *sdaPort;
	uint16_t sdaPin;
	
	void (*setGpioClk)(void);
	bool (*init)(struct I2CDrv *this);
	void (*delay)(struct I2CDrv *this);
	void (*start)(struct I2CDrv *this);
	void (*stop)(struct I2CDrv *this);
	void (*sendByte)(struct I2CDrv *this, uint8_t _ucByte);
	uint8_t (*readByte)(struct I2CDrv *this);
	bool (*waitAck)(struct I2CDrv *this);
	void (*ack)(struct I2CDrv *this);
	void (*nAck)(struct I2CDrv *this);
	bool (*checkDevice)(struct I2CDrv *this, uint8_t _Address);
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
static bool i2c_Init(struct I2CDrv *this);
static void i2c_Delay(struct I2CDrv *this);
static void i2c_Start(struct I2CDrv *this);
static void i2c_Stop(struct I2CDrv *this);
static void i2c_SendByte(struct I2CDrv *this, uint8_t _ucByte);
static uint8_t i2c_ReadByte(struct I2CDrv *this);
static bool i2c_WaitAck(struct I2CDrv *this);
static void i2c_Ack(struct I2CDrv *this);
static void i2c_NAck(struct I2CDrv *this);
static bool i2c_CheckDevice(struct I2CDrv *this, uint8_t _Address);

/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
static struct I2CDrv g_I2CDev[] = {
#if (I2C1_EN)
{
	.dev					= DEV_I2C1,
	.delayTime				= I2C1_BUS_DELAY,
	.sclPort				= I2C1_SCL_GPIO_PORT,
	.sclPin					= I2C1_SCL_GPIO_PIN,
	.sdaPort				= I2C1_SDA_GPIO_PORT,
	.sdaPin					= I2C1_SDA_GPIO_PIN,
	
	.setGpioClk				= SetI2C1_GpioClk,
	.init					= i2c_Init,
	.delay					= i2c_Delay,
	.start					= i2c_Start,
	.stop					= i2c_Stop,
	.sendByte				= i2c_SendByte,
	.readByte				= i2c_ReadByte,
	.waitAck				= i2c_WaitAck,
	.ack					= i2c_Ack,
	.nAck					= i2c_NAck,
	.checkDevice			= i2c_CheckDevice,
},
#endif

#if (I2C2_EN)
{
	.dev					= DEV_I2C2,
	.delayTime				= I2C2_BUS_DELAY,
	.sclPort				= I2C2_SCL_GPIO_PORT,
	.sclPin					= I2C2_SCL_GPIO_PIN,
	.sdaPort				= I2C2_SDA_GPIO_PORT,
	.sdaPin					= I2C2_SDA_GPIO_PIN,
	
	.setGpioClk				= SetI2C2_GpioClk,
	.init					= i2c_Init,
	.delay					= i2c_Delay,
	.start					= i2c_Start,
	.stop					= i2c_Stop,
	.sendByte				= i2c_SendByte,
	.readByte				= i2c_ReadByte,
	.waitAck				= i2c_WaitAck,
	.ack					= i2c_Ack,
	.nAck					= i2c_NAck,
	.checkDevice			= i2c_CheckDevice,
},
#endif

#if (I2C3_EN)
{
	.dev					= DEV_I2C3,
	.delayTime				= I2C3_BUS_DELAY,
	.sclPort				= I2C3_SCL_GPIO_PORT,
	.sclPin					= I2C3_SCL_GPIO_PIN,
	.sdaPort				= I2C3_SDA_GPIO_PORT,
	.sdaPin					= I2C3_SDA_GPIO_PIN,
	
	.setGpioClk				= SetI2C3_GpioClk,
	.init					= i2c_Init,
	.delay					= i2c_Delay,
	.start					= i2c_Start,
	.stop					= i2c_Stop,
	.sendByte				= i2c_SendByte,
	.readByte				= i2c_ReadByte,
	.waitAck				= i2c_WaitAck,
	.ack					= i2c_Ack,
	.nAck					= i2c_NAck,
	.checkDevice			= i2c_CheckDevice,
},
#endif

#if (I2C4_EN)
{
	.dev					= DEV_I2C4,
	.delayTime				= I2C4_BUS_DELAY,
	.sclPort				= I2C4_SCL_GPIO_PORT,
	.sclPin					= I2C4_SCL_GPIO_PIN,
	.sdaPort				= I2C4_SDA_GPIO_PORT,
	.sdaPin					= I2C4_SDA_GPIO_PIN,
	
	.setGpioClk				= SetI2C4_GpioClk,
	.init					= i2c_Init,
	.delay					= i2c_Delay,
	.start					= i2c_Start,
	.stop					= i2c_Stop,
	.sendByte				= i2c_SendByte,
	.readByte				= i2c_ReadByte,
	.waitAck				= i2c_WaitAck,
	.ack					= i2c_Ack,
	.nAck					= i2c_NAck,
	.checkDevice			= i2c_CheckDevice,
},
#endif

#if (I2C5_EN)
{
	.dev					= DEV_I2C5,
	.delayTime				= I2C5_BUS_DELAY,
	.sclPort				= I2C5_SCL_GPIO_PORT,
	.sclPin					= I2C5_SCL_GPIO_PIN,
	.sdaPort				= I2C5_SDA_GPIO_PORT,
	.sdaPin					= I2C5_SDA_GPIO_PIN,
	
	.setGpioClk				= SetI2C5_GpioClk,
	.init					= i2c_Init,
	.delay					= i2c_Delay,
	.start					= i2c_Start,
	.stop					= i2c_Stop,
	.sendByte				= i2c_SendByte,
	.readByte				= i2c_ReadByte,
	.waitAck				= i2c_WaitAck,
	.ack					= i2c_Ack,
	.nAck					= i2c_NAck,
	.checkDevice			= i2c_CheckDevice,
},
#endif

#if (I2C6_EN)
{
	.dev					= DEV_I2C6,
	.delayTime				= I2C6_BUS_DELAY,
	.sclPort				= I2C6_SCL_GPIO_PORT,
	.sclPin					= I2C6_SCL_GPIO_PIN,
	.sdaPort				= I2C6_SDA_GPIO_PORT,
	.sdaPin					= I2C6_SDA_GPIO_PIN,
	
	.setGpioClk				= SetI2C6_GpioClk,
	.init					= i2c_Init,
	.delay					= i2c_Delay,
	.start					= i2c_Start,
	.stop					= i2c_Stop,
	.sendByte				= i2c_SendByte,
	.readByte				= i2c_ReadByte,
	.waitAck				= i2c_WaitAck,
	.ack					= i2c_Ack,
	.nAck					= i2c_NAck,
	.checkDevice			= i2c_CheckDevice,
},
#endif

#if (I2C7_EN)
{
	.dev					= DEV_I2C7,
	.delayTime				= I2C7_BUS_DELAY,
	.sclPort				= I2C7_SCL_GPIO_PORT,
	.sclPin					= I2C7_SCL_GPIO_PIN,
	.sdaPort				= I2C7_SDA_GPIO_PORT,
	.sdaPin					= I2C7_SDA_GPIO_PIN,
	
	.setGpioClk				= SetI2C7_GpioClk,
	.init					= i2c_Init,
	.delay					= i2c_Delay,
	.start					= i2c_Start,
	.stop					= i2c_Stop,
	.sendByte				= i2c_SendByte,
	.readByte				= i2c_ReadByte,
	.waitAck				= i2c_WaitAck,
	.ack					= i2c_Ack,
	.nAck					= i2c_NAck,
	.checkDevice			= i2c_CheckDevice,
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
static struct I2CDrv *GetDevHandle(I2C_DevEnum dev)
{
	struct I2CDrv *handle = NULL;	
	for (uint8_t i=0; i<ARRAY_SIZE(g_I2CDev); ++i) {		
		if (dev == g_I2CDev[i].dev) {
			handle = &g_I2CDev[i];
			break;
		}
	}	
	return handle;
}

/*
*********************************************************************************************************
* Function Name : i2c_Init
* Description	: I2C初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool i2c_Init(struct I2CDrv *this)
{
	if (this->setGpioClk)
	{
		this->setGpioClk();
	}
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = this->sclPin; 
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(this->sclPort, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = this->sdaPin; 
	HAL_GPIO_Init(this->sdaPort, &GPIO_InitStructure);

	/* 给一个停止信号, 复位I2C总线上的所有设备到待机模式 */
	if (this->stop)
	{
		this->stop(this);
	}
	
	return true;
}

/*
*********************************************************************************************************
* Function Name : i2c_Delay
* Description	: I2C延时
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void i2c_Delay(struct I2CDrv *this)
{
	for (uint8_t i = 0; i<this->delayTime; i++);
}

/*
*********************************************************************************************************
* Function Name : i2c_Start
* Description	: I2C发送开始
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void i2c_Start(struct I2CDrv *this)
{
	/* 当SCL高电平时，SDA出现一个下跳沿表示I2C总线启动信号 */
	I2C_SDA_1(this);
	I2C_SCL_1(this);
	i2c_Delay(this);
	I2C_SDA_0(this);
	i2c_Delay(this);
	
	I2C_SCL_0(this);
	i2c_Delay(this);
}

/*
*********************************************************************************************************
* Function Name : i2c_Stop
* Description	: CPU发起I2C总线停止信号
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void i2c_Stop(struct I2CDrv *this)
{
	/* 当SCL高电平时，SDA出现一个上跳沿表示I2C总线停止信号 */
	I2C_SDA_0(this);
	I2C_SCL_1(this);
	i2c_Delay(this);
	I2C_SDA_1(this);
	i2c_Delay(this);
}

/*
*********************************************************************************************************
* Function Name : i2c_SendByte
* Description	: CPU向I2C总线设备发送8bit数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void i2c_SendByte(struct I2CDrv *this, uint8_t _ucByte)
{
	/* 先发送字节的高位bit7 */
	for (uint8_t i = 0; i < 8; i++)
	{
		if (_ucByte & 0x80)
		{
			I2C_SDA_1(this);
		}
		else
		{
			I2C_SDA_0(this);
		}
		i2c_Delay(this);
		I2C_SCL_1(this);
		i2c_Delay(this);
		I2C_SCL_0(this);
		if (i == 7)
		{
			 I2C_SDA_1(this); // 释放总线
		}
		_ucByte <<= 1;	/* 左移一个bit */
		i2c_Delay(this);
	}
}

/*
*********************************************************************************************************
* Function Name : i2c_ReadByte
* Description	: CPU从I2C总线设备读取8bit数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static uint8_t i2c_ReadByte(struct I2CDrv *this)
{
	/* 读到第1个bit为数据的bit7 */
	uint8_t value = 0;
	for (uint8_t i = 0; i < 8; i++)
	{
		value <<= 1;
		I2C_SCL_1(this);
		i2c_Delay(this);
		if (I2C_SDA_READ(this))
		{
			value++;
		}
		I2C_SCL_0(this);
		i2c_Delay(this);
	}
	return value;
}

/*
*********************************************************************************************************
* Function Name : i2c_WaitAck
* Description	: CPU产生一个时钟，并读取器件的ACK应答信号
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool i2c_WaitAck(struct I2CDrv *this)
{
	bool re;

	I2C_SDA_1(this);	/* CPU释放SDA总线 */
	i2c_Delay(this);
	I2C_SCL_1(this);	/* CPU驱动SCL = 1, 此时器件会返回ACK应答 */
	i2c_Delay(this);
	if (I2C_SDA_READ(this))	/* CPU读取SDA口线状态 */
	{
		re = false;
	}
	else
	{
		re = true;
	}
	I2C_SCL_0(this);
	i2c_Delay(this);
	return re;
}

/*
*********************************************************************************************************
* Function Name : i2c_Ack
* Description	: CPU产生一个ACK信号
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void i2c_Ack(struct I2CDrv *this)
{
	I2C_SDA_0(this);	/* CPU驱动SDA = 0 */
	i2c_Delay(this);
	I2C_SCL_1(this);	/* CPU产生1个时钟 */
	i2c_Delay(this);
	I2C_SCL_0(this);
	i2c_Delay(this);
	I2C_SDA_1(this);	/* CPU释放SDA总线 */
}

/*
*********************************************************************************************************
* Function Name : i2c_NAck
* Description	: CPU产生1个NACK信号
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void i2c_NAck(struct I2CDrv *this)
{
	I2C_SDA_1(this);	/* CPU驱动SDA = 1 */
	i2c_Delay(this);
	I2C_SCL_1(this);	/* CPU产生1个时钟 */
	i2c_Delay(this);
	I2C_SCL_0(this);
	i2c_Delay(this);
}

/*
*********************************************************************************************************
* Function Name : i2c_CheckDevice
* Description	: 检测I2C总线设备，CPU向发送设备地址，然后读取设备应答来判断该设备是否存在
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool i2c_CheckDevice(struct I2CDrv *this, uint8_t _Address)
{
	bool ucAck;

	if (I2C_SDA_READ(this) && I2C_SCL_READ(this))
	{
		i2c_Start(this);		/* 发送启动信号 */

		/* 发送设备地址+读写控制bit（0 = w， 1 = r) bit7 先传 */
		i2c_SendByte(this, _Address | I2C_WR);
		ucAck = i2c_WaitAck(this);	/* 检测设备的ACK应答 */

		i2c_Stop(this);			/* 发送停止信号 */

		return ucAck;
	}
	return false;	/* I2C总线异常 */
}

/*
*********************************************************************************************************
*                              				API
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_InitI2CBus
* Description	: 初始化I2C总线
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitI2CBus(void)
{
	for (uint8_t i=0; i<ARRAY_SIZE(g_I2CDev); ++i) {
		if (g_I2CDev[i].init) {
			bool ret = g_I2CDev[i].init(&g_I2CDev[i]);
			ECHO(ECHO_DEBUG, "[BSP] I2C %d 初始化 .......... %s", g_I2CDev[i].dev+1, (ret == true) ? "OK" : "ERROR");
		}
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_I2CStart
* Description	: I2C开始
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_I2CStart(I2C_DevEnum dev)
{
	struct I2CDrv *this = GetDevHandle(dev);
	if (this == NULL) {
		return;
	}
	if (this->start) {
		this->start(this);
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_I2CStop
* Description	: I2C停止
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_I2CStop(I2C_DevEnum dev)
{
	struct I2CDrv *this = GetDevHandle(dev);
	if (this == NULL) {
		return;
	}
	if (this->stop) {
		this->stop(this);
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_I2CSendByte
* Description	: I2C发送字节
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_I2CSendByte(I2C_DevEnum dev, uint8_t _ucByte)
{
	struct I2CDrv *this = GetDevHandle(dev);
	if (this == NULL) {
		return;
	}
	if (this->sendByte) {
		this->sendByte(this, _ucByte);
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_I2CReadByte
* Description	: I2C读取字节
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
uint8_t bsp_I2CReadByte(I2C_DevEnum dev)
{
	uint8_t _ucByte = 0;
	struct I2CDrv *this = GetDevHandle(dev);
	if (this == NULL) {
		return _ucByte;
	}
	if (this->readByte) {
		_ucByte = this->readByte(this);
	}
	return _ucByte;
}

/*
*********************************************************************************************************
* Function Name : bsp_I2CWaitAck
* Description	: I2C等待应答
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_I2CWaitAck(I2C_DevEnum dev)
{
	bool ret = false;
	struct I2CDrv *this = GetDevHandle(dev);
	if (this == NULL) {
		return ret;
	}
	if (this->waitAck) {
		ret = this->waitAck(this);
	}
	return ret;
}

/*
*********************************************************************************************************
* Function Name : bsp_I2CAck
* Description	: I2C应答
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_I2CAck(I2C_DevEnum dev)
{
	struct I2CDrv *this = GetDevHandle(dev);
	if (this == NULL) {
		return;
	}
	if (this->ack) {
		this->ack(this);
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_I2CnAck
* Description	: I2C不应答
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_I2CnAck(I2C_DevEnum dev)
{
	struct I2CDrv *this = GetDevHandle(dev);
	if (this == NULL) {
		return;
	}
	if (this->nAck) {
		this->nAck(this);
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_I2CCheckDevice
* Description	: I2C检测设备
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_I2CCheckDevice(I2C_DevEnum dev, uint8_t _Address)
{
	bool ret = false;
	struct I2CDrv *this = GetDevHandle(dev);
	if (this == NULL) {
		return ret;
	}
	if (this->checkDevice) {
		ret = this->checkDevice(this, _Address);
	}
	return ret;
}

/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
