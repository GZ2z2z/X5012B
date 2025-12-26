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

/* 寄存器定义 (命令字中的位) */
#define REG_CONFIG              0x03  // 配置寄存器
#define REG_OFFSET              0x01  // 偏移寄存器
#define REG_GAIN                0x02  // 增益寄存器
#define REG_SETUP               0x04  // 建立寄存器 (CS5532等才有，5530主要是Config)

/* 配置寄存器位掩码 (参考数据手册) */
// 假设使用 RS=1 (复位系统), 具体的字速率(Word Rate)设置需要根据你的晶振频率计算
// 这里提供一个宏供初始化使用，具体数值可能需要根据你的MCLK调整
// SF (Sinc Filter) bits [19:0] define the rate.
// 假设使用默认设置进行验证

/* 函数声明 */
void CS5530_Init_All(void);
void CS5530_Start_Continuous(void);
int32_t CS5530_Read_Data(uint8_t ch_index, uint8_t *new_data_flag);
void CS5530_Reset_Serial(void);
void ADC_Recover_Channel(uint8_t ch);

#endif /* __DRI_CS5530_H__ */
