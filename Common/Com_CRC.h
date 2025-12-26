#ifndef __COM_CRC_H__
#define __COM_CRC_H__

#include <stdint.h>

/**
 * @brief 计算 CRC16 (多项式 X16+X12+X5+1, CCITT)
 * @param p_data 数据指针
 * @param len 数据长度
 * @param crc_init 初始值 (通常为 0)
 * @return 计算出的 CRC16 值
 */
uint16_t Com_CRC16_Calculate(uint8_t *p_data, uint16_t len, uint16_t crc_init);

#endif /* __COM_CRC_H__ */
