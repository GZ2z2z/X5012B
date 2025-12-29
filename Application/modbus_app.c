#include "modbus_app.h"

/* 本地保持寄存器缓冲区 */
USHORT usRegHoldingBuf[REG_HOLDING_NREGS];
// modbus_app.c
wiz_NetInfo g_cached_net_info = {.gw = {192, 168, 0, 1}}; // 给个初始默认值
/* 网络配置命令状态缓存：0x0000 idle, 0x0001 OK, 0xFFFF error */
USHORT usNetCfgCmdStatus = 0x0000;

extern int32_t g_adc_buffer[12];

void User_Modbus_Register_Init(void)
{
    // 清空本地保持寄存器缓存
    memset(usRegHoldingBuf, 0, sizeof(usRegHoldingBuf));
}

eMBErrorCode
eMBRegHoldingCB(UCHAR *pucRegBuffer, USHORT usAddress,
                USHORT usNRegs, eMBRegisterMode eMode)
{
    eMBErrorCode eStatus = MB_ENOERR;
    USHORT iRegIndex;

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
                if (iRegIndex >= REG_HOLD_NET_IP0 && iRegIndex <= REG_HOLD_NET_GW3)
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

                iRegIndex++;
                usNRegs--;
            }
        }
        else
        {
            eStatus = MB_EINVAL;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}
/* 输入寄存器回调 (对应 04 功能码) */
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

                int32_t raw = g_adc_buffer[iRegIndex];
                usValue = (uint16_t)(raw >> 6);
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

/* 线圈寄存器回调 (对应 01, 05, 15 功能码) */
eMBErrorCode
eMBRegCoilsCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNCoils,
              eMBRegisterMode eMode)
{
    // 如果暂不使用，直接返回错误或成功均可
    return MB_ENOREG;
}

/* 离散输入寄存器回调 (对应 02 功能码) */
eMBErrorCode
eMBRegDiscreteCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNDiscrete)
{
    // 如果暂不使用
    return MB_ENOREG;
}
