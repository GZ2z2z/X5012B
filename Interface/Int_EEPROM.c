#include "Int_EEPROM.h"


/*
*********************************************************************************************************
*	函 数 名: ee_CheckOk
*	功能说明: 判断串行EERPOM是否正常
*	形    参:  无
*	返 回 值: 1 表示正常， 0 表示不正常
*********************************************************************************************************
*/
uint8_t ee_CheckOk(void)
{
	if (i2c_CheckDevice(EE_DEV_ADDR) == 0)
	{
		return 1;
	}
	else
	{
		/* 失败后，切记发送I2C总线停止信号 */
		i2c_Stop();
		return 0;
	}
}


/*
*********************************************************************************************************
*	函 数 名: ee_ReadBytes
*	功能说明: 从串行EEPROM指定地址处开始读取若干数据
*	形    参:  _usAddress : 起始地址
*			 _usSize : 数据长度，单位为字节
*			 _pReadBuf : 存放读到的数据的缓冲区指针
*	返 回 值: 0 表示失败，1表示成功
*********************************************************************************************************
*/
uint8_t ee_ReadBytes(uint8_t *_pReadBuf, uint16_t _usAddress, uint16_t _usSize)
{
	uint16_t i;

	/* 采用串行EEPROM随即读取指令序列，连续读取若干字节 */

	/* 第1步：发起I2C总线启动信号 */
	i2c_Start();

	/* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
	i2c_SendByte(EE_DEV_ADDR | I2C_WR);	/* 此处是写指令 */

	/* 第3步：发送ACK */
	if (i2c_WaitAck() != 0)
	{
		goto cmd_fail;	/* EEPROM器件无应答 */
	}

	/* 第4步：发送字节地址，24C02只有256字节，因此1个字节就够了，如果是24C04以上，那么此处需要连发多个地址 */
	if (EE_ADDR_BYTES == 1)
	{
		i2c_SendByte((uint8_t)_usAddress);
		if (i2c_WaitAck() != 0)
		{
			goto cmd_fail;	/* EEPROM器件无应答 */
		}
	}
	else
	{
		i2c_SendByte(_usAddress >> 8);
		if (i2c_WaitAck() != 0)
		{
			goto cmd_fail;	/* EEPROM器件无应答 */
		}

		i2c_SendByte(_usAddress);
		if (i2c_WaitAck() != 0)
		{
			goto cmd_fail;	/* EEPROM器件无应答 */
		}
	}

	/* 第6步：重新启动I2C总线。下面开始读取数据 */
	i2c_Start();

	/* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
	i2c_SendByte(EE_DEV_ADDR | I2C_RD);	/* 此处是读指令 */

	/* 第8步：发送ACK */
	if (i2c_WaitAck() != 0)
	{
		goto cmd_fail;	/* EEPROM器件无应答 */
	}

	/* 第9步：循环读取数据 */
	for (i = 0; i < _usSize; i++)
	{
		_pReadBuf[i] = i2c_ReadByte();	/* 读1个字节 */

		/* 每读完1个字节后，需要发送Ack， 最后一个字节不需要Ack，发Nack */
		if (i != _usSize - 1)
		{
			i2c_Ack();	/* 中间字节读完后，CPU产生ACK信号(驱动SDA = 0) */
		}
		else
		{
			i2c_NAck();	/* 最后1个字节读完后，CPU产生NACK信号(驱动SDA = 1) */
		}
	}
	/* 发送I2C总线停止信号 */
	i2c_Stop();
	return 1;	/* 执行成功 */

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
	/* 发送I2C总线停止信号 */
	i2c_Stop();
	return 0;
}

/*
*********************************************************************************************************
*	函 数 名: ee_WriteBytes
*	功能说明: 向串行EEPROM指定地址写入若干数据，采用页写操作提高写入效率
*	形    参:  _usAddress : 起始地址
*			 _usSize : 数据长度，单位为字节
*			 _pWriteBuf : 存放读到的数据的缓冲区指针
*	返 回 值: 0 表示失败，1表示成功
*********************************************************************************************************
*/
uint8_t ee_WriteBytes(uint8_t *_pWriteBuf, uint16_t _usAddress, uint16_t _usSize)
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
		if ((i == 0) || (usAddr & (EE_PAGE_SIZE - 1)) == 0)
		{
			/*　第０步：发停止信号，启动内部写操作　*/
			i2c_Stop();

			/* 通过检查器件应答的方式，判断内部写操作是否完成, 一般小于 10ms
				CLK频率为200KHz时，查询次数为30次左右
			*/
			for (m = 0; m < 1000; m++)
			{
				/* 第1步：发起I2C总线启动信号 */
				i2c_Start();

				/* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
				i2c_SendByte(EE_DEV_ADDR | I2C_WR);	/* 此处是写指令 */

				/* 第3步：发送一个时钟，判断器件是否正确应答 */
				if (i2c_WaitAck() == 0)
				{
					break;
				}
			}
			if (m  == 1000)
			{
				goto cmd_fail;	/* EEPROM器件写超时 */
			}

			/* 第4步：发送字节地址，24C02只有256字节，因此1个字节就够了，如果是24C04以上，那么此处需要连发多个地址 */
			if (EE_ADDR_BYTES == 1)
			{
				i2c_SendByte((uint8_t)usAddr);
				if (i2c_WaitAck() != 0)
				{
					goto cmd_fail;	/* EEPROM器件无应答 */
				}
			}
			else
			{
				i2c_SendByte(usAddr >> 8);
				if (i2c_WaitAck() != 0)
				{
					goto cmd_fail;	/* EEPROM器件无应答 */
				}

				i2c_SendByte(usAddr);
				if (i2c_WaitAck() != 0)
				{
					goto cmd_fail;	/* EEPROM器件无应答 */
				}
			}
		}

		/* 第6步：开始写入数据 */
		i2c_SendByte(_pWriteBuf[i]);

		/* 第7步：发送ACK */
		if (i2c_WaitAck() != 0)
		{
			goto cmd_fail;	/* EEPROM器件无应答 */
		}

		usAddr++;	/* 地址增1 */
	}

	/* 命令执行成功，发送I2C总线停止信号 */
	i2c_Stop();
	return 1;

cmd_fail: /* 命令执行失败后，切记发送停止信号，避免影响I2C总线上其他设备 */
	/* 发送I2C总线停止信号 */
	i2c_Stop();
	return 0;
}

// 初始化 wrapper
void Int_EEPROM_Init(void)
{
    bsp_InitI2C(); // 调用底层的 GPIO 初始化
    // 可以在这里加一个 check 检查设备是否存在
    if(ee_CheckOk() == 0) {
        // Log Error: EEPROM missing
    }
}

// 读封装 (HAL_OK = 0x00)
uint8_t Int_EEPROM_ReadBuffer(uint16_t addr, uint8_t *buffer, uint16_t length)
{
    if(ee_ReadBytes(buffer, addr, length) == 1) {
        return 0; // 0 == HAL_OK
    }
    return 1; // Error
}

// 写封装
uint8_t Int_EEPROM_WriteBuffer(uint16_t addr, uint8_t *buffer, uint16_t length)
{
    if(ee_WriteBytes(buffer, addr, length) == 1) {
        return 0; // 0 == HAL_OK
    }
    return 1; // Error
}


void EEPROM_Wait_WriteCycle(void)
{
    // 你的驱动里 i2c_CheckDevice 返回 0 表示检测到设备(ACK)，返回 1 表示忙/无设备
    // 这里设置一个超时防止死循环
    uint32_t timeout = 0xFFFF;
    while(i2c_CheckDevice(EE_DEV_ADDR) != 0 && timeout--);
}

// 跨页安全写入函数
void Int_EEPROM_WriteSafe(uint16_t WriteAddr, uint8_t *pBuffer, uint16_t NumByteToWrite)
{
    uint8_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;

    // 计算当前页还剩多少字节可以写
    // 例如：地址是 0x0102，页大小32，则 (0x0102 % 32) = 2，还剩 30 字节
    Addr = WriteAddr % EE_PAGE_SIZE;
    count = EE_PAGE_SIZE - Addr;
    
    // 如果要写的数据量 <= 当前页剩余空间 -> 不需要跨页，直接写
    NumOfPage =  NumByteToWrite / EE_PAGE_SIZE;
    NumOfSingle = NumByteToWrite % EE_PAGE_SIZE;

    // 如果起始地址刚好是页对齐的 (Addr == 0)
    if (Addr == 0) 
    {
        // 满页写入逻辑...
        if (NumOfPage == 0) 
        {
            Int_EEPROM_WriteBuffer(WriteAddr, pBuffer, NumOfSingle);
            EEPROM_Wait_WriteCycle(); // 必须等待内部写入完成
        }
        else 
        {
            while (NumOfPage--)
            {
                Int_EEPROM_WriteBuffer(WriteAddr, pBuffer, EE_PAGE_SIZE);
                EEPROM_Wait_WriteCycle(); // 必须等待内部写入完成
                WriteAddr += EE_PAGE_SIZE;
                pBuffer += EE_PAGE_SIZE;
            }
            if (NumOfSingle != 0)
            {
                Int_EEPROM_WriteBuffer(WriteAddr, pBuffer, NumOfSingle);
                EEPROM_Wait_WriteCycle(); // 必须等待内部写入完成
            }
        }
    }
    // 如果起始地址不对齐 (Addr != 0)
    else 
    {
        // 情况A：数据量很小，甚至填不满当前页剩余空间
        if (NumByteToWrite < count) 
        {
            NumOfPage = 0;
            NumOfSingle = NumByteToWrite;
            
            Int_EEPROM_WriteBuffer(WriteAddr, pBuffer, NumOfSingle);
            EEPROM_Wait_WriteCycle(); // 必须等待内部写入完成
        }
        // 情况B：数据量大，先把当前页剩余填满，再进行后续操作
        else 
        {
            NumByteToWrite -= count;
            NumOfPage =  NumByteToWrite / EE_PAGE_SIZE;
            NumOfSingle = NumByteToWrite % EE_PAGE_SIZE;

            // 1. 填满当前页剩余部分
            Int_EEPROM_WriteBuffer(WriteAddr, pBuffer, count);
            EEPROM_Wait_WriteCycle(); // 必须等待内部写入完成
            
            WriteAddr += count;
            pBuffer += count;

            // 2. 写中间的满页
            while (NumOfPage--)
            {
                Int_EEPROM_WriteBuffer(WriteAddr, pBuffer, EE_PAGE_SIZE);
                EEPROM_Wait_WriteCycle(); // 必须等待内部写入完成
                WriteAddr += EE_PAGE_SIZE;
                pBuffer += EE_PAGE_SIZE;
            }

            // 3. 写最后剩下的尾巴
            if (NumOfSingle != 0)
            {
                Int_EEPROM_WriteBuffer(WriteAddr, pBuffer, NumOfSingle);
                EEPROM_Wait_WriteCycle(); // 必须等待内部写入完成
            }
        }
    }
}
