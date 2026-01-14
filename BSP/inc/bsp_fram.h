/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BSP_FRAME_H
#define __BSP_FRAME_H

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void bsp_InitFram( void );
bool bsp_FramCheckOk(I2C_DevEnum dev);
bool bsp_FramWriteBytes(uint8_t* dat,uint16_t addr,uint16_t len);
bool bsp_FramReadBytes(uint8_t *p,uint16_t addr,uint16_t len); 

#endif

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
