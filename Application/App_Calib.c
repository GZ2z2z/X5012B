#include "App_Calib.h"
#include "Int_EEPROM.h"
#include <string.h>

// EEPROM 地址定义
#define CALIB_EEPROM_BASE 0x0100
#define CALIB_DATA_SIZE sizeof(ChannelCalib_t)

#define CALIB_INTERNAL_ADDR 0x0800

#define SCALING_FACTOR 0.74505806f

ChannelCalib_t g_CalibConfigs[CALIB_CH_COUNT];
ChannelRuntime_t g_RuntimeData[CALIB_CH_COUNT];

InternalCalibPoint_t g_InternalCalib[CALIB_CH_COUNT];

static const int32_t TARGET_STD_VALUES[3] = {0, 50000, 200000};

static const int32_t INTERNAL_CALIB_DEFAULT[12][3] = {
    {0, 50000, 200000}, // Ch0
    {0, 50000, 200000}, // Ch1
    {0, 50000, 200000}, // Ch2
    {0, 50000, 200000}, // Ch3
    {0, 50000, 200000}, // Ch4
    {0, 50000, 200000}, // Ch5
    {0, 50000, 200000}, // Ch6
    {0, 50000, 200000}, // Ch7
    {0, 50000, 200000}, // Ch8
    {0, 50000, 200000}, // Ch9
    {0, 50000, 200000}, // Ch10
    {0, 50000, 200000}  // Ch11
};

static uint16_t Calc_Bytes_CRC(uint8_t *pData, uint16_t len)
{
    uint16_t sum = 0;
    for (int i = 0; i < len; i++)
        sum += pData[i];
    return sum;
}

static uint16_t Calc_CRC(ChannelCalib_t *pData)
{
    return Calc_Bytes_CRC((uint8_t *)pData, sizeof(ChannelCalib_t) - 2);
}

void Calib_LoadInternal(void)
{
    InternalCalibStorage_t storage;

    // 1. 读取 EEPROM
    Int_EEPROM_ReadBuffer(CALIB_INTERNAL_ADDR, (uint8_t *)&storage, sizeof(InternalCalibStorage_t));

    // 2. 计算 CRC
    uint16_t calc_crc = Calc_Bytes_CRC((uint8_t *)&storage.channels, sizeof(storage.channels));

    // 3. 校验
    if (calc_crc == storage.crc)
    {
        // 校验成功，使用 EEPROM 数据
        memcpy(g_InternalCalib, storage.channels, sizeof(g_InternalCalib));
    }
    else
    {
        // 校验失败（可能是第一次运行），加载默认值并保存
        Calib_ResetInternalToDefault();
    }
}

void Calib_SaveInternal(void)
{
    InternalCalibStorage_t storage;

    // 拷贝 RAM 数据到存储结构
    memcpy(storage.channels, g_InternalCalib, sizeof(storage.channels));

    // 计算 CRC
    storage.crc = Calc_Bytes_CRC((uint8_t *)&storage.channels, sizeof(storage.channels));

    // 写入 EEPROM
    Int_EEPROM_WriteSafe(CALIB_INTERNAL_ADDR, (uint8_t *)&storage, sizeof(InternalCalibStorage_t));
}

void Calib_ResetInternalToDefault(void)
{
    for (int i = 0; i < CALIB_CH_COUNT; i++)
    {
        for (int k = 0; k < 3; k++)
        {
            g_InternalCalib[i].point_raw[k] = INTERNAL_CALIB_DEFAULT[i][k];
        }
    }
    Calib_SaveInternal(); // 立即保存默认值
}

static int32_t Internal_Correct_Process(uint8_t ch_idx, int32_t raw_in)
{
    if (ch_idx >= CALIB_CH_COUNT)
        return raw_in;

    // 修改：不再指向 const 数组，而是指向 RAM 变量
    // const int32_t *src = INTERNAL_CALIB_SOURCE[ch_idx];
    const int32_t *src = g_InternalCalib[ch_idx].point_raw;

    const int32_t *dst = TARGET_STD_VALUES;

    float result = 0.0f;
    // 区间 1: 0mV ~ 0.5mV
    if (raw_in < src[1])
    {
        if (src[1] == src[0])
            return dst[0];
        float slope = (float)(dst[1] - dst[0]) / (float)(src[1] - src[0]);
        result = dst[0] + slope * (raw_in - src[0]);
    }
    // 区间 2: 0.5mV ~ 2mV 及以上
    else
    {
        if (src[2] == src[1])
            return dst[1];
        float slope = (float)(dst[2] - dst[1]) / (float)(src[2] - src[1]);
        result = dst[1] + slope * (raw_in - src[1]);
    }

    return (int32_t)result;
}

void Calib_Init(void)
{
    memset(g_RuntimeData, 0, sizeof(g_RuntimeData));

    // 1. 初始化用户标定 (原逻辑)
    for (int i = 0; i < CALIB_CH_COUNT; i++)
    {
        uint16_t addr = CALIB_EEPROM_BASE + (i * CALIB_DATA_SIZE);
        Int_EEPROM_ReadBuffer(addr, (uint8_t *)&g_CalibConfigs[i], sizeof(ChannelCalib_t));

        if (Calc_CRC(&g_CalibConfigs[i]) != g_CalibConfigs[i].crc)
        {
            memset(&g_CalibConfigs[i], 0, sizeof(ChannelCalib_t));
        }
    }

    // 2. 初始化内部永久校准 (新增逻辑)
    Calib_LoadInternal();
}

void Calib_SaveChannel(uint8_t ch_idx)
{
    if (ch_idx >= CALIB_CH_COUNT)
        return;
    uint8_t count = 0;
    for (int i = 0; i < CALIB_MAX_POINTS; i++)
    {
        if (g_CalibConfigs[ch_idx].points[i].raw_code != 0 || g_CalibConfigs[ch_idx].points[i].weight_val != 0)
        {
            count++;
        }
    }
    g_CalibConfigs[ch_idx].valid_points = count;
    g_CalibConfigs[ch_idx].crc = 0;
    g_CalibConfigs[ch_idx].crc = Calc_CRC(&g_CalibConfigs[ch_idx]);
    uint16_t addr = CALIB_EEPROM_BASE + (ch_idx * CALIB_DATA_SIZE);
    Int_EEPROM_WriteSafe(addr, (uint8_t *)&g_CalibConfigs[ch_idx], sizeof(ChannelCalib_t));
}

void Calib_Tare(uint8_t ch_idx)
{
    if (ch_idx >= CALIB_CH_COUNT)
        return;
    g_RuntimeData[ch_idx].tare_val = g_RuntimeData[ch_idx].abs_raw;
}

void Calib_Process(uint8_t ch_idx, int32_t raw_input_from_adc)
{
    if (ch_idx >= CALIB_CH_COUNT)
        return;
    // 1. 硬件去噪：右移3位
    int32_t val_shifted = raw_input_from_adc >> 11;

    float val_f = (float)val_shifted * SCALING_FACTOR;
    int32_t val_scaled = (int32_t)val_f;

    // 此处调用已经修改过的 Internal_Correct_Process
    int32_t val_standardized = Internal_Correct_Process(ch_idx, val_scaled);

    // 4. 更新全局绝对码 (0~200000 的标准码)
    g_RuntimeData[ch_idx].abs_raw = val_standardized;

    // 5. 更新相对码
    g_RuntimeData[ch_idx].rel_raw = g_RuntimeData[ch_idx].abs_raw - g_RuntimeData[ch_idx].tare_val;
    // 6. 用户 Modbus 标定映射
    int32_t input_val = g_RuntimeData[ch_idx].rel_raw;
    ChannelCalib_t *pCfg = &g_CalibConfigs[ch_idx];
    if (pCfg->valid_points < 2)
    {
        g_RuntimeData[ch_idx].phy_val = input_val;
        return;
    }

    int i;
    if (input_val <= pCfg->points[0].raw_code)
    {
        i = 0;
    }
    else if (input_val >= pCfg->points[pCfg->valid_points - 1].raw_code)
    {
        i = pCfg->valid_points - 2;
    }
    else
    {
        for (i = 0; i < pCfg->valid_points - 1; i++)
        {
            if (input_val >= pCfg->points[i].raw_code && input_val < pCfg->points[i + 1].raw_code)
            {
                break;
            }
        }
    }

    int32_t x0 = pCfg->points[i].raw_code;
    int32_t x1 = pCfg->points[i + 1].raw_code;
    int32_t y0 = pCfg->points[i].weight_val;
    int32_t y1 = pCfg->points[i + 1].weight_val;
    if (x1 == x0)
    {
        g_RuntimeData[ch_idx].phy_val = y0;
    }
    else
    {
        float slope = (float)(y1 - y0) / (float)(x1 - x0);
        float result = y0 + slope * (input_val - x0);
        g_RuntimeData[ch_idx].phy_val = (int32_t)result;
    }
}
