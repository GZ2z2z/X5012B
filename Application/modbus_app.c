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

                *pucRegBuffer++ = (UCHAR)(usValue >> 8);
                *pucRegBuffer++ = (UCHAR)(usValue & 0xFF);
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

                // 移动指针
                pucRegBuffer += 2;
                iRegIndex++;
                usNRegs--;
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

eMBErrorCode
eMBRegInputCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;

    // 地址检查：映射 0-11 到 ADC 通道 0-11
    if ((usAddress >= REG_INPUT_START) &&
        (usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS))
    {
        iRegIndex = (int)(usAddress - REG_INPUT_START);

        while (usNRegs > 0)
        {
            uint16_t usValue = 0;

            if (iRegIndex >= 0 && iRegIndex < 12)
            {
                // 1. 获取原始值
                int32_t raw = g_adc_buffer[iRegIndex];

                // 2. 减去皮重
                // 如果是绝对模式(Coil=0)，g_tare_offset 是 0，相当于没减
                // 如果是相对模式(Coil=1)，g_tare_offset 是之前存的值
                int32_t final_val = raw - g_tare_offset[iRegIndex];

                // 3. 执行移位操作
                // 注意：由于可能有负数，移位后再强转
                usValue = (uint16_t)final_val;
            }

            // Modbus 大端序填充
            *pucRegBuffer++ = (UCHAR)(usValue >> 8);
            *pucRegBuffer++ = (UCHAR)(usValue & 0xFF);

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
    // int iCoilIndex;

    // 1. 检查地址范围 (线圈 0-11 对应地址 REG_COILS_START 到 +11)
    // 假设 REG_COILS_START 定义为 0 或 1，请根据你的 mb.h 调整，通常 Modbus 地址从 1 开始
    // 这里假设你的 REG_COILS_START 是 1，对应线圈0 (Channel 0)
    if ((usAddress >= REG_COILS_START) &&
        (usAddress + usNCoils <= REG_COILS_START + 12)) // 只有12个通道
    {
        // 计算数组起始索引
        iIndex = (int)(usAddress - REG_COILS_START);

        if (eMode == MB_REG_READ)
        {
            /* --- 读线圈状态 --- */
            UCHAR *pucBits = pucRegBuffer;
            USHORT usBitsLeft = usNCoils;
            USHORT usBitOffset = 0;

            // 清空缓冲区以免残留脏数据
            memset(pucRegBuffer, 0, (usNCoils + 7) / 8);

            while (usBitsLeft > 0)
            {
                // 如果当前通道是相对模式(1)，则置位
                if (g_coil_states[iIndex] == 1)
                {
                    // 设置字节中的特定位
                    *pucBits |= (UCHAR)(1 << usBitOffset);
                }

                iIndex++;
                usBitsLeft--;
                usBitOffset++;

                // 填满一个字节后，指针后移
                if (usBitOffset == 8)
                {
                    usBitOffset = 0;
                    pucBits++;
                }
            }
        }
        else if (eMode == MB_REG_WRITE)
        {
            /* --- 写线圈状态 (核心逻辑) --- */
            UCHAR *pucBits = pucRegBuffer;
            USHORT usBitsLeft = usNCoils;
            USHORT usBitOffset = 0;

            while (usBitsLeft > 0)
            {
                // 从缓冲区提取出当前位是 0 还是 1
                uint8_t new_state = (*pucBits >> usBitOffset) & 0x01;

                // --- 状态切换逻辑 ---
                if (new_state == 1)
                {
                    // 只要写 1，不管之前是啥，立刻抓取当前值作为皮重
                    g_tare_offset[iIndex] = g_adc_buffer[iIndex];
                }
                else
                {
                    // [命令：恢复绝对值]
                    // 写入 0，皮重清零
                    g_tare_offset[iIndex] = 0;
                }

                // 更新状态记录
                g_coil_states[iIndex] = new_state;

                iIndex++;
                usBitsLeft--;
                usBitOffset++;

                if (usBitOffset == 8)
                {
                    usBitOffset = 0;
                    pucBits++;
                }
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
