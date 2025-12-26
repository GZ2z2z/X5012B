#ifndef __COM_UTIL_H__
#define __COM_UTIL_H__
#include <stdint.h>
#include "stm32f4xx_hal.h"

void Delay_Init(void);
void Delay_us(__IO uint32_t delay);
void Delay_ms(uint16_t count);
void Delay_s(uint16_t count);

#endif /* __COM_UTIL_H__ */
