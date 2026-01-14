/*
*********************************************************************************************************
* @file    	bsp_ch455g.c
* @author  	SY
* @version 	V1.0.0
* @date    	2016-12-15 20:08:56
* @IDE	 	Keil V5.22.0.0
* @Chip    	STM32F407VE
* @brief   	CH455G驱动源文件
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
// 设置系统参数命令
#define CH455_BIT_ENABLE	0x01		// 开启/关闭位
#define CH455_BIT_SLEEP		0x04		// 睡眠控制位
#define CH455_BIT_7SEG		0x08		// 7段控制位
#define CH455_BIT_INTENS1	0x10		// 1级亮度
#define CH455_BIT_INTENS2	0x20		// 2级亮度
#define CH455_BIT_INTENS3	0x30		// 3级亮度
#define CH455_BIT_INTENS4	0x40		// 4级亮度
#define CH455_BIT_INTENS5	0x50		// 5级亮度
#define CH455_BIT_INTENS6	0x60		// 6级亮度
#define CH455_BIT_INTENS7	0x70		// 7级亮度
#define CH455_BIT_INTENS8	0x00		// 8级亮度

#define CH455_SYSOFF	0x0400			// 关闭显示、关闭键盘
#define CH455_SYSON		( CH455_SYSOFF | CH455_BIT_ENABLE )	// 开启显示、键盘
#define CH455_SLEEPOFF	CH455_SYSOFF						// 关闭睡眠
#define CH455_SLEEPON	( CH455_SYSOFF | CH455_BIT_SLEEP )	// 开启睡眠
#define CH455_7SEG_ON	( CH455_SYSON | CH455_BIT_7SEG )	// 开启七段模式
#define CH455_8SEG_ON	( CH455_SYSON | 0x00 )				// 开启八段模式
#define CH455_SYSON_4	( CH455_SYSON | CH455_BIT_INTENS4 )	// 开启显示、键盘、4级亮度
#define CH455_SYSON_8	( CH455_SYSON | CH455_BIT_INTENS8 )	// 开启显示、键盘、8级亮度

// 加载字数据命令
#define CH455_DIG0		0x1400			// 数码管位0显示,需另加8位数据
#define CH455_DIG1		0x1500			// 数码管位1显示,需另加8位数据
#define CH455_DIG2		0x1600			// 数码管位2显示,需另加8位数据
#define CH455_DIG3		0x1700			// 数码管位3显示,需另加8位数据

// 读取按键代码命令
#define CH455_GET_KEY	0x0700			// 获取按键,返回按键代码

// CH455接口定义
#define	CH455_I2C_ADDR	0x40			// CH455的地址
#define	CH455_I2C_MASK	0x3E			// CH455的高字节命令掩码

/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/
struct CH455G_Drv {
	CH455G_DevEnum dev;
	I2C_DevEnum i2cDev;
	uint32_t deviceAddr;
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
static bool Init(struct CH455G_Drv *this);
static bool ReadKey(struct CH455G_Drv *this, uint32_t *value);
static bool WriteSeg(struct CH455G_Drv *this, uint32_t value);
static bool WriteCmd(struct CH455G_Drv *this, uint32_t value);

/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
static struct CH455G_Drv g_CH455G_Dev[] = {
{
	.dev				= DEV_CH455G_1,
	.i2cDev				= DEV_I2C3,
	.deviceAddr			= CH455_I2C_ADDR,
},
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
static struct CH455G_Drv *GetDevHandle(CH455G_DevEnum dev)
{
	struct CH455G_Drv *handle = NULL;
	for (uint8_t i=0; i<ARRAY_SIZE(g_CH455G_Dev); ++i) {		
		if (dev == g_CH455G_Dev[i].dev) {
			handle = &g_CH455G_Dev[i];
			break;
		}
	}
	return handle;
}

/*
*********************************************************************************************************
* Function Name : Init
* Description	: 硬件初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool Init(struct CH455G_Drv *this)
{
	return WriteCmd(this, CH455_SYSON | CH455_BIT_INTENS1);
}

/*
*********************************************************************************************************
* Function Name : Read
* Description	: 读取数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ReadKey(struct CH455G_Drv *this, uint32_t *value)
{
	bsp_I2CStart(this->i2cDev);
	bsp_I2CSendByte(this->i2cDev, ((uint8_t)(CH455_GET_KEY >> 7) & CH455_I2C_MASK) | this->deviceAddr | I2C_RD);
	bsp_I2CWaitAck(this->i2cDev);
	*value = bsp_I2CReadByte(this->i2cDev);
	bsp_I2CStop(this->i2cDev);
	return true;
}

/*
*********************************************************************************************************
* Function Name : WritecCmd
* Description	: 写命令
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool WriteCmd(struct CH455G_Drv *this, uint32_t value)
{
	bsp_I2CStart(this->i2cDev);
	bsp_I2CSendByte(this->i2cDev, ((uint8_t)(value >> 7) & CH455_I2C_MASK) | this->deviceAddr | I2C_WR);
	bsp_I2CWaitAck(this->i2cDev);
	bsp_I2CSendByte(this->i2cDev, (uint8_t)value);
	bsp_I2CWaitAck(this->i2cDev);
	bsp_I2CStop(this->i2cDev);
	return true;
}

/*
*********************************************************************************************************
* Function Name : WriteSeg
* Description	: 写数码管数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool WriteSeg(struct CH455G_Drv *this, uint32_t value)
{
	bool ret[4] = {false};
	ret[0] = WriteCmd(this, CH455_DIG0 | ((value >> 0) & 0xFF));	
	ret[1] = WriteCmd(this, CH455_DIG1 | ((value >> 8) & 0xFF));
	ret[2] = WriteCmd(this, CH455_DIG2 | ((value >> 16) & 0xFF));
	ret[3] = WriteCmd(this, CH455_DIG3 | ((value >> 24) & 0xFF));
	return (ret[0] & ret[1] & ret[2] & ret[3]);
}

/*
*********************************************************************************************************
*                              				API
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_InitCH455G
* Description	: 初始化CH455G
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitCH455G(void)
{
	bsp_DelayMS(100);
	for (uint8_t i=0; i<ARRAY_SIZE(g_CH455G_Dev); ++i) {
		bool ret = Init(&g_CH455G_Dev[i]);
		ECHO(ECHO_DEBUG, "[BSP] CH455G %d 初始化 .......... %s", g_CH455G_Dev[i].dev+1, (ret == true) ? "OK" : "ERROR");
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_CH455G_ReadKey
* Description	: 读取CH455G键值
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_CH455G_ReadKey(CH455G_DevEnum dev, uint32_t *value)
{
	struct CH455G_Drv *this = GetDevHandle(dev);
	if (!this) {
		return false;
	}
	return ReadKey(this, value);
}

/*
*********************************************************************************************************
* Function Name : bsp_CH455G_WriteSeg
* Description	: 写入数码管到CH455G
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_CH455G_WriteSeg(CH455G_DevEnum dev, uint32_t value)
{
	struct CH455G_Drv *this = GetDevHandle(dev);
	if (!this) {
		return false;
	}
	return WriteSeg(this, value);
}

/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
