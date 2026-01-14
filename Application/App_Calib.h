#ifndef __APP_CALIB_H__
#define __APP_CALIB_H__

#include <stdint.h>
#include <stdbool.h>

#define CALIB_MAX_POINTS  10
#define CALIB_CH_COUNT    12

#pragma pack(push, 1)

typedef struct {
    int32_t weight_val; 
    int32_t raw_code;   
} CalibPoint_t;

typedef struct {
    uint8_t      valid_points;
    CalibPoint_t points[CALIB_MAX_POINTS]; 
    uint16_t     crc;          
} ChannelCalib_t;


typedef struct {
    // 每个通道3个关键点的原始ADC采样值 (对应 0mV, 0.5mV, 2mV)
    // 目标值固定为: 0, 50000, 200000，不需要存，只存采到的 Raw
    int32_t point_raw[3]; 
} InternalCalibPoint_t;

// 包含CRC的存储结构
typedef struct {
    InternalCalibPoint_t channels[CALIB_CH_COUNT];
    uint16_t crc;
} InternalCalibStorage_t;   

#pragma pack(pop)

typedef struct {
    int32_t abs_raw;   
    int32_t tare_val;  
    int32_t rel_raw;   
    int32_t phy_val;   
} ChannelRuntime_t;

extern ChannelCalib_t   g_CalibConfigs[CALIB_CH_COUNT];
extern ChannelRuntime_t g_RuntimeData[CALIB_CH_COUNT];
extern InternalCalibPoint_t g_InternalCalib[CALIB_CH_COUNT];
// 函数声明
void Calib_Init(void);
void Calib_SaveChannel(uint8_t ch_idx);
void Calib_Process(uint8_t ch_idx, int32_t raw_input_from_adc);
void Calib_Tare(uint8_t ch_idx); 


void Calib_SaveInternal(void);
void Calib_ResetInternalToDefault(void);

#endif
