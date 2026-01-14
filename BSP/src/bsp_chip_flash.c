/*
*********************************************************************************************************
* @file    	bsp_chip_flash.c
* @author  	SY
* @version 	V1.0.0
* @date    	2018-5-16 13:03:14
* @IDE	 	Keil V4.72.10.0
* @Chip    	STM32F407VE
* @brief   	芯片内部FLASH源文件
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
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */

#define	USER_DATA_BLOCK_BASE	ADDR_FLASH_SECTOR_7							//用户数据块基地址	
									
/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/


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


/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                              				Private functions
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_InitChipFlash
* Description	: 芯片flash初始化
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/	
void bsp_InitChipFlash(void)
{	
    /* Unlock the Flash to enable the flash control register access *************/ 
	HAL_FLASH_Unlock();
	
	ECHO(DEBUG_BSP_INIT,"[BSP] 芯片内存初始化 .......... OK");
}

/**
  * @brief  Gets the sector of a given address
  * @param  None
  * @retval The sector of a given address
  */
static uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_SECTOR_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_SECTOR_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_SECTOR_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_SECTOR_10;  
  }
  else /* (Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11) */
  {
    sector = FLASH_SECTOR_11;  
  }

  return sector;
}

#if 0
/**
  * @brief  Gets sector Size
  * @param  None
  * @retval The size of a given sector
  */
static uint32_t GetSectorSize(uint32_t Sector)
{
  uint32_t sectorsize = 0x00;

  if((Sector == FLASH_SECTOR_0) || (Sector == FLASH_SECTOR_1) || (Sector == FLASH_SECTOR_2) || (Sector == FLASH_SECTOR_3))
  {
    sectorsize = 16 * 1024;
  }
  else if(Sector == FLASH_SECTOR_4)
  {
    sectorsize = 64 * 1024;
  }
  else
  {
    sectorsize = 128 * 1024;
  }  
  return sectorsize;
}
#endif

/*
*********************************************************************************************************
* Function Name : FlashProgram
* Description	: flash写数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/	
static void FlashProgram(uint32_t *pulData, uint32_t ulAddress, uint32_t ulCount)
{
	while (ulCount != 0)
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, ulAddress, *pulData);
		pulData++;
		ulAddress += 4;
		ulCount -= 4;
	}
}

/*
*********************************************************************************************************
* Function Name : FlashErase
* Description	: flash擦除
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/	
static bool FlashErase(uint32_t startAddress, uint32_t count)
{
	static FLASH_EraseInitTypeDef EraseInitStruct;
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	EraseInitStruct.Sector = GetSector(startAddress);
	EraseInitStruct.NbSectors = count;
	uint32_t SectorError = 0;
	if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
	{ 
		BSP_ErrorHandler();
		return false;
	}
	
	return true;
}

/*
*********************************************************************************************************
* Function Name : bsp_ChipReadBytes
* Description	: 读取flash数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/	
bool bsp_ChipReadBytes(uint8_t *dat, uint16_t addr, uint16_t len)
{
	for(uint16_t i=0; i<len; i++)
	{
		dat[i] = *((uint8_t *)(USER_DATA_BLOCK_BASE + addr + i));
	}
	return true;
}

#define FLASH_BLOCK_SIZE			2048
static uint8_t data_buff[FLASH_BLOCK_SIZE];		
/*
*********************************************************************************************************
* Function Name : bsp_FramWriteBytes
* Description	: 写入数据到芯片
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool bsp_ChipWriteBytes(uint8_t *dat, uint16_t addr, uint16_t len)
{
	uint16_t block_num = addr / FLASH_BLOCK_SIZE;					 
	uint16_t block_addr = addr - block_num * FLASH_BLOCK_SIZE;
	if (FLASH_BLOCK_SIZE - block_addr >= len)				//如果在同一块内的数据
	{
		bsp_ChipReadBytes(data_buff, block_num * FLASH_BLOCK_SIZE, FLASH_BLOCK_SIZE);
		for (uint16_t j=0; j<len; j++)
		{
			data_buff[block_addr+j] = *dat++; 
		}
		
		if (!FlashErase(USER_DATA_BLOCK_BASE, 1)) {
			return false;
		}
		FlashProgram((uint32_t *)data_buff, USER_DATA_BLOCK_BASE + block_num * FLASH_BLOCK_SIZE,\
			FLASH_BLOCK_SIZE);
	}
	else									//如果在两块flash内的数据
	{
		bsp_ChipReadBytes(data_buff, block_num * FLASH_BLOCK_SIZE, FLASH_BLOCK_SIZE);
		for (uint16_t j=0; j<FLASH_BLOCK_SIZE-block_addr;j++)
		{
			data_buff[block_addr+j]=*dat++; 
		}
		FlashProgram((uint32_t *)data_buff, USER_DATA_BLOCK_BASE + block_num * FLASH_BLOCK_SIZE,\
			FLASH_BLOCK_SIZE);
		
		bsp_ChipReadBytes(data_buff, (block_num+1) * FLASH_BLOCK_SIZE, FLASH_BLOCK_SIZE);
		for(uint16_t j=0;j<(len+block_addr-FLASH_BLOCK_SIZE);j++)
		{
			data_buff[j] = *dat++;
		}
		FlashProgram((uint32_t *)data_buff, USER_DATA_BLOCK_BASE + (block_num+1) * FLASH_BLOCK_SIZE,\
			FLASH_BLOCK_SIZE);
	}
	return true;
}


/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
