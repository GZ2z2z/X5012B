/*
*********************************************************************************************************
* @file    	bsp_eeprom_24xx.c
* @author  	SY
* @version 	V1.0.0
* @date    	2016-7-10 09:42:53
* @IDE	 	Keil V4.72.10.0
* @Chip    	STM32F407VE
* @brief   	AT24CXX EEPROM源文件
*********************************************************************************************************
* @attention
*	对于EEPROM来说：如果读完字后，马上写数据，则没有关系；
	如果写完字后，马上读数据，则读取数据出错。
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
/* 仅供参考 */
#if AT24C02
	#define EE_MODEL_NAME		"AT24C02"
	#define EE_DEV_ADDR			0xA0		/* 设备地址 */
	#define EE_PAGE_SIZE		8			/* 页面大小(字节) */
	#define EE_SIZE				256			/* 总容量(字节) */
	#define EE_ADDR_BYTES		1			/* 地址字节个数 */
#elif AT24C16
	#define EE_MODEL_NAME		"AT24C16"
	#define EE_DEV_ADDR			0xA0		/* 设备地址 */
	#define EE_PAGE_SIZE		16			/* 页面大小(字节) */
	#define EE_SIZE				(2*1024)	/* 总容量(字节) */
	#define EE_ADDR_BYTES		2			/* 地址字节个数 */
#elif AT24C64
	#define EE_MODEL_NAME		"AT24C64"
	#define EE_DEV_ADDR			0xA0		/* 设备地址 */
	#define EE_PAGE_SIZE		32			/* 页面大小(字节) */
	#define EE_SIZE				(8*1024)	/* 总容量(字节) */
	#define EE_ADDR_BYTES		2			/* 地址字节个数 */
#elif AT24C128
	#define EE_MODEL_NAME		"AT24C128"
	#define EE_DEV_ADDR			0xA0		/* 设备地址 */
	#define EE_PAGE_SIZE		64			/* 页面大小(字节) */
	#define EE_SIZE				(16*1024)	/* 总容量(字节) */
	#define EE_ADDR_BYTES		2			/* 地址字节个数 */
#elif AT24C512
	#define EE_MODEL_NAME		"AT24C512"
	#define EE_DEV_ADDR			0xA0		/* 设备地址 */
	#define EE_PAGE_SIZE		128			/* 页面大小(字节) */
	#define EE_SIZE				(64*1024)	/* 总容量(字节) */
	#define EE_ADDR_BYTES		2			/* 地址字节个数 */
#endif

/* 设备定义 */
#define EEPROM1_24XX_EN						(true)
#if (EEPROM1_24XX_EN)
	#define EE1_I2C_DEV						DEV_I2C1
	#define EE1_MODEL_NAME					"AT24C512"
	#define EE1_DEV_ADDR					0xA0
	#define EE1_PAGE_SIZE					(128)
	#define EE1_TOTAL_SIZE					(64*1024)
	#define EE1_ADDR_BYTE_NUMS				(2)
#endif

#define EEPROM2_24XX_EN						(true)
#if (EEPROM2_24XX_EN)
	#define EE2_I2C_DEV						DEV_I2C2
	#define EE2_MODEL_NAME					"AT24C64"
	#define EE2_DEV_ADDR					0xA0
	#define EE2_PAGE_SIZE					(32)
	#define EE2_TOTAL_SIZE					(8*1024)
	#define EE2_ADDR_BYTE_NUMS				(2)
#endif

#define EEPROM3_24XX_EN						(false)
#if (EEPROM3_24XX_EN)
	#define EE3_I2C_DEV						DEV_I2C3
	#define EE3_MODEL_NAME					"AT24C512"
	#define EE3_DEV_ADDR					0xA0
	#define EE3_PAGE_SIZE					(128)
	#define EE3_TOTAL_SIZE					(64*1024)
	#define EE3_ADDR_BYTE_NUMS				(2)
#endif

#define EEPROM4_24XX_EN						(false)
#if (EEPROM4_24XX_EN)
	#define EE4_I2C_DEV						DEV_I2C4
	#define EE4_MODEL_NAME					"AT24C512"
	#define EE4_DEV_ADDR					0xA0
	#define EE4_PAGE_SIZE					(128)
	#define EE4_TOTAL_SIZE					(64*1024)
	#define EE4_ADDR_BYTE_NUMS				(2)
#endif

#define EEPROM5_24XX_EN						(false)
#if (EEPROM5_24XX_EN)
	#define EE5_I2C_DEV						DEV_I2C5
	#define EE5_MODEL_NAME					"AT24C512"
	#define EE5_DEV_ADDR					0xA0
	#define EE5_PAGE_SIZE					(128)
	#define EE5_TOTAL_SIZE					(64*1024)
	#define EE5_ADDR_BYTE_NUMS				(2)
#endif

#define EEPROM6_24XX_EN						(false)
#if (EEPROM6_24XX_EN)
	#define EE6_I2C_DEV						DEV_I2C6
	#define EE6_MODEL_NAME					"AT24C512"
	#define EE6_DEV_ADDR					0xA0
	#define EE6_PAGE_SIZE					(128)
	#define EE6_TOTAL_SIZE					(64*1024)
	#define EE6_ADDR_BYTE_NUMS				(2)
#endif

#define EEPROM7_24XX_EN						(false)
#if (EEPROM7_24XX_EN)
	#define EE7_I2C_DEV						DEV_I2C7
	#define EE7_MODEL_NAME					"AT24C512"
	#define EE7_DEV_ADDR					0xA0
	#define EE7_PAGE_SIZE					(128)
	#define EE7_TOTAL_SIZE					(64*1024)
	#define EE7_ADDR_BYTE_NUMS				(2)
#endif

#define TIME_DELAY_MS						(10)

/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/
struct WriteWait {
	bool flagWrite;
	uint32_t time;	
};

struct EEPROM_24XXDrv {
	EEPROM_24XX_DevEnum dev;		
	I2C_DevEnum i2cDev;
	char *deviceName;
	uint8_t deviceAddr;
	uint32_t pageSize;
	uint32_t totalSize;
	uint8_t addrBytesNums;
	struct WriteWait writeWait;
	
	bool (*init)(struct EEPROM_24XXDrv *this);
	bool (*exist)(struct EEPROM_24XXDrv *this);
	bool (*read)(struct EEPROM_24XXDrv *this, uint8_t *_pReadBuf, uint16_t _usAddress, uint16_t _usSize);
	bool (*write)(struct EEPROM_24XXDrv *this, uint8_t *_pWriteBuf, uint16_t _usAddress, uint16_t _usSize);
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
static bool __Init(struct EEPROM_24XXDrv *this);
static bool __Exist(struct EEPROM_24XXDrv *this);
static bool EEP_Read(struct EEPROM_24XXDrv *this, uint8_t *_pReadBuf, \
	uint16_t _usAddress, uint16_t _usSize);
static bool EEP_Write(struct EEPROM_24XXDrv *this, uint8_t *_pWriteBuf, \
	uint16_t _usAddress, uint16_t _usSize);
#if 0	
static bool FM24_Read(struct EEPROM_24XXDrv *this, uint8_t *_pReadBuf, \
	uint16_t _usAddress, uint16_t _usSize);
static bool FM24_Write(struct EEPROM_24XXDrv *this, uint8_t *_pWriteBuf, \
	uint16_t _usAddress, uint16_t _usSize);
#endif	
/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
static struct EEPROM_24XXDrv g_EEPROM_24XXDev[] = {
#if (EEPROM1_24XX_EN)
{
	.dev					= DEV_EEPROM1_24XX,
	.i2cDev					= EE1_I2C_DEV,
	.deviceName				= EE1_MODEL_NAME,
	.deviceAddr				= EE1_DEV_ADDR,
	.pageSize				= EE1_PAGE_SIZE,
	.totalSize				= EE1_TOTAL_SIZE,
	.addrBytesNums			= EE1_ADDR_BYTE_NUMS,
	
	.init					= __Init,
	.exist					= __Exist,
	.read					= EEP_Read,
	.write					= EEP_Write,
},
#endif

#if (EEPROM2_24XX_EN)
{
	.dev					= DEV_EEPROM2_24XX,
	.i2cDev					= EE2_I2C_DEV,
	.deviceName				= EE2_MODEL_NAME,
	.deviceAddr				= EE2_DEV_ADDR,
	.pageSize				= EE2_PAGE_SIZE,
	.totalSize				= EE2_TOTAL_SIZE,
	.addrBytesNums			= EE2_ADDR_BYTE_NUMS,
	
	.init					= __Init,
	.exist					= __Exist,
	.read					= EEP_Read,
	.write					= EEP_Write,
},
#endif
	
#if (EEPROM3_24XX_EN)
{
	.dev					= DEV_EEPROM3_24XX,
	.i2cDev					= EE3_I2C_DEV,
	.deviceName				= EE3_MODEL_NAME,
	.deviceAddr				= EE3_DEV_ADDR,
	.pageSize				= EE3_PAGE_SIZE,
	.totalSize				= EE3_TOTAL_SIZE,
	.addrBytesNums			= EE3_ADDR_BYTE_NUMS,
	
	.init					= __Init,
	.exist					= __Exist,
	.read					= EEP_Read,
	.write					= EEP_Write,
},
#endif

#if (EEPROM4_24XX_EN)
{
	.dev					= DEV_EEPROM4_24XX,
	.i2cDev					= EE4_I2C_DEV,
	.deviceName				= EE4_MODEL_NAME,
	.deviceAddr				= EE4_DEV_ADDR,
	.pageSize				= EE4_PAGE_SIZE,
	.totalSize				= EE4_TOTAL_SIZE,
	.addrBytesNums			= EE4_ADDR_BYTE_NUMS,
	
	.init					= __Init,
	.exist					= __Exist,
	.read					= EEP_Read,
	.write					= EEP_Write,
},
#endif

#if (EEPROM5_24XX_EN)
{
	.dev					= DEV_EEPROM5_24XX,
	.i2cDev					= EE5_I2C_DEV,
	.deviceName				= EE5_MODEL_NAME,
	.deviceAddr				= EE5_DEV_ADDR,
	.pageSize				= EE5_PAGE_SIZE,
	.totalSize				= EE5_TOTAL_SIZE,
	.addrBytesNums			= EE5_ADDR_BYTE_NUMS,
	
	.init					= __Init,
	.exist					= __Exist,
	.read					= EEP_Read,
	.write					= EEP_Write,
},
#endif

#if (EEPROM6_24XX_EN)
{
	.dev					= DEV_EEPROM6_24XX,
	.i2cDev					= EE6_I2C_DEV,
	.deviceName				= EE6_MODEL_NAME,
	.deviceAddr				= EE6_DEV_ADDR,
	.pageSize				= EE6_PAGE_SIZE,
	.totalSize				= EE6_TOTAL_SIZE,
	.addrBytesNums			= EE6_ADDR_BYTE_NUMS,
	
	.init					= __Init,
	.exist					= __Exist,
	.read					= EEP_Read,
	.write					= EEP_Write,
},
#endif

#if (EEPROM7_24XX_EN)
{
	.dev					= DEV_EEPROM7_24XX,
	.i2cDev					= EE7_I2C_DEV,
	.deviceName				= EE7_MODEL_NAME,
	.deviceAddr				= EE7_DEV_ADDR,
	.pageSize				= EE7_PAGE_SIZE,
	.totalSize				= EE7_TOTAL_SIZE,
	.addrBytesNums			= EE7_ADDR_BYTE_NUMS,
	
	.init					= __Init,
	.exist					= __Exist,
	.read					= EEP_Read,
	.write					= EEP_Write,
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
static struct EEPROM_24XXDrv *GetDevHandle(EEPROM_24XX_DevEnum dev)
{
	struct EEPROM_24XXDrv *handle = NULL;	
	for (uint8_t i=0; i<ARRAY_SIZE(g_EEPROM_24XXDev); ++i) {		
		if (dev == g_EEPROM_24XXDev[i].dev) {
			handle = &g_EEPROM_24XXDev[i];
			break;
		}
	}	
	return handle;
}

/*
*********************************************************************************************************
* Function Name : __Init
* Description	: 初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool __Init(struct EEPROM_24XXDrv *this)
{
	return __Exist(this);
}

/*
*********************************************************************************************************
* Function Name : __Exist
* Description	: 设备是否存在
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool __Exist(struct EEPROM_24XXDrv *this)
{
	if (bsp_I2CCheckDevice(this->i2cDev, this->deviceAddr))
	{
		return true;
	}
	else
	{
		/* 失败后，切记发送I2C总线停止信号 */
		bsp_I2CStop(this->i2cDev);
		return false;
	}
}

#if 0
/*
*********************************************************************************************************
* Function Name : FM24_Read
* Description	: 铁电存储器读取数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool FM24_Read(struct EEPROM_24XXDrv *this, uint8_t *_pReadBuf, \
	uint16_t _usAddress, uint16_t _usSize)
{
	if (_usSize < 1) {
		return false;
	}
	bsp_I2CStart(this->i2cDev);										//启动I2C
	bsp_I2CSendByte(this->i2cDev, (this->deviceAddr | I2C_WR) | ((_usAddress>>7)&0x0E));	//发送器件地址
	bsp_I2CWaitAck(this->i2cDev);
	bsp_I2CSendByte(this->i2cDev, (uint8_t)((_usAddress)&0xff));
	bsp_I2CWaitAck(this->i2cDev);        
	bsp_I2CStart(this->i2cDev);
	bsp_I2CSendByte(this->i2cDev, (this->deviceAddr | I2C_RD) | ((_usAddress>>7)&0x0E));
	bsp_I2CWaitAck(this->i2cDev);	
	for(uint16_t i=0;i<_usSize-1;i++) 			//读取字节数据
	{
		*(_pReadBuf+i)=bsp_I2CReadByte(this->i2cDev);	//读取数据
		bsp_I2CAck(this->i2cDev);
	}
	*(_pReadBuf+_usSize-1)=bsp_I2CReadByte(this->i2cDev);
	bsp_I2CnAck(this->i2cDev);
	bsp_I2CStop(this->i2cDev);
	return true;
}

/*
*********************************************************************************************************
* Function Name : FM24_Write
* Description	: 铁电存储器写入数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool FM24_Write(struct EEPROM_24XXDrv *this, uint8_t *_pWriteBuf, \
	uint16_t _usAddress, uint16_t _usSize)
{
	bsp_I2CStart(this->i2cDev);								
	bsp_I2CSendByte(this->i2cDev, (this->deviceAddr | I2C_WR) | ((_usAddress>>7)&0x0E));
	bsp_I2CWaitAck(this->i2cDev);
	bsp_I2CSendByte(this->i2cDev, (uint8_t)((_usAddress)&0xff));
	bsp_I2CWaitAck(this->i2cDev);
	for (uint16_t i=0;i<_usSize;i++)				
	{
		bsp_I2CSendByte(this->i2cDev, *(_pWriteBuf+i));
		bsp_I2CWaitAck(this->i2cDev);              
	}
	bsp_I2CStop(this->i2cDev);
	return true;
}
#endif

/*
*********************************************************************************************************
* Function Name : EEP_Read
* Description	: 读取数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool EEP_Read(struct EEPROM_24XXDrv *this, uint8_t *_pReadBuf, \
	uint16_t _usAddress, uint16_t _usSize)
{
	uint16_t i;

	/* 采用串行EEPROM随即读取指令序列，连续读取若干字节 */

	/* 第1步：发起I2C总线启动信号 */
	bsp_I2CStart(this->i2cDev);

	/* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
	bsp_I2CSendByte(this->i2cDev, this->deviceAddr | I2C_WR);	/* 此处是写指令 */

	/* 第3步：发送ACK */
	if (!bsp_I2CWaitAck(this->i2cDev))
	{
		goto cmd_fail;	/* EEPROM器件无应答 */
	}

	/* 第4步：发送字节地址，24C02只有256字节，因此1个字节就够了，如果是24C04以上，那么此处需要连发多个地址 */
	if (this->addrBytesNums == 1)
	{
		bsp_I2CSendByte(this->i2cDev, (uint8_t)_usAddress);
		if (!bsp_I2CWaitAck(this->i2cDev))
		{
			goto cmd_fail;	/* EEPROM器件无应答 */
		}
	}
	else
	{
		bsp_I2CSendByte(this->i2cDev, _usAddress >> 8);
		if (!bsp_I2CWaitAck(this->i2cDev))
		{
			goto cmd_fail;	/* EEPROM器件无应答 */
		}

		bsp_I2CSendByte(this->i2cDev, _usAddress);
		if (!bsp_I2CWaitAck(this->i2cDev))
		{
			goto cmd_fail;	/* EEPROM器件无应答 */
		}
	}

	/* 第6步：重新启动I2C总线。下面开始读取数据 */
	bsp_I2CStart(this->i2cDev);

	/* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
	bsp_I2CSendByte(this->i2cDev, this->deviceAddr | I2C_RD);	/* 此处是读指令 */

	/* 第8步：发送ACK */
	if (!bsp_I2CWaitAck(this->i2cDev))
	{
		goto cmd_fail;	/* EEPROM器件无应答 */
	}

	/* 第9步：循环读取数据 */
	for (i = 0; i < _usSize; i++)
	{
		_pReadBuf[i] = bsp_I2CReadByte(this->i2cDev);	/* 读1个字节 */

		/* 每读完1个字节后，需要发送Ack， 最后一个字节不需要Ack，发Nack */
		if (i != _usSize - 1)
		{
			bsp_I2CAck(this->i2cDev);	/* 中间字节读完后，CPU产生ACK信号(驱动SDA = 0) */
		}
		else
		{
			bsp_I2CnAck(this->i2cDev);	/* 最后1个字节读完后，CPU产生NACK信号(驱动SDA = 1) */
		}
	}
	/* 发送I2C总线停止信号 */
	bsp_I2CStop(this->i2cDev);
	return true;	/* 执行成功 */

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
	/* 发送I2C总线停止信号 */
	bsp_I2CStop(this->i2cDev);
	return false;
}

/*
*********************************************************************************************************
* Function Name : EEP_Write
* Description	: 写入数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool EEP_Write(struct EEPROM_24XXDrv *this, uint8_t *_pWriteBuf, \
	uint16_t _usAddress, uint16_t _usSize)
{
	uint16_t i,m;
	uint16_t usAddr;

	/*
		写串行EEPROM不像读操作可以连续读取很多字节，每次写操作只能在同一个page。
		对于24xx02，page size = 8
		简单的处理方法为：按字节写操作模式，每写1个字节，都发送地址
		为了提高连续写的效率: 本函数采用page wirte操作。
	*/

	usAddr = _usAddress;
	for (i = 0; i < _usSize; i++)
	{
		/* 当发送第1个字节或是页面首地址时，需要重新发起启动信号和地址 */
		if ((i == 0) || (usAddr & (this->pageSize - 1)) == 0)
		{
			/*　第０步：发停止信号，启动内部写操作　*/
			bsp_I2CStop(this->i2cDev);

			/* 通过检查器件应答的方式，判断内部写操作是否完成, 一般小于 10ms
				CLK频率为200KHz时，查询次数为30次左右
			*/
			for (m = 0; m < 1000; m++)
			{
				/* 第1步：发起I2C总线启动信号 */
				bsp_I2CStart(this->i2cDev);

				/* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
				bsp_I2CSendByte(this->i2cDev, this->deviceAddr | I2C_WR);	/* 此处是写指令 */

				/* 第3步：发送一个时钟，判断器件是否正确应答 */
				if (bsp_I2CWaitAck(this->i2cDev))
				{
					break;
				}
			}
			if (m  == 1000)
			{
				goto cmd_fail;	/* EEPROM器件写超时 */
			}

			/* 第4步：发送字节地址，24C02只有256字节，因此1个字节就够了，如果是24C04以上，那么此处需要连发多个地址 */
			if (this->addrBytesNums == 1)
			{
				bsp_I2CSendByte(this->i2cDev, (uint8_t)usAddr);
				if (!bsp_I2CWaitAck(this->i2cDev))
				{
					goto cmd_fail;	/* EEPROM器件无应答 */
				}
			}
			else
			{
				bsp_I2CSendByte(this->i2cDev, usAddr >> 8);
				if (!bsp_I2CWaitAck(this->i2cDev))
				{
					goto cmd_fail;	/* EEPROM器件无应答 */
				}

				bsp_I2CSendByte(this->i2cDev, usAddr);
				if (!bsp_I2CWaitAck(this->i2cDev))
				{
					goto cmd_fail;	/* EEPROM器件无应答 */
				}
			}
		}

		/* 第6步：开始写入数据 */
		bsp_I2CSendByte(this->i2cDev, _pWriteBuf[i]);

		/* 第7步：发送ACK */
		if (!bsp_I2CWaitAck(this->i2cDev))
		{
			goto cmd_fail;	/* EEPROM器件无应答 */
		}

		usAddr++;	/* 地址增1 */		
	}

	/* 命令执行成功，发送I2C总线停止信号 */
	bsp_I2CStop(this->i2cDev);
	return true;

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
	/* 发送I2C总线停止信号 */
	bsp_I2CStop(this->i2cDev);
	return false;
}

/*
*********************************************************************************************************
*                              				API
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_InitEeprom24xx
* Description	: 初始化24CXX
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitEeprom24xx(void)
{
	for (uint8_t i=0; i<ARRAY_SIZE(g_EEPROM_24XXDev); ++i) {
		if (g_EEPROM_24XXDev[i].init) {			
			bool ret = g_EEPROM_24XXDev[i].init(&g_EEPROM_24XXDev[i]);
			ECHO(ECHO_DEBUG, "[BSP] EEPROM/24CXX %d 初始化 .......... %s", g_EEPROM_24XXDev[i].dev+1, (ret == true) ? "OK" : "ERROR");
		}
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_InitEeprom24xx
* Description	: 初始化24CXX
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_EEP_Exist(EEPROM_24XX_DevEnum dev)
{
	struct EEPROM_24XXDrv *this = GetDevHandle(dev);
	if (this == NULL) {
		return false;
	}
	
	bool ret = false;
	if (this->exist) {
		ret = this->exist(this);
	}
	return ret;
}

/*
*********************************************************************************************************
* Function Name : WriteSync
* Description	: 写同步
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void WriteSync(struct EEPROM_24XXDrv *this)
{
	if (this->writeWait.flagWrite) {
		this->writeWait.flagWrite = false;
		uint32_t time = bsp_GetRunTime();
		uint32_t timeDiff = time - this->writeWait.time;
		if (timeDiff < TIME_DELAY_MS) {
			bsp_DelayMS(TIME_DELAY_MS - timeDiff);
			ECHO(ECHO_DEBUG, "[BSP] %s EEPROM[%d] delay %d ms", __FUNCTION__, this->dev+1, TIME_DELAY_MS - timeDiff);			
		}
	}	
}

/*
*********************************************************************************************************
* Function Name : bsp_EEP_ReadBytes
* Description	: 读取字节
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_EEP_ReadBytes(EEPROM_24XX_DevEnum dev, uint8_t *_pReadBuf, uint16_t _usAddress,\
	uint16_t _usSize)
{
	struct EEPROM_24XXDrv *this = GetDevHandle(dev);
	if (this == NULL) {
		return false;
	}
	
	WriteSync(this);
	
	bool ret = false;
	if (this->read) {
		ret = this->read(this, _pReadBuf, _usAddress, _usSize);
	}
	return ret;
}

static uint8_t g_Buffer[1024];
/*
*********************************************************************************************************
* Function Name : bsp_EEP_WriteBytes
* Description	: 写入字节
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_EEP_WriteBytes(EEPROM_24XX_DevEnum dev, uint8_t *_pWriteBuf, uint16_t _usAddress,\
	uint16_t _usSize)
{
	struct EEPROM_24XXDrv *this = GetDevHandle(dev);
	if (this == NULL) {
		return false;
	}

	//Data compare
	if (_usSize <= sizeof(g_Buffer)) {	
		if (bsp_EEP_ReadBytes(dev, g_Buffer, _usAddress, _usSize)) {
			bool durty = false;
			if (memcmp(g_Buffer, _pWriteBuf, _usSize)) {
				durty = true;
				ECHO(ECHO_DEBUG, "[BSP] %s EEPROM[%d] data duty.", __FUNCTION__, this->dev+1);
			}
			if (!durty) {
				return true;
			}
		}
	}
	
	bool ret = false;
	if (this->write) {
		ret = this->write(this, _pWriteBuf, _usAddress, _usSize);
		this->writeWait.flagWrite = true;
		this->writeWait.time = bsp_GetRunTime();
	}
	return ret;
}

/*
*********************************************************************************************************
* Function Name : bsp_EEP_WaitWriteDone
* Description	: 等待写入完成
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_EEP_WaitWriteDone(EEPROM_24XX_DevEnum dev)
{
	struct EEPROM_24XXDrv *this = GetDevHandle(dev);
	if (this == NULL) {
		return;
	}	
	WriteSync(this);
}


/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
