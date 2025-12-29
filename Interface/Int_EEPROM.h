#ifndef __INT_EEPROM_H
#define __INT_EEPROM_H

#include "main.h"
#include "Dri_I2C.h"
#include "FreeRTOS.h"
#include "task.h"

//#define AT24C02
#define AT24C64
//#define AT24C128

#ifdef AT24C02
	#define EE_MODEL_NAME		"AT24C02"
	#define EE_DEV_ADDR			0xA0		/* 设备地址 */
	#define EE_PAGE_SIZE		8			/* 页面大小(字节) */
	#define EE_SIZE				256			/* 总容量(字节) */
	#define EE_ADDR_BYTES		1			/* 地址字节个数 */
#endif

#ifdef AT24C64
	#define EE_MODEL_NAME		"AT24C64"
	#define EE_DEV_ADDR			0xA0		/* 设备地址 */
	#define EE_PAGE_SIZE		32			/* 页面大小(字节) */
	#define EE_SIZE				(8*1024)	/* 总容量(字节) */
	#define EE_ADDR_BYTES		2			/* 地址字节个数 */
#endif

#ifdef AT24C128
	#define EE_MODEL_NAME		"AT24C128"
	#define EE_DEV_ADDR			0xA0		/* 设备地址 */
	#define EE_PAGE_SIZE		64			/* 页面大小(字节) */
	#define EE_SIZE				(16*1024)	/* 总容量(字节) */
	#define EE_ADDR_BYTES		2			/* 地址字节个数 */
#endif

uint8_t ee_CheckOk(void);
uint8_t ee_ReadBytes(uint8_t *_pReadBuf, uint16_t _usAddress, uint16_t _usSize);
uint8_t ee_WriteBytes(uint8_t *_pWriteBuf, uint16_t _usAddress, uint16_t _usSize);
void Int_EEPROM_Init(void);
uint8_t Int_EEPROM_ReadBuffer(uint16_t addr, uint8_t *buffer, uint16_t length);
uint8_t Int_EEPROM_WriteBuffer(uint16_t addr, uint8_t *buffer, uint16_t length);
#endif
