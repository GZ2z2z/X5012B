#ifndef __DRI_CS5530_H__
#define __DRI_CS5530_H__

#include "main.h"
#include "Com_Debug.h"
#include "FreeRTOS.h"
#include "task.h"

// 定义ADC数量
#define CS5530_COUNT  12

/* CS5530 命令定义 */
#define CMD_CONVERT_SINGLE      0x80  // 单次转换
#define CMD_CONVERT_CONT        0xC0  // 连续转换
#define CMD_SYNC0               0xFE  // 同步0
#define CMD_SYNC1               0xFF  // 同步1
#define CMD_NULL                0x00  // 空操作

/* 寄存器定义 */
#define REG_CONFIG              0x03  // 配置寄存器
#define REG_OFFSET              0x01  // 偏移寄存器
#define REG_GAIN                0x02  // 增益寄存器
#define REG_SETUP               0x04  // 建立寄存器 


/* 函数声明 */
void CS5530_Init_All(void);
void CS5530_Start_Continuous(void);
int32_t CS5530_Read_Data(uint8_t ch_index, uint8_t *new_data_flag);
void CS5530_Reset_Serial(void);
void ADC_Recover_Channel(uint8_t ch);

#endif /* __DRI_CS5530_H__ */
