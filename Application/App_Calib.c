#include "App_Calib.h"
#include "Int_EEPROM.h"
#include <string.h>

// EEPROM 地址规划
// 网络配置在 0x0000 附近，我们从 0x0100 开始存标定数据，防止冲突
#define CALIB_EEPROM_BASE_ADDR 0x0100
#define CALIB_DATA_SIZE        sizeof(ChannelCalib_t)

ChannelCalib_t g_CalibData[CALIB_CH_COUNT];

// 计算简单的累加和校验
static uint16_t Calib_CalcCRC(ChannelCalib_t *pData) {
    uint16_t sum = 0;
    uint8_t *p = (uint8_t*)pData;
    // 计算除最后2字节(crc本身)之外的所有字节
    for(int i=0; i < sizeof(ChannelCalib_t) - 2; i++) {
        sum += p[i];
    }
    return sum;
}

// 初始化：上电读取所有通道标定参数
void Calib_Init(void) {
    for(int i=0; i<CALIB_CH_COUNT; i++) {
        uint16_t addr = CALIB_EEPROM_BASE_ADDR + (i * CALIB_DATA_SIZE);
        
        // 从 EEPROM 读取
        Int_EEPROM_ReadBuffer(addr, (uint8_t*)&g_CalibData[i], sizeof(ChannelCalib_t));
        
        // 校验数据完整性
        uint16_t calc_crc = Calib_CalcCRC(&g_CalibData[i]);
        if(calc_crc != g_CalibData[i].crc || g_CalibData[i].point_count > CALIB_MAX_POINTS) {
            // 校验失败，重置为空
            memset(&g_CalibData[i], 0, sizeof(ChannelCalib_t));
        }
    }
}

// App_Calib.c 中的保存函数
void Calib_SaveChannel(uint8_t ch_idx) {
    if(ch_idx >= CALIB_CH_COUNT) return;
    
    // 更新 CRC
    g_CalibData[ch_idx].crc = Calib_CalcCRC(&g_CalibData[ch_idx]);
    
    uint16_t addr = CALIB_EEPROM_BASE_ADDR + (ch_idx * CALIB_DATA_SIZE);
    

    // 注意：这里包含了 HAL_Delay，会阻塞几十毫秒，确保不要在极高频中断里调用
    Int_EEPROM_WriteSafe(addr, (uint8_t*)&g_CalibData[ch_idx], sizeof(ChannelCalib_t));
}

// 清除通道标定
void Calib_Clear(uint8_t ch_idx) {
    if(ch_idx >= CALIB_CH_COUNT) return;
    memset(&g_CalibData[ch_idx], 0, sizeof(ChannelCalib_t));
    Calib_SaveChannel(ch_idx);
}

// 设置标定点 (仅更新内存，不立即写EEPROM，通常由上层调用Save)
void Calib_SetPoint(uint8_t ch_idx, uint8_t point_idx, int32_t raw, int16_t weight) {
    if(ch_idx >= CALIB_CH_COUNT || point_idx >= CALIB_MAX_POINTS) return;
    
    g_CalibData[ch_idx].points[point_idx].raw_code = raw;
    g_CalibData[ch_idx].points[point_idx].weight_val = weight;
    
    // 自动更新点数：如果是设置第N个点，总点数至少是 N+1
    if((point_idx + 1) > g_CalibData[ch_idx].point_count) {
        g_CalibData[ch_idx].point_count = point_idx + 1;
    }
}




// === 核心算法：多点线性插值 ===
float Calib_Calculate(uint8_t ch_idx, int32_t current_raw) {
    ChannelCalib_t *pCal = &g_CalibData[ch_idx];
    
    // 1. 无标定数据或只有1个点，返回原始值(或0)
    if(pCal->point_count < 2) {
        return (float)current_raw; 
    }

    // 2. 寻找区间
    // 默认假设标定点是按 Raw 值从小到大增加的
    int i;
    
    if(current_raw <= pCal->points[0].raw_code) {
        // 小于第1个点，使用第一段斜率外推
        i = 0; 
    }
    else if(current_raw >= pCal->points[pCal->point_count-1].raw_code) {
        // 大于最后一个点，使用最后一段斜率外推
        i = pCal->point_count - 2;
    }
    else {
        // 在中间寻找区间
        for(i = 0; i < pCal->point_count - 1; i++) {
            if(current_raw >= pCal->points[i].raw_code && current_raw < pCal->points[i+1].raw_code) {
                break;
            }
        }
    }
    
    // 3. 线性插值公式: y = y0 + (x - x0) * (y1 - y0) / (x1 - x0)
    int32_t x0 = pCal->points[i].raw_code;
    int16_t y0 = pCal->points[i].weight_val;
    int32_t x1 = pCal->points[i+1].raw_code;
    int16_t y1 = pCal->points[i+1].weight_val;
    
    if(x1 == x0) return (float)y0; // 防止除零错误
    
    float slope = (float)(y1 - y0) / (float)(x1 - x0);
    float result = y0 + slope * (current_raw - x0);
    
    return result;
}

// 逻辑：遍历30个点，如果 Raw 和 Weight 不全为0，则视为有效点
void Calib_RefreshCountAndSave(uint8_t ch_idx) {
    if(ch_idx >= CALIB_CH_COUNT) return;

    uint8_t valid_count = 0;
    
    // 假设用户是连续填充的，我们统计有效个数
    for(int i = 0; i < CALIB_MAX_POINTS; i++) {
        // 简单判断：只要 Raw 不为0 (或者 Weight 不为0) 就认为是有效点
        // 注意：如果你允许 Raw=0 作为标定点，需要更复杂的逻辑(比如判断 Weight!=0)
        // 这里假设未使用的点都被初始化为 0
        if(g_CalibData[ch_idx].points[i].raw_code != 0 || g_CalibData[ch_idx].points[i].weight_val != 0) {
            valid_count++;
        }
    }
    
    g_CalibData[ch_idx].point_count = valid_count;
    
    // 立即保存到 EEPROM
    Calib_SaveChannel(ch_idx);
}
