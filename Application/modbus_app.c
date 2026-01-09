#include "modbus_app.h"

// 引用外部数据
extern ChannelCalib_t g_CalibConfigs[CALIB_CH_COUNT];
extern ChannelRuntime_t g_RuntimeData[CALIB_CH_COUNT];
extern wiz_NetInfo g_net_info_cache;
extern volatile bool g_apply_net_change_flag;
bool g_is_streaming_mode = false;
USHORT usRegHoldingBuf[REG_HOLDING_NREGS];

void User_Modbus_Register_Init(void)
{
    // 清空本地保持寄存器缓存
    memset(usRegHoldingBuf, 0, sizeof(usRegHoldingBuf));
}

// ==============================================================================
//  输入寄存器回调 (功能码 04) - 读取实时采样数据
// ==============================================================================
eMBErrorCode eMBRegInputCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;

    // 检查地址范围
    if ((usAddress >= REG_INPUT_START) &&
        (usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS))
    {
        iRegIndex = (int)(usAddress - REG_INPUT_START);

        while (usNRegs > 0)
        {
            int32_t val32 = 0;
            int ch_idx = 0;
            bool is_high = false; // 是否为高16位

            // === A. 校准值 (00 - 23) ===
            // 物理值 (经过标定后的值，如 kg, N)
            if (iRegIndex >= 0 && iRegIndex <= 23)
            {
                ch_idx = iRegIndex / 2;
                is_high = ((iRegIndex % 2) == 0);
                // 扩展 int16 到 int32 以便分两个寄存器传输
                val32 = (int32_t)g_RuntimeData[ch_idx].phy_val;
            }
            // === B. 相对码 (30 - 53) ===
            // 去皮后的原始值 (Abs - Tare)
            else if (iRegIndex >= 30 && iRegIndex <= 53)
            {
                ch_idx = (iRegIndex - 30) / 2;
                is_high = (((iRegIndex - 30) % 2) == 0);
                val32 = g_RuntimeData[ch_idx].rel_raw;
            }
            // === C. 绝对码 (60 - 83) ===
            // ADC 直接读出的原始值
            else if (iRegIndex >= 60 && iRegIndex <= 83)
            {
                ch_idx = (iRegIndex - 60) / 2;
                is_high = (((iRegIndex - 60) % 2) == 0);
                val32 = g_RuntimeData[ch_idx].abs_raw;
            }
            else
            {
                val32 = 0; // 未定义区域填充 0
            }

            // 数据拆分 (Big Endian: 高位在前)
            uint16_t val16;
            if (is_high)
                val16 = (uint16_t)((val32 >> 16) & 0xFFFF);
            else
                val16 = (uint16_t)(val32 & 0xFFFF);

            *pucRegBuffer++ = (UCHAR)(val16 >> 8);
            *pucRegBuffer++ = (UCHAR)(val16 & 0xFF);

            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}


eMBErrorCode eMBRegHoldingCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode)
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;
    bool is_net_changed = false;
    
    // 临时变量：用于保存可能被修改的端口号
    // 默认取当前运行的端口，防止用户只改IP不改端口时端口变成0
    static uint16_t temp_port_for_save = 0; 
    if (temp_port_for_save == 0) temp_port_for_save = g_modbus_tcp_port;

    if ((usAddress >= REG_HOLDING_START) && 
        (usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS))
    {
        iRegIndex = (int)(usAddress - REG_HOLDING_START);

        while (usNRegs > 0)
        {
            // 区域 1: 网络参数 (10 - 28)
            if (iRegIndex >= 10 && iRegIndex <= 28)
            {
                if (eMode == MB_REG_READ)
                {
                    USHORT val = 0;
                    if (iRegIndex >= 10 && iRegIndex <= 13) val = g_net_info_cache.ip[iRegIndex-10];
                    else if (iRegIndex >= 14 && iRegIndex <= 17) val = g_net_info_cache.sn[iRegIndex-14];
                    else if (iRegIndex >= 18 && iRegIndex <= 21) val = g_net_info_cache.gw[iRegIndex-18];
                    else if (iRegIndex == 22) val = temp_port_for_save; // 返回当前端口
                    else if (iRegIndex >= 23 && iRegIndex <= 28) val = g_net_info_cache.mac[iRegIndex-23];
                    
                    *pucRegBuffer++ = (UCHAR)(val >> 8);
                    *pucRegBuffer++ = (UCHAR)(val & 0xFF);
                }
                else // Write
                {
                    USHORT val = (USHORT)pucRegBuffer[0] << 8 | (USHORT)pucRegBuffer[1];
                    
                    // 写入逻辑 (更新缓存并标记改变)
                    if (iRegIndex >= 10 && iRegIndex <= 13) { 
                        g_net_info_cache.ip[iRegIndex-10] = (uint8_t)val; 
                        is_net_changed = true; 
                    }
                    else if (iRegIndex >= 14 && iRegIndex <= 17) { 
                        g_net_info_cache.sn[iRegIndex-14] = (uint8_t)val; 
                        is_net_changed = true; 
                    }
                    else if (iRegIndex >= 18 && iRegIndex <= 21) { 
                        g_net_info_cache.gw[iRegIndex-18] = (uint8_t)val; 
                        is_net_changed = true; 
                    }
                    else if (iRegIndex == 22) { 
                        temp_port_for_save = val; // 记录新端口
                        is_net_changed = true; 
                    }
                    // MAC地址一般不允许远程修改，或者修改后不触发热重启，此处略过标记 is_net_changed
                    
                    pucRegBuffer += 2;
                }
            }
            // 区域 2: 标定参数 (31 - 390)
            else if (iRegIndex >= REG_CAL_START_ADDR && iRegIndex <= 390)
            {
                int offset_global = iRegIndex - REG_CAL_START_ADDR;
                int ch_idx        = offset_global / REG_CAL_BLOCK_SIZE;
                int offset_inner  = offset_global % REG_CAL_BLOCK_SIZE;

                if (ch_idx < CALIB_CH_COUNT)
                {
                    ChannelCalib_t *pCfg = &g_CalibConfigs[ch_idx];

                    if (eMode == MB_REG_READ)
                    {
                        int16_t val16 = 0;
                        if (offset_inner < 10) {
                            val16 = pCfg->points[offset_inner].weight_val;
                        } else {
                            int raw_pt_idx = (offset_inner - 10) / 2;
                            bool is_high   = ((offset_inner - 10) % 2 == 0);
                            int32_t val32 = pCfg->points[raw_pt_idx].raw_code;
                            if (is_high) val16 = (int16_t)(val32 >> 16);
                            else         val16 = (int16_t)(val32 & 0xFFFF);
                        }
                        *pucRegBuffer++ = (UCHAR)(val16 >> 8); *pucRegBuffer++ = (UCHAR)(val16 & 0xFF);
                    }
                    else // MB_REG_WRITE
                    {
                        USHORT usValue = (USHORT)pucRegBuffer[0] << 8 | (USHORT)pucRegBuffer[1];
                        if (offset_inner < 10) {
                            pCfg->points[offset_inner].weight_val = (int16_t)usValue;
                        } else {
                            int raw_pt_idx = (offset_inner - 10) / 2;
                            bool is_high   = ((offset_inner - 10) % 2 == 0);
                            int32_t current = pCfg->points[raw_pt_idx].raw_code;
                            if (is_high) { current &= 0x0000FFFF; current |= ((int32_t)usValue << 16); }
                            else         { current &= 0xFFFF0000; current |= (uint16_t)usValue; }
                            pCfg->points[raw_pt_idx].raw_code = current;
                        }
                        // 写入标定参数后立即保存
                        Calib_SaveChannel(ch_idx);
                        pucRegBuffer += 2;
                    }
                }
                else {
                    if(eMode == MB_REG_READ) { *pucRegBuffer++ = 0; *pucRegBuffer++ = 0; } else { pucRegBuffer += 2; }
                }
            }
            else if (iRegIndex == REG_STREAM_CTRL)
            {
                if (eMode == MB_REG_READ) {
                    USHORT val = g_is_streaming_mode ? 1 : 0;
                    *pucRegBuffer++ = (UCHAR)(val >> 8); *pucRegBuffer++ = (UCHAR)(val & 0xFF);
                } else {
                    USHORT val = (USHORT)pucRegBuffer[0] << 8 | (USHORT)pucRegBuffer[1];
                    bool new_state = (val == 1);
                    if (g_is_streaming_mode != new_state) {
                        g_is_streaming_mode = new_state;
                    }
                    pucRegBuffer += 2;
                }
            }
            else
            {
                // 空闲区域
                if(eMode == MB_REG_READ) { *pucRegBuffer++ = 0; *pucRegBuffer++ = 0; } else { pucRegBuffer += 2; }
            }

            iRegIndex++;
            usNRegs--;
        }

        if (is_net_changed)
        {
            // 1. 保存参数到 EEPROM
            NetConfig_SaveParams(g_net_info_cache.ip, 
                                 g_net_info_cache.sn, 
                                 g_net_info_cache.gw, 
                                 g_net_info_cache.mac,
                                 temp_port_for_save);
            
            // 2. 触发 FreeRTOS 任务中的热重启逻辑
            g_apply_net_change_flag = true;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

// 线圈回调
eMBErrorCode eMBRegCoilsCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode)
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iIndex;

    if ((usAddress >= REG_COILS_START) &&
        (usAddress + usNCoils <= REG_COILS_START + REG_COILS_SIZE))
    {
        iIndex = (int)(usAddress - REG_COILS_START);

        if (eMode == MB_REG_WRITE)
        {
            UCHAR *pucBits = pucRegBuffer;
            USHORT usBitsLeft = usNCoils;
            USHORT usBitOffset = 0;

            while (usBitsLeft--)
            {
                uint8_t status = (*pucBits >> usBitOffset) & 0x01;
                if (status == 1)
                {
                    Calib_Tare(iIndex);
                }
                iIndex++;
                usBitOffset++;
                if (usBitOffset == 8)
                {
                    usBitOffset = 0;
                    pucBits++;
                }
            }
        }
        else // MB_REG_READ
        {
            // 直接清零返回，因为线圈是触发型动作，无状态保持
            memset(pucRegBuffer, 0, (usNCoils + 7) / 8);
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

eMBErrorCode eMBRegDiscreteCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNDiscrete)
{
    return MB_ENOREG;
}
