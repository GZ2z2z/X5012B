#ifndef __APP_CALIB_H__
#define __APP_CALIB_H__

#include <stdint.h>
#include <stdbool.h>

// 定义最大标定点数为 10 
#define CALIB_MAX_POINTS  10
#define CALIB_CH_COUNT    12

// === 关键修改：开启 1 字节对齐 ===
#pragma pack(push, 1)

// 单个标定点结构 (6 Bytes)
typedef struct {
    int16_t weight_val; 
    int32_t raw_code;   
} CalibPoint_t;

// 单个通道的标定配置 (63 Bytes)
typedef struct {
    uint8_t      valid_points; 
    CalibPoint_t points[CALIB_MAX_POINTS]; 
    uint16_t     crc;          
} ChannelCalib_t;

// === 恢复默认对齐 ===
#pragma pack(pop)

// 通道运行时数据 (存 RAM)
typedef struct {
    int32_t abs_raw;   // 绝对码 (寄存器 60-83) 
    int32_t tare_val;  // 皮重值 (清零时保存的绝对码)
    int32_t rel_raw;   // 相对码 (寄存器 30-53) = abs_raw - tare_val 
    int32_t phy_val;   // 校准值 (寄存器 00-23) 
} ChannelRuntime_t;

extern ChannelCalib_t   g_CalibConfigs[CALIB_CH_COUNT];
extern ChannelRuntime_t g_RuntimeData[CALIB_CH_COUNT];

// 函数声明
void Calib_Init(void);
void Calib_SaveChannel(uint8_t ch_idx);
void Calib_Process(uint8_t ch_idx, int32_t current_abs_raw);
void Calib_Tare(uint8_t ch_idx); // 清零

#endif
