#include "App_Calib.h"
#include "Int_EEPROM.h"
#include <string.h>

// 假设 EEPROM 从 0x0100 开始存标定数据
#define CALIB_EEPROM_BASE  0x0100
#define CALIB_DATA_SIZE    sizeof(ChannelCalib_t)

ChannelCalib_t   g_CalibConfigs[CALIB_CH_COUNT];
ChannelRuntime_t g_RuntimeData[CALIB_CH_COUNT];
// 计算 CRC
static uint16_t Calc_CRC(ChannelCalib_t *pData) {
    uint8_t *p = (uint8_t*)pData;
    uint16_t sum = 0;
    for(int i = 0; i < sizeof(ChannelCalib_t) - 2; i++) {
        sum += p[i];
    }
    return sum;
}

// 初始化
void Calib_Init(void) {
    memset(g_RuntimeData, 0, sizeof(g_RuntimeData));
    for(int i = 0; i < CALIB_CH_COUNT; i++) {
        uint16_t addr = CALIB_EEPROM_BASE + (i * CALIB_DATA_SIZE);
        Int_EEPROM_ReadBuffer(addr, (uint8_t*)&g_CalibConfigs[i], sizeof(ChannelCalib_t));
        if(Calc_CRC(&g_CalibConfigs[i]) != g_CalibConfigs[i].crc) {
            memset(&g_CalibConfigs[i], 0, sizeof(ChannelCalib_t));
        }
    }
}

// 保存
void Calib_SaveChannel(uint8_t ch_idx) {
    if(ch_idx >= CALIB_CH_COUNT) return;
    uint8_t count = 0;
    for(int i=0; i<CALIB_MAX_POINTS; i++) {
        if(g_CalibConfigs[ch_idx].points[i].raw_code != 0 || g_CalibConfigs[ch_idx].points[i].weight_val != 0) {
            count++;
        }
    }
    g_CalibConfigs[ch_idx].valid_points = count;
    g_CalibConfigs[ch_idx].crc = 0; 
    g_CalibConfigs[ch_idx].crc = Calc_CRC(&g_CalibConfigs[ch_idx]);
    uint16_t addr = CALIB_EEPROM_BASE + (ch_idx * CALIB_DATA_SIZE);
    Int_EEPROM_WriteSafe(addr, (uint8_t*)&g_CalibConfigs[ch_idx], sizeof(ChannelCalib_t));
}

// 去皮 (设置皮重)
void Calib_Tare(uint8_t ch_idx) {
    if(ch_idx >= CALIB_CH_COUNT) return;
    // 皮重 = 当前绝对码
    g_RuntimeData[ch_idx].tare_val = g_RuntimeData[ch_idx].abs_raw;
}

// 核心计算
void Calib_Process(uint8_t ch_idx, int32_t current_abs_raw) {
    if(ch_idx >= CALIB_CH_COUNT) return;

    // 1. 更新绝对码 (寄存器 60-83)
    g_RuntimeData[ch_idx].abs_raw = current_abs_raw; 

    // 2. 更新相对码 (寄存器 30-53)
    g_RuntimeData[ch_idx].rel_raw = current_abs_raw - g_RuntimeData[ch_idx].tare_val;

    // 3. 更新校准值 (寄存器 00-23)
    int32_t input_val = g_RuntimeData[ch_idx].rel_raw; 
    
    ChannelCalib_t *pCfg = &g_CalibConfigs[ch_idx];
    
    if(pCfg->valid_points < 2) {
        g_RuntimeData[ch_idx].phy_val = input_val; 
        return;
    }

    int i;
    if (input_val <= pCfg->points[0].raw_code) {
        i = 0; 
    } else if (input_val >= pCfg->points[pCfg->valid_points - 1].raw_code) {
        i = pCfg->valid_points - 2; 
    } else {
        for (i = 0; i < pCfg->valid_points - 1; i++) {
            if (input_val >= pCfg->points[i].raw_code && input_val < pCfg->points[i+1].raw_code) {
                break;
            }
        }
    }

    int32_t x0 = pCfg->points[i].raw_code;
    int32_t x1 = pCfg->points[i+1].raw_code;
    int32_t y0 = pCfg->points[i].weight_val;
    int32_t y1 = pCfg->points[i+1].weight_val;

    if (x1 == x0) {
        g_RuntimeData[ch_idx].phy_val = y0;
    } else {
        float slope = (float)(y1 - y0) / (float)(x1 - x0);
        float result = y0 + slope * (input_val - x0);
        g_RuntimeData[ch_idx].phy_val = (int32_t)result; // 强转回 int32
    }
}
