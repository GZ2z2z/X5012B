#include "modbus_app.h"

/* 本地保持寄存器缓冲区 */
USHORT usRegHoldingBuf[REG_HOLDING_NREGS];
// modbus_app.c
wiz_NetInfo g_cached_net_info = {.gw = {192, 168, 0, 1}}; // 给个初始默认值
/* 网络配置命令状态缓存：0x0000 idle, 0x0001 OK, 0xFFFF error */
USHORT usNetCfgCmdStatus = 0x0000;

extern int32_t g_adc_buffer[12];
// 存储每个通道的皮重值 (Tare Value)
int32_t g_tare_offset[12] = {0};

// 存储线圈状态 (0=绝对, 1=相对)
uint8_t g_coil_states[12] = {0};

static uint16_t usCurrentCalibCh = 0; // 当前操作的通道

// 定义流模式的状态
bool g_is_streaming_mode = false;

extern bool NetConfig_SaveParams(uint8_t ip[4], uint8_t sn[4], uint8_t gw[4], uint16_t port);
extern wiz_NetInfo g_net_info_cache;
extern volatile bool g_apply_net_change_flag;

void User_Modbus_Register_Init(void)
{
    // 清空本地保持寄存器缓存
    memset(usRegHoldingBuf, 0, sizeof(usRegHoldingBuf));
    memset(g_tare_offset, 0, sizeof(g_tare_offset));
    memset(g_coil_states, 0, sizeof(g_coil_states));
}

eMBErrorCode
eMBRegHoldingCB(UCHAR *pucRegBuffer, USHORT usAddress,
                USHORT usNRegs, eMBRegisterMode eMode)
{
    eMBErrorCode eStatus = MB_ENOERR;
    USHORT iRegIndex;
    bool is_net_changed = false; // 标记网络参数是否被修改
    // 标记是否修改了标定数据，如果修改了，最后需要保存
    bool is_calib_changed = false;
    /* 地址范围检查：REG_HOLDING_START=0，长度 REG_HOLDING_NREGS */
    if ((usAddress + usNRegs) <= (REG_HOLDING_START + REG_HOLDING_NREGS))
    {
        iRegIndex = (USHORT)(usAddress - REG_HOLDING_START - 1);

        if (eMode == MB_REG_READ)
        {
            while (usNRegs > 0)
            {
                USHORT usValue = usRegHoldingBuf[iRegIndex];

                /* 对网络相关寄存器：优先从当前 W5500 取运行态值（可选） */
                if (iRegIndex >= REG_HOLD_NET_IP0 && iRegIndex <= REG_HOLD_NET_MAC5)
                {
                    wiz_NetInfo net_info;
                    wizchip_getnetinfo(&net_info);
                    switch (iRegIndex)
                    {
                    case REG_HOLD_NET_IP0:
                        usValue = g_net_info_cache.ip[0];
                        break;
                    case REG_HOLD_NET_IP1:
                        usValue = g_net_info_cache.ip[1];
                        break;
                    case REG_HOLD_NET_IP2:
                        usValue = g_net_info_cache.ip[2];
                        break;
                    case REG_HOLD_NET_IP3:
                        usValue = g_net_info_cache.ip[3];
                        break;

                    case REG_HOLD_NET_SN0:
                        usValue = g_net_info_cache.sn[0];
                        break;
                    case REG_HOLD_NET_SN1:
                        usValue = g_net_info_cache.sn[1];
                        break;
                    case REG_HOLD_NET_SN2:
                        usValue = g_net_info_cache.sn[2];
                        break;
                    case REG_HOLD_NET_SN3:
                        usValue = g_net_info_cache.sn[3];
                        break;

                    case REG_HOLD_NET_GW0:
                        usValue = g_net_info_cache.gw[0];
                        break;
                    case REG_HOLD_NET_GW1:
                        usValue = g_net_info_cache.gw[1];
                        break;
                    case REG_HOLD_NET_GW2:
                        usValue = g_net_info_cache.gw[2];
                        break;
                    case REG_HOLD_NET_GW3:
                        usValue = g_net_info_cache.gw[3];
                        break;
                    case REG_HOLD_NET_PORT:
                        usValue = MODBUS_PORT;
                        break;
                    case REG_HOLD_NET_MAC0:
                        usValue = g_net_info_cache.mac[0];
                        break;
                    case REG_HOLD_NET_MAC1:
                        usValue = g_net_info_cache.mac[1];
                        break;
                    case REG_HOLD_NET_MAC2:
                        usValue = g_net_info_cache.mac[2];
                        break;
                    case REG_HOLD_NET_MAC3:
                        usValue = g_net_info_cache.mac[3];
                        break;
                    case REG_HOLD_NET_MAC4:
                        usValue = g_net_info_cache.mac[4];
                        break;
                    case REG_HOLD_NET_MAC5:
                        usValue = g_net_info_cache.mac[5];
                        break;

                    default:
                        break;
                    }
                }
                // B. 标定参数区 (30-90)
                else
                {
                    // 1. 通道索引 (30)
                    if (iRegIndex == REG_CAL_CH_INDEX)
                    {
                        usValue = usCurrentCalibCh;
                    }
                    // 2. 码值表 (31-60) -> 对应 Weight
                    else if (iRegIndex >= REG_CAL_WEIGHT_START && iRegIndex <= REG_CAL_WEIGHT_END)
                    {
                        int pt_idx = iRegIndex - REG_CAL_WEIGHT_START;
                        usValue = (USHORT)g_CalibData[usCurrentCalibCh].points[pt_idx].weight_val;
                    }
                    // 3. 原始值表 (61-90) -> 对应 Raw
                    else if (iRegIndex >= REG_CAL_RAW_START && iRegIndex <= REG_CAL_RAW_END)
                    {
                        int pt_idx = iRegIndex - REG_CAL_RAW_START;
                        usValue = (USHORT)g_CalibData[usCurrentCalibCh].points[pt_idx].raw_code;
                    }
                }
                // 2. 发送数据 (遵照你的要求：大端序，不交换)
                *pucRegBuffer++ = (UCHAR)(usValue >> 8);   // High Byte First
                *pucRegBuffer++ = (UCHAR)(usValue & 0xFF); // Low Byte Second

                iRegIndex++;
                usNRegs--;
            }
        }
        else if (eMode == MB_REG_WRITE)
        {
            /* -------- 写保持寄存器 -------- */
            while (usNRegs > 0)
            {
                // 1. 获取写入的值 (大端序)
                USHORT usValue = (USHORT)pucRegBuffer[0] << 8 | (USHORT)pucRegBuffer[1];

                // 2. 更新本地缓存 usRegHoldingBuf (可选，如果你的逻辑依赖它)
                usRegHoldingBuf[iRegIndex] = usValue;

                // 3. 实时映射到 g_net_info_cache
                // IP (10-13)
                if (iRegIndex >= REG_HOLD_NET_IP0 && iRegIndex <= REG_HOLD_NET_IP3)
                {
                    g_net_info_cache.ip[iRegIndex - REG_HOLD_NET_IP0] = (uint8_t)usValue;
                    is_net_changed = true;
                }
                // SN (14-17)
                else if (iRegIndex >= REG_HOLD_NET_SN0 && iRegIndex <= REG_HOLD_NET_SN3)
                {
                    g_net_info_cache.sn[iRegIndex - REG_HOLD_NET_SN0] = (uint8_t)usValue;
                    is_net_changed = true;
                }
                // GW (18-21)
                else if (iRegIndex >= REG_HOLD_NET_GW0 && iRegIndex <= REG_HOLD_NET_GW3)
                {
                    g_net_info_cache.gw[iRegIndex - REG_HOLD_NET_GW0] = (uint8_t)usValue;
                    is_net_changed = true;
                }
                // Port (22)
                else if (iRegIndex == REG_HOLD_NET_PORT)
                {
                    // 端口直接存在外部变量里，或者 g_modbus_tcp_port
                    // 这里假设你要存入 EEPROM，所以需要更新到临时的变量用于保存
                    // 注意：g_net_info_cache 结构体里没有 port，port 是单独管理的
                    // 我们需要传递给 NetConfig_SaveParams
                    is_net_changed = true;
                }
                // MAC (23-28) - 这里的 usValue 低8位有效
                else if (iRegIndex >= REG_HOLD_NET_MAC0 && iRegIndex <= REG_HOLD_NET_MAC5)
                {
                    g_net_info_cache.mac[iRegIndex - REG_HOLD_NET_MAC0] = (uint8_t)usValue;
                    // MAC地址修改通常不需要频繁热重载，但如果你需要，也可以设为 true
                    // is_net_changed = true;
                }
                /* 在 eMBRegHoldingCB 的写操作分支里增加 */
                else if (iRegIndex == REG_STREAM_CTRL) // 95
                {
                    bool new_state = (usValue == 1);

                    // 只有状态发生跳变时才处理
                    if (g_is_streaming_mode != new_state)
                    {
                        if (new_state == true)
                        {
                            // 开启瞬间：清空队列，确保发出去的第一包数据是最新的
                            xQueueReset(g_adc_data_queue);
                        }
                        g_is_streaming_mode = new_state;
                    }
                }
                // B. 标定参数 (30-90)
                else
                {
                    // 1. 切换当前通道
                    if (iRegIndex == REG_CAL_CH_INDEX)
                    {
                        if (usValue < CALIB_CH_COUNT)
                        {
                            usCurrentCalibCh = usValue;
                        }
                    }
                    // 2. 写入码值 (Weight)
                    else if (iRegIndex >= REG_CAL_WEIGHT_START && iRegIndex <= REG_CAL_WEIGHT_END)
                    {
                        int pt_idx = iRegIndex - REG_CAL_WEIGHT_START;
                        g_CalibData[usCurrentCalibCh].points[pt_idx].weight_val = (int16_t)usValue;
                        is_calib_changed = true; // 标记数据变动
                    }
                    // 3. 写入原始值 (Raw)
                    else if (iRegIndex >= REG_CAL_RAW_START && iRegIndex <= REG_CAL_RAW_END)
                    {
                        int pt_idx = iRegIndex - REG_CAL_RAW_START;
                        // 这里接收 int16，如果你的ADC是负数，强转 int16 即可
                        g_CalibData[usCurrentCalibCh].points[pt_idx].raw_code = (int16_t)usValue;
                        is_calib_changed = true; // 标记数据变动
                    }
                }
                // 移动指针
                pucRegBuffer += 2;
                iRegIndex++;
                usNRegs--;
            }
            // 如果标定数据发生了改变，立即计算有效点数并保存到 EEPROM
            if (is_calib_changed)
            {
                Calib_RefreshCountAndSave((uint8_t)usCurrentCalibCh);
            }
            // 4. 如果涉及网络参数修改，立即保存并触发热重载
            if (is_net_changed)
            {
                // 获取当前的端口号 (因为上面循环里可能刚刚更新了 usRegHoldingBuf[22])
                uint16_t new_port = usRegHoldingBuf[REG_HOLD_NET_PORT];
                if (new_port == 0)
                    new_port = 502; // 保护

                // 保存到 EEPROM
                NetConfig_SaveParams(g_net_info_cache.ip,
                                     g_net_info_cache.sn,
                                     g_net_info_cache.gw,
                                     new_port);

                // 触发 W5500 任务里的重载逻辑
                g_apply_net_change_flag = true;
            }
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}
// modbus_app.c

// 确保 modbus_app.h 里定义 REG_INPUT_START 为 0
eMBErrorCode eMBRegInputCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;

    // 地址检查：0-23 (甚至更多)
    if (usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS)
    {
        iRegIndex = (int)(usAddress - REG_INPUT_START);

        while (usNRegs > 0)
        {
            // 计算通道号 (0-11)
            int ch_idx = iRegIndex / 2;
            bool is_high_word = ((iRegIndex % 2) == 0);

            if (ch_idx < 12)
            {
                // 1. 获取最新数据 (从全局缓存)
                int32_t val = g_adc_buffer[ch_idx];

                // 2. 去皮逻辑
                if (g_coil_states[ch_idx] == 1) {
                    val -= g_tare_offset[ch_idx];
                }

                // 3. 拆分 32-bit 为 16-bit
                uint16_t sRegValue;
                if (is_high_word) sRegValue = (uint16_t)((val >> 16) & 0xFFFF);
                else              sRegValue = (uint16_t)(val & 0xFFFF);

                // 4. 写入 Modbus 缓冲区 (大端)
                *pucRegBuffer++ = (UCHAR)(sRegValue >> 8);
                *pucRegBuffer++ = (UCHAR)(sRegValue & 0xFF);
            }
            else
            {
                *pucRegBuffer++ = 0; *pucRegBuffer++ = 0;
            }

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

eMBErrorCode
eMBRegCoilsCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNCoils,
              eMBRegisterMode eMode)
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iIndex;

    // 检查地址范围：线圈 0-11
    if ((usAddress >= REG_COILS_START) &&
        (usAddress + usNCoils <= REG_COILS_START + 12))
    {
        iIndex = (int)(usAddress - REG_COILS_START);

        if (eMode == MB_REG_READ)
        {
            // ... (读线圈状态代码，通常不需要改，保持原样即可) ...
            // 简单的位填充逻辑
             UCHAR *pucBits = pucRegBuffer;
             USHORT usBitsLeft = usNCoils;
             USHORT usBitOffset = 0;
             memset(pucRegBuffer, 0, (usNCoils + 7) / 8); // 清零
             while(usBitsLeft--) {
                 if(g_coil_states[iIndex++]) *pucBits |= (1 << usBitOffset);
                 usBitOffset++;
                 if(usBitOffset==8) { usBitOffset=0; pucBits++; }
             }
        }
        else if (eMode == MB_REG_WRITE)
        {
            /* --- 写线圈：控制去皮/清零 --- */
            UCHAR *pucBits = pucRegBuffer;
            USHORT usBitsLeft = usNCoils;
            USHORT usBitOffset = 0;

            while (usBitsLeft > 0)
            {
                // 获取上位机写入的 0 或 1
                uint8_t new_state = (*pucBits >> usBitOffset) & 0x01;

                // 核心去皮逻辑
                if (new_state == 1)
                {
                    // === 上位机置 1：执行去皮 ===
                    // 立即抓取当前的 ADC 原始值作为皮重
                    g_tare_offset[iIndex] = g_adc_buffer[iIndex];
                    g_coil_states[iIndex] = 1;
                }
                else
                {
                    // === 上位机置 0：清除皮重 ===
                    g_tare_offset[iIndex] = 0;
                    g_coil_states[iIndex] = 0;
                }

                iIndex++;
                usBitsLeft--;
                usBitOffset++;
                if (usBitOffset == 8) { usBitOffset = 0; pucBits++; }
            }
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

/* 离散输入寄存器回调 (对应 02 功能码) */
eMBErrorCode
eMBRegDiscreteCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNDiscrete)
{
    // 如果暂不使用
    return MB_ENOREG;
}
