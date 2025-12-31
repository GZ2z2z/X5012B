#ifndef __APP_CALIB_H__
#define __APP_CALIB_H__

#include <stdint.h>

// 修改最大点数为 30
#define CALIB_MAX_POINTS  30
#define CALIB_CH_COUNT    12

typedef struct {
    int32_t raw_code;   // 原始值
    int16_t weight_val; // 物理量
} CalibPoint_t;

typedef struct {
    uint8_t point_count;             // 有效点数 (程序自动计算)
    CalibPoint_t points[CALIB_MAX_POINTS]; 
    uint16_t crc;
} ChannelCalib_t;

extern ChannelCalib_t g_CalibData[CALIB_CH_COUNT];

// 函数声明
void Calib_Init(void);
void Calib_SaveChannel(uint8_t ch_idx);
float Calib_Calculate(uint8_t ch_idx, int32_t current_raw);
// 新增一个刷新点数的辅助函数
void Calib_RefreshCountAndSave(uint8_t ch_idx);

#endif
