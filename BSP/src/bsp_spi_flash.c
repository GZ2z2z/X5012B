/*
*********************************************************************************************************
*
*	模块名称 : SPI接口串行FLASH 读写模块
*	文件名称 : bsp_spi_flash.c
*	版    本 : V1.0
*	说    明 : 支持 SST25VF016B、MX25L1606E、 W25Q64BVSSIG、W25Q80DVSSIG
*			   SST25VF016B 的写操作采用AAI指令，可以提高写入效率。
*
*			   缺省使用STM32F4的硬件SPI1接口，时钟频率最高为 42MHz （超频使用）
*
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2013-02-01 armfly  正式发布
*
*	Copyright (C), 2013-2014, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"

/*
*********************************************************************************************************
*                              				Private define
*********************************************************************************************************
*/		
#define SF_MAX_PAGE_SIZE			(4 * 1024)

#define CMD_AAI       				0xAD  		/* AAI 连续编程指令(FOR SST25VF016B) */
#define CMD_DISWR	 				0x04		/* 禁止写, 退出AAI状态 */
#define CMD_EWRSR	  				0x50		/* 允许写状态寄存器的命令 */
#define CMD_WRSR      				0x01  		/* 写状态寄存器命令 */
#define CMD_WREN      				0x06		/* 写使能命令 */
#define CMD_READ      				0x03  		/* 读数据区命令 */
#define CMD_RDSR      				0x05		/* 读状态寄存器命令 */
#define CMD_RDID      				0x9F		/* 读器件ID命令 */
#define CMD_SE        				0x20		/* 擦除扇区命令 */
#define CMD_BE        				0xC7		/* 批量擦除命令 */
#define DUMMY_BYTE    				0xA5		/* 哑命令，可以为任意值，用于读操作 */
#define WIP_FLAG      				0x01		/* 状态寄存器中的正在编程标志（WIP) */


#define SPI_FLASH_1_EN				(true)
#if (SPI_FLASH_1_EN)
#define SPI1_FLASH					(DEV_SPI3)
#define SF1_GPIO_CLK_ENABLE()		do {\
										__HAL_RCC_GPIOD_CLK_ENABLE();\
									} while (0)
#define	SF1_CS_GPIO_PORT			GPIOD
#define	SF1_CS_GPIO_PIN				GPIO_PIN_0
#endif

/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/
enum SPI_FLASH_Enum
{
	SST25VF016B_ID 	= 0xBF2541,
	MX25L1606E_ID  	= 0xC22015,
	W25Q64BV_ID    	= 0xEF4017,
	W25Q80DV_ID		= 0XEF4014,
};

struct SPI_FLASH_Drv {
	SPI_Flash_DevEnum dev;		
	SPI_DevEnum spiDev;
	uint32_t ChipID;		/* 芯片ID */
	char ChipName[16];		/* 芯片型号字符串，主要用于显示 */
	uint32_t TotalSize;		/* 总容量 */
	uint16_t PageSize;		/* 页面大小 */
	GPIO_TypeDef *GPIOx;
	uint16_t GPIO_Pin;
	void (*csLow)(struct SPI_FLASH_Drv *this);
	void (*csHigh)(struct SPI_FLASH_Drv *this);
};

/*
*********************************************************************************************************
*                              				Private macro
*********************************************************************************************************
*/
static void CS_High(struct SPI_FLASH_Drv *this)
{
	HAL_GPIO_WritePin(this->GPIOx, this->GPIO_Pin, GPIO_PIN_SET);
}

static void CS_Low(struct SPI_FLASH_Drv *this)
{
	HAL_GPIO_WritePin(this->GPIOx, this->GPIO_Pin, GPIO_PIN_RESET);
}

/*
*********************************************************************************************************
*                              				Private function prototypes
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
static uint8_t s_spiBuf[SF_MAX_PAGE_SIZE];	/* 用于写函数，先读出整个page，修改缓冲区后，再整个page回写 */
static struct SPI_FLASH_Drv spiFlashDev[] = {
#if (SPI_FLASH_1_EN)
{
	.dev			= DEV_SPI_FLASH_1,
	.spiDev			= SPI1_FLASH,
	.GPIOx			= SF1_CS_GPIO_PORT,
	.GPIO_Pin		= SF1_CS_GPIO_PIN,
	.csLow			= CS_Low,
	.csHigh			= CS_High,
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
* Function Name : GetPulseDevHandle
* Description	: 获取脉冲设备句柄
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static struct SPI_FLASH_Drv *GetDevHandle(SPI_Flash_DevEnum dev)
{
	struct SPI_FLASH_Drv *handle = NULL;
	for (uint8_t i=0; i<ARRAY_SIZE(spiFlashDev); ++i) {
		if (dev == spiFlashDev[i].dev) {
			handle = &spiFlashDev[i];
			break;
		}
	}
	return handle;
}	

/*
*********************************************************************************************************
*	函 数 名: sf_WriteEnable
*	功能说明: 向器件发送写使能命令
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
static void sf_WriteEnable(struct SPI_FLASH_Drv *this)
{
	if (this->csLow) {
		this->csLow(this);
	}
	SPI_WriteByte(this->spiDev, CMD_WREN);				
	if (this->csHigh) {
		this->csHigh(this);
	}
}

/*
*********************************************************************************************************
*	函 数 名: sf_WriteStatus
*	功能说明: 写状态寄存器
*	形    参:  _ucValue : 状态寄存器的值
*	返 回 值: 无
*********************************************************************************************************
*/
static void sf_WriteStatus(struct SPI_FLASH_Drv *this, uint8_t _ucValue)
{
	if (this->ChipID == SST25VF016B_ID)
	{
		/* 第1步：先使能写状态寄存器 */
		if (this->csLow) {
			this->csLow(this);
		}
		SPI_WriteByte(this->spiDev, CMD_EWRSR);							/* 发送命令， 允许写状态寄存器 */
		if (this->csHigh) {
			this->csHigh(this);
		}

		/* 第2步：再写状态寄存器 */
		if (this->csLow) {
			this->csLow(this);
		}
		SPI_WriteByte(this->spiDev, CMD_WRSR);							/* 发送命令， 写状态寄存器 */
		SPI_WriteByte(this->spiDev, _ucValue);							/* 发送数据：状态寄存器的值 */
		if (this->csHigh) {
			this->csHigh(this);
		}
	}
	else
	{
		if (this->csLow) {
			this->csLow(this);
		}
		SPI_WriteByte(this->spiDev, CMD_WRSR);							/* 发送命令， 写状态寄存器 */
		SPI_WriteByte(this->spiDev, _ucValue);							/* 发送数据：状态寄存器的值 */
		if (this->csHigh) {
			this->csHigh(this);
		}
	}
}

/*
*********************************************************************************************************
*	函 数 名: sf_WaitForWriteEnd
*	功能说明: 采用循环查询的方式等待器件内部写操作完成
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
static void sf_WaitForWriteEnd(struct SPI_FLASH_Drv *this)
{
	if (this->csLow) {
		this->csLow(this);
	}
	SPI_WriteByte(this->spiDev, CMD_RDSR);							/* 发送命令， 读状态寄存器 */
	while((SPI_WriteByte1(this->spiDev, DUMMY_BYTE) & WIP_FLAG) == SET);	/* 判断状态寄存器的忙标志位 */
	if (this->csHigh) {
		this->csHigh(this);
	}
}

/*
*********************************************************************************************************
*	函 数 名: sf_EraseSector
*	功能说明: 擦除指定的扇区
*	形    参:  _uiSectorAddr : 扇区地址
*	返 回 值: 无
*********************************************************************************************************
*/
static void sf_EraseSector(struct SPI_FLASH_Drv *this, uint32_t _uiSectorAddr)
{
	sf_WriteEnable(this);								/* 发送写使能命令 */

	/* 擦除扇区操作 */
	if (this->csLow) {
		this->csLow(this);
	}
	SPI_WriteByte(this->spiDev, CMD_SE);							/* 发送擦除命令 */
	SPI_WriteByte(this->spiDev, (_uiSectorAddr & 0xFF0000) >> 16);	/* 发送扇区地址的高8bit */
	SPI_WriteByte(this->spiDev, (_uiSectorAddr & 0xFF00) >> 8);		/* 发送扇区地址中间8bit */
	SPI_WriteByte(this->spiDev, _uiSectorAddr & 0xFF);				/* 发送扇区地址低8bit */
	if (this->csHigh) {
		this->csHigh(this);
	}									

	sf_WaitForWriteEnd(this);							/* 等待串行Flash内部写操作完成 */
}
#if false
/*
*********************************************************************************************************
*	函 数 名: sf_EraseChip
*	功能说明: 擦除整个芯片
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
static void sf_EraseChip(struct SPI_FLASH_Drv *this)
{
	sf_WriteEnable(this);								/* 发送写使能命令 */

	/* 擦除扇区操作 */
	if (this->csLow) {
		this->csLow(this);
	}
	SPI_WriteByte(this->spiDev, CMD_BE);							/* 发送整片擦除命令 */
	if (this->csHigh) {
		this->csHigh(this);
	}

	sf_WaitForWriteEnd(this);							/* 等待串行Flash内部写操作完成 */
}
#endif
/*
*********************************************************************************************************
*	函 数 名: sf_PageWrite
*	功能说明: 向一个page内写入若干字节。字节个数不能超出页面大小（4K)
*	形    参:  	_pBuf : 数据源缓冲区；
*				_uiWriteAddr ：目标区域首地址
*				_usSize ：数据个数，不能超过页面大小
*	返 回 值: 无
*********************************************************************************************************
*/
static void sf_PageWrite(struct SPI_FLASH_Drv *this, uint8_t * _pBuf, uint32_t _uiWriteAddr, uint16_t _usSize)
{
	uint32_t i, j;

	if (this->ChipID == SST25VF016B_ID)
	{
		/* AAI指令要求传入的数据个数是偶数 */
		if ((_usSize < 2) && (_usSize % 2))
		{
			return ;
		}

		sf_WriteEnable(this);								/* 发送写使能命令 */

		if (this->csLow) {
			this->csLow(this);
		}
		SPI_WriteByte(this->spiDev, CMD_AAI);							/* 发送AAI命令(地址自动增加编程) */
		SPI_WriteByte(this->spiDev, (_uiWriteAddr & 0xFF0000) >> 16);	/* 发送扇区地址的高8bit */
		SPI_WriteByte(this->spiDev, (_uiWriteAddr & 0xFF00) >> 8);		/* 发送扇区地址中间8bit */
		SPI_WriteByte(this->spiDev, _uiWriteAddr & 0xFF);				/* 发送扇区地址低8bit */
		SPI_WriteByte(this->spiDev, *_pBuf++);							/* 发送第1个数据 */
		SPI_WriteByte(this->spiDev, *_pBuf++);							/* 发送第2个数据 */
		if (this->csHigh) {
			this->csHigh(this);
		}

		sf_WaitForWriteEnd(this);							/* 等待串行Flash内部写操作完成 */

		_usSize -= 2;									/* 计算剩余字节数 */

		for (i = 0; i < _usSize / 2; i++)
		{
			if (this->csLow) {
				this->csLow(this);
			}
			SPI_WriteByte(this->spiDev, CMD_AAI);						/* 发送AAI命令(地址自动增加编程) */
			SPI_WriteByte(this->spiDev, *_pBuf++);						/* 发送数据 */
			SPI_WriteByte(this->spiDev, *_pBuf++);						/* 发送数据 */
			if (this->csHigh) {
				this->csHigh(this);
			}
			sf_WaitForWriteEnd(this);						/* 等待串行Flash内部写操作完成 */
		}

		/* 进入写保护状态 */
		if (this->csLow) {
			this->csLow(this);
		}
		SPI_WriteByte(this->spiDev, CMD_DISWR);
		if (this->csHigh) {
			this->csHigh(this);
		}

		sf_WaitForWriteEnd(this);							/* 等待串行Flash内部写操作完成 */
	}
	else	/* for MX25L1606E 、 W25Q64BV */
	{
		for (j = 0; j < _usSize / 256; j++)
		{
			sf_WriteEnable(this);								/* 发送写使能命令 */

			if (this->csLow) {
				this->csLow(this);
			}
			SPI_WriteByte(this->spiDev, 0x02);								/* 发送AAI命令(地址自动增加编程) */
			SPI_WriteByte(this->spiDev, (_uiWriteAddr & 0xFF0000) >> 16);	/* 发送扇区地址的高8bit */
			SPI_WriteByte(this->spiDev, (_uiWriteAddr & 0xFF00) >> 8);		/* 发送扇区地址中间8bit */
			SPI_WriteByte(this->spiDev, _uiWriteAddr & 0xFF);				/* 发送扇区地址低8bit */

			for (i = 0; i < 256; i++)
			{
				SPI_WriteByte(this->spiDev, *_pBuf++);					/* 发送数据 */
			}
			if (this->csHigh) {
				this->csHigh(this);
			}

			sf_WaitForWriteEnd(this);						/* 等待串行Flash内部写操作完成 */

			_uiWriteAddr += 256;
		}

		/* 进入写保护状态 */
		if (this->csLow) {
			this->csLow(this);
		}
		SPI_WriteByte(this->spiDev, CMD_DISWR);
		if (this->csHigh) {
			this->csHigh(this);
		}

		sf_WaitForWriteEnd(this);							/* 等待串行Flash内部写操作完成 */
	}
}

/*
*********************************************************************************************************
*	函 数 名: sf_ReadBuffer
*	功能说明: 连续读取若干字节。字节个数不能超出芯片容量。
*	形    参:  	_pBuf : 数据源缓冲区；
*				_uiReadAddr ：首地址
*				_usSize ：数据个数, 可以大于PAGE_SIZE,但是不能超出芯片总容量
*	返 回 值: 无
*********************************************************************************************************
*/
static bool sf_ReadBuffer(struct SPI_FLASH_Drv *this, uint8_t * _pBuf, uint32_t _uiReadAddr, uint32_t _uiSize)
{
	/* 如果读取的数据长度为0或者超出串行Flash地址空间，则直接返回 */
	if ((_uiSize == 0) ||(_uiReadAddr + _uiSize) > this->TotalSize)
	{
		return false;
	}

	/* 擦除扇区操作 */
	if (this->csLow) {
		this->csLow(this);
	}
	SPI_WriteByte(this->spiDev, CMD_READ);							/* 发送读命令 */
	SPI_WriteByte(this->spiDev, (_uiReadAddr & 0xFF0000) >> 16);	/* 发送扇区地址的高8bit */
	SPI_WriteByte(this->spiDev, (_uiReadAddr & 0xFF00) >> 8);		/* 发送扇区地址中间8bit */
	SPI_WriteByte(this->spiDev, _uiReadAddr & 0xFF);				/* 发送扇区地址低8bit */
	while (_uiSize--)
	{
		*_pBuf++ = SPI_WriteByte1(this->spiDev, DUMMY_BYTE);			/* 读一个字节并存储到pBuf，读完后指针自加1 */
	}
	if (this->csHigh) {
		this->csHigh(this);
	}
	return true;
}

/*
*********************************************************************************************************
*	函 数 名: sf_CmpData
*	功能说明: 比较Flash的数据.
*	形    参:  	_ucpTar : 数据缓冲区
*				_uiSrcAddr ：Flash地址
*				_uiSize ：数据个数, 可以大于PAGE_SIZE,但是不能超出芯片总容量
*	返 回 值: 0 = 相等, 1 = 不等
*********************************************************************************************************
*/
static uint8_t sf_CmpData(struct SPI_FLASH_Drv *this, uint32_t _uiSrcAddr, uint8_t *_ucpTar, uint32_t _uiSize)
{
	uint8_t ucValue;

	/* 如果读取的数据长度为0或者超出串行Flash地址空间，则直接返回 */
	if ((_uiSrcAddr + _uiSize) > this->TotalSize)
	{
		return 1;
	}

	if (_uiSize == 0)
	{
		return 0;
	}

	if (this->csLow) {
		this->csLow(this);
	}
	SPI_WriteByte(this->spiDev, CMD_READ);							/* 发送读命令 */
	SPI_WriteByte(this->spiDev, (_uiSrcAddr & 0xFF0000) >> 16);		/* 发送扇区地址的高8bit */
	SPI_WriteByte(this->spiDev, (_uiSrcAddr & 0xFF00) >> 8);		/* 发送扇区地址中间8bit */
	SPI_WriteByte(this->spiDev, _uiSrcAddr & 0xFF);					/* 发送扇区地址低8bit */
	while (_uiSize--)
	{
		/* 读一个字节 */
		ucValue = SPI_WriteByte1(this->spiDev, DUMMY_BYTE);
		if (*_ucpTar++ != ucValue)
		{
			if (this->csHigh) {
				this->csHigh(this);
			}
			return 1;
		}
	}
	if (this->csHigh) {
		this->csHigh(this);
	}
	return 0;
}

/*
*********************************************************************************************************
*	函 数 名: sf_NeedErase
*	功能说明: 判断写PAGE前是否需要先擦除。
*	形    参:   _ucpOldBuf ： 旧数据
*			   _ucpNewBuf ： 新数据
*			   _uiLen ：数据个数，不能超过页面大小
*	返 回 值: 0 : 不需要擦除， 1 ：需要擦除
*********************************************************************************************************
*/
static uint8_t sf_NeedErase(struct SPI_FLASH_Drv *this, uint8_t * _ucpOldBuf, uint8_t *_ucpNewBuf, uint16_t _usLen)
{
	uint16_t i;
	uint8_t ucOld;

	/*
	算法第1步：old 求反, new 不变
	      old    new
		  1101   0101
	~     1111
		= 0010   0101

	算法第2步: old 求反的结果与 new 位与
		  0010   old
	&	  0101   new
		 =0000

	算法第3步: 结果为0,则表示无需擦除. 否则表示需要擦除
	*/

	for (i = 0; i < _usLen; i++)
	{
		ucOld = *_ucpOldBuf++;
		ucOld = ~ucOld;

		/* 注意错误的写法: if (ucOld & (*_ucpNewBuf++) != 0) */
		if ((ucOld & (*_ucpNewBuf++)) != 0)
		{
			return 1;
		}
	}
	return 0;
}

/*
*********************************************************************************************************
*	函 数 名: sf_AutoWritePage
*	功能说明: 写1个PAGE并校验,如果不正确则再重写两次。本函数自动完成擦除操作。
*	形    参:  	_pBuf : 数据源缓冲区；
*				_uiWriteAddr ：目标区域首地址
*				_usSize ：数据个数，不能超过页面大小
*	返 回 值: 0 : 错误， 1 ： 成功
*********************************************************************************************************
*/
static uint8_t sf_AutoWritePage(struct SPI_FLASH_Drv *this, uint8_t *_ucpSrc, uint32_t _uiWrAddr, uint16_t _usWrLen)
{
	uint16_t i;
	uint16_t j;					/* 用于延时 */
	uint32_t uiFirstAddr;		/* 扇区首址 */
	uint8_t ucNeedErase;		/* 1表示需要擦除 */
	uint8_t cRet;

	/* 长度为0时不继续操作,直接认为成功 */
	if (_usWrLen == 0)
	{
		return 1;
	}

	/* 如果偏移地址超过芯片容量则退出 */
	if (_uiWrAddr >= this->TotalSize)
	{
		return 0;
	}

	/* 如果数据长度大于扇区容量，则退出 */
	if (_usWrLen > this->PageSize)
	{
		return 0;
	}

	/* 如果FLASH中的数据没有变化,则不写FLASH */
	sf_ReadBuffer(this, s_spiBuf, _uiWrAddr, _usWrLen);
	if (memcmp(s_spiBuf, _ucpSrc, _usWrLen) == 0)
	{
		return 1;
	}

	/* 判断是否需要先擦除扇区 */
	/* 如果旧数据修改为新数据，所有位均是 1->0 或者 0->0, 则无需擦除,提高Flash寿命 */
	ucNeedErase = 0;
	if (sf_NeedErase(this, s_spiBuf, _ucpSrc, _usWrLen))
	{
		ucNeedErase = 1;
	}

	uiFirstAddr = _uiWrAddr & (~(this->PageSize - 1));

	if (_usWrLen == this->PageSize)		/* 整个扇区都改写 */
	{
		for	(i = 0; i < this->PageSize; i++)
		{
			s_spiBuf[i] = _ucpSrc[i];
		}
	}
	else						/* 改写部分数据 */
	{
		/* 先将整个扇区的数据读出 */
		sf_ReadBuffer(this, s_spiBuf, uiFirstAddr, this->PageSize);

		/* 再用新数据覆盖 */
		i = _uiWrAddr & (this->PageSize - 1);
		memcpy(&s_spiBuf[i], _ucpSrc, _usWrLen);
	}

	/* 写完之后进行校验，如果不正确则重写，最多3次 */
	cRet = 0;
	for (i = 0; i < 3; i++)
	{

		/* 如果旧数据修改为新数据，所有位均是 1->0 或者 0->0, 则无需擦除,提高Flash寿命 */
		if (ucNeedErase == 1)
		{
			sf_EraseSector(this, uiFirstAddr);		/* 擦除1个扇区 */
		
		}

		/* 编程一个PAGE */
		sf_PageWrite(this, s_spiBuf, uiFirstAddr, this->PageSize);

		if (sf_CmpData(this, _uiWrAddr, _ucpSrc, _usWrLen) == 0)
		{
			cRet = 1;
			break;
		}
		else
		{
			if (sf_CmpData(this, _uiWrAddr, _ucpSrc, _usWrLen) == 0)
			{
				cRet = 1;
				break;
			}

			/* 失败后延迟一段时间再重试 */
			for (j = 0; j < 10000; j++);
		}
	}

	return cRet;
}

/*
*********************************************************************************************************
*	函 数 名: sf_WriteBuffer
*	功能说明: 写1个扇区并校验,如果不正确则再重写两次。本函数自动完成擦除操作。
*	形    参:  	_pBuf : 数据源缓冲区；
*				_uiWrAddr ：目标区域首地址
*				_usSize ：数据个数，不能超过页面大小
*	返 回 值: 1 : 成功， 0 ： 失败
*********************************************************************************************************
*/
static bool sf_WriteBuffer(struct SPI_FLASH_Drv *this, uint8_t* _pBuf, uint32_t _uiWriteAddr, uint16_t _usWriteSize)
{
	uint16_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;

	Addr = _uiWriteAddr % this->PageSize;
	count = this->PageSize - Addr;
	NumOfPage =  _usWriteSize / this->PageSize;
	NumOfSingle = _usWriteSize % this->PageSize;

	if (Addr == 0) /* 起始地址是页面首地址  */
	{
		if (NumOfPage == 0) /* 数据长度小于页面大小 */
		{
			if (sf_AutoWritePage(this, _pBuf, _uiWriteAddr, _usWriteSize) == 0)
			{
				return false;
			}
		}
		else 	/* 数据长度大于等于页面大小 */
		{
			while (NumOfPage--)
			{
				if (sf_AutoWritePage(this, _pBuf, _uiWriteAddr, this->PageSize) == 0)
				{
					return false;
				}
				_uiWriteAddr +=  this->PageSize;
				_pBuf += this->PageSize;
			}
			if (sf_AutoWritePage(this, _pBuf, _uiWriteAddr, NumOfSingle) == 0)
			{
				return false;
			}
		}
	}
	else  /* 起始地址不是页面首地址  */
	{
		if (NumOfPage == 0) /* 数据长度小于页面大小 */
		{
			if (NumOfSingle > count) /* (_usWriteSize + _uiWriteAddr) > SPI_FLASH_PAGESIZE */
			{
				temp = NumOfSingle - count;

				if (sf_AutoWritePage(this, _pBuf, _uiWriteAddr, count) == 0)
				{
					return false;
				}

				_uiWriteAddr +=  count;
				_pBuf += count;

				if (sf_AutoWritePage(this, _pBuf, _uiWriteAddr, temp) == 0)
				{
					return false;
				}
			}
			else
			{
				if (sf_AutoWritePage(this, _pBuf, _uiWriteAddr, _usWriteSize) == 0)
				{
					return false;
				}
			}
		}
		else	/* 数据长度大于等于页面大小 */
		{
			_usWriteSize -= count;
			NumOfPage =  _usWriteSize / this->PageSize;
			NumOfSingle = _usWriteSize % this->PageSize;

			if (sf_AutoWritePage(this, _pBuf, _uiWriteAddr, count) == 0)
			{
				return false;
			}

			_uiWriteAddr +=  count;
			_pBuf += count;

			while (NumOfPage--)
			{
				if (sf_AutoWritePage(this, _pBuf, _uiWriteAddr, this->PageSize) == 0)
				{
					return false;
				}
				_uiWriteAddr +=  this->PageSize;
				_pBuf += this->PageSize;
			}

			if (NumOfSingle != 0)
			{
				if (sf_AutoWritePage(this, _pBuf, _uiWriteAddr, NumOfSingle) == 0)
				{
					return false;
				}
			}
		}
	}
	return true;	/* 成功 */
}

/*
*********************************************************************************************************
*	函 数 名: sf_ReadID
*	功能说明: 读取器件ID
*	形    参:  无
*	返 回 值: 32bit的器件ID (最高8bit填0，有效ID位数为24bit）
*********************************************************************************************************
*/
static uint32_t sf_ReadID(struct SPI_FLASH_Drv *this)
{
	uint32_t uiID;
	uint8_t id1, id2, id3;

	if (this->csLow) {
		this->csLow(this);
	}
	SPI_WriteByte1(this->spiDev, CMD_RDID);							/* 发送读ID命令 */
	id1 = SPI_WriteByte1(this->spiDev, DUMMY_BYTE);					/* 读ID的第1个字节 */
	id2 = SPI_WriteByte1(this->spiDev, DUMMY_BYTE);					/* 读ID的第2个字节 */
	id3 = SPI_WriteByte1(this->spiDev, DUMMY_BYTE);					/* 读ID的第3个字节 */
	if (this->csHigh) {
		this->csHigh(this);
	}

	uiID = ((uint32_t)id1 << 16) | ((uint32_t)id2 << 8) | id3;

	return uiID;
}

/*
*********************************************************************************************************
*	函 数 名: sf_ReadInfo
*	功能说明: 读取器件ID,并填充器件参数
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
static void sf_ReadInfo(struct SPI_FLASH_Drv *this)
{
	/* 自动识别串行Flash型号 */
	{
		this->ChipID = sf_ReadID(this);	/* 芯片ID */

		switch (this->ChipID)
		{
			case SST25VF016B_ID:
				strcpy(this->ChipName, "SST25VF016B");
				this->TotalSize = 2 * 1024 * 1024;	/* 总容量 = 2M */
				this->PageSize = 4 * 1024;			/* 页面大小 = 4K */
				break;

			case MX25L1606E_ID:
				strcpy(this->ChipName, "MX25L1606E");
				this->TotalSize = 2 * 1024 * 1024;	/* 总容量 = 2M */
				this->PageSize = 4 * 1024;			/* 页面大小 = 4K */
				break;

			case W25Q64BV_ID:
				strcpy(this->ChipName, "W25Q64BV");
				this->TotalSize = 8 * 1024 * 1024;	/* 总容量 = 8M */
				this->PageSize = 4 * 1024;			/* 页面大小 = 4K */
				break;
			
			case W25Q80DV_ID:
				strcpy(this->ChipName, "W25Q80DV");
				this->TotalSize = 1 * 1024 * 1024;	/* 总容量 = 1M */
				this->PageSize = 4 * 1024;			/* 页面大小 = 4K */
				break;

			default:
				strcpy(this->ChipName, "Unknow Flash");
				this->TotalSize = 2 * 1024 * 1024;
				this->PageSize = 4 * 1024;
				break;
		}
	}
}

/*
*********************************************************************************************************
*	函 数 名: bsp_InitSpiFlash
*	功能说明: 初始化串行Flash硬件接口（配置STM32的SPI时钟、GPIO)
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
static void sf_InitSFlash(struct SPI_FLASH_Drv *this)
{
	if (this->dev == DEV_SPI_FLASH_1) {
		SF1_GPIO_CLK_ENABLE();
		
		GPIO_InitTypeDef GPIO_InitStructure = {0}; 
		GPIO_InitStructure.Pin = this->GPIO_Pin; 
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
		GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(this->GPIOx, &GPIO_InitStructure);
	}
		
	sf_ReadInfo(this);					/* 自动识别芯片型号 */
	if (this->csLow) {
		this->csLow(this);
	}
	SPI_WriteByte(this->spiDev, CMD_DISWR);		/* 发送禁止写入的命令,即使能软件写保护 */
	if (this->csHigh) {
		this->csHigh(this);
	}
	sf_WaitForWriteEnd(this);			/* 等待串行Flash内部操作完成 */
	sf_WriteStatus(this, 0);			/* 解除所有BLOCK的写保护 */
}

/*
*********************************************************************************************************
*                              				API
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
void bsp_InitSFlash(void)
{
	for (uint32_t i=0; i<ARRAY_SIZE(spiFlashDev); ++i) {
		sf_InitSFlash(&spiFlashDev[i]);
	}
}

/*
*********************************************************************************************************
* Function Name : bsp_SFlashRead
* Description	: 读数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_SFlashRead(SPI_Flash_DevEnum dev, uint8_t *buffer, uint32_t addr, uint32_t size)
{
	struct SPI_FLASH_Drv *this= GetDevHandle(dev);
	if (this == NULL) {
		return false;
	}
	return sf_ReadBuffer(this, buffer, addr, size);
}

/*
*********************************************************************************************************
* Function Name : bsp_SFlashWrite
* Description	: 写数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_SFlashWrite(SPI_Flash_DevEnum dev, uint8_t *buffer, uint32_t addr, uint32_t size)
{
	struct SPI_FLASH_Drv *this= GetDevHandle(dev);
	if (this == NULL) {
		return false;
	}
	return sf_WriteBuffer(this, buffer, addr, size);
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
