#include "net_config.h"
#include <string.h>

/* ------------------- CRC16-CCITT 计算函数 ------------------- */
static uint16_t NetConfig_CalcCRC(const uint8_t *pData, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    uint16_t i;

    while (length--)
    {
        crc ^= ((uint16_t)(*pData++)) << 8;
        for (i = 0; i < 8; i++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/* ------------------- 填充默认配置 ------------------- */
static void NetConfig_FillDefault(NetConfig_t *cfg)
{
    memset(cfg, 0, sizeof(NetConfig_t));

    cfg->magic = NETCFG_MAGIC;
    cfg->version = NETCFG_VERSION;

    cfg->ip[0] = NETCFG_DEFAULT_IP0;
    cfg->ip[1] = NETCFG_DEFAULT_IP1;
    cfg->ip[2] = NETCFG_DEFAULT_IP2;
    cfg->ip[3] = NETCFG_DEFAULT_IP3;

    cfg->sn[0] = NETCFG_DEFAULT_SN0;
    cfg->sn[1] = NETCFG_DEFAULT_SN1;
    cfg->sn[2] = NETCFG_DEFAULT_SN2;
    cfg->sn[3] = NETCFG_DEFAULT_SN3;

    cfg->gw[0] = NETCFG_DEFAULT_GW0;
    cfg->gw[1] = NETCFG_DEFAULT_GW1;
    cfg->gw[2] = NETCFG_DEFAULT_GW2;
    cfg->gw[3] = NETCFG_DEFAULT_GW3;

    cfg->port = NETCFG_DEFAULT_PORT;
}

/* ------------------- 从 EEPROM 读取配置 ------------------- */
bool NetConfig_LoadFromEEPROM(NetConfig_t *cfg)
{
    if (cfg == NULL)
        return false;

    if (Int_EEPROM_ReadBuffer(NETCFG_EEPROM_ADDR, (uint8_t *)cfg, sizeof(NetConfig_t)) != HAL_OK)
    {
        return false;
    }

    if (cfg->magic != NETCFG_MAGIC || cfg->version != NETCFG_VERSION)
    {
        return false;
    }

    uint16_t calc_crc = NetConfig_CalcCRC((uint8_t *)cfg,
                                          (uint16_t)(sizeof(NetConfig_t) - sizeof(uint16_t))); // 不包含 crc16 字段

    if (calc_crc != cfg->crc16)
    {
        return false;
    }

    return true;
}

/* ------------------- 保存配置到 EEPROM ------------------- */
bool NetConfig_SaveToEEPROM(const NetConfig_t *cfg)
{
    if (cfg == NULL)
        return false;

    NetConfig_t tmp;
    memcpy(&tmp, cfg, sizeof(NetConfig_t));

    tmp.crc16 = NetConfig_CalcCRC((uint8_t *)&tmp,
                                  (uint16_t)(sizeof(NetConfig_t) - sizeof(uint16_t)));

    Int_EEPROM_WriteSafe(NETCFG_EEPROM_ADDR, (uint8_t *)&tmp, sizeof(NetConfig_t));

    return true;
}

/* ------------------- 直接用参数构造并保存 ------------------- */
bool NetConfig_SaveParams(uint8_t ip[4], uint8_t sn[4], uint8_t gw[4], uint8_t mac[6], uint16_t port)
{
    NetConfig_t cfg;
    NetConfig_FillDefault(&cfg);

    if (ip != NULL)
    {
        cfg.ip[0] = ip[0];
        cfg.ip[1] = ip[1];
        cfg.ip[2] = ip[2];
        cfg.ip[3] = ip[3];
    }

    if (sn != NULL)
    {
        cfg.sn[0] = sn[0];
        cfg.sn[1] = sn[1];
        cfg.sn[2] = sn[2];
        cfg.sn[3] = sn[3];
    }

    if (gw != NULL)
    {
        cfg.gw[0] = gw[0];
        cfg.gw[1] = gw[1];
        cfg.gw[2] = gw[2];
        cfg.gw[3] = gw[3];
    }

    if (mac != NULL)
    {
        cfg.mac[0] = mac[0];
        cfg.mac[1] = mac[1];
        cfg.mac[2] = mac[2];
        cfg.mac[3] = mac[3];
        cfg.mac[4] = mac[4];
        cfg.mac[5] = mac[5];
    }

    cfg.port = port;

    return NetConfig_SaveToEEPROM(&cfg);
}

/* ------------------- 生成默认 NetInfo ------------------- */
static void NetConfig_FillDefaultNetInfo(wiz_NetInfo *pNetInfo, uint16_t *pPort)
{
    NetConfig_t cfg;
    NetConfig_FillDefault(&cfg);

    if (pNetInfo != NULL)
    {
        memset(pNetInfo, 0, sizeof(wiz_NetInfo));

        pNetInfo->ip[0] = cfg.ip[0];
        pNetInfo->ip[1] = cfg.ip[1];
        pNetInfo->ip[2] = cfg.ip[2];
        pNetInfo->ip[3] = cfg.ip[3];

        pNetInfo->sn[0] = cfg.sn[0];
        pNetInfo->sn[1] = cfg.sn[1];
        pNetInfo->sn[2] = cfg.sn[2];
        pNetInfo->sn[3] = cfg.sn[3];

        pNetInfo->gw[0] = cfg.gw[0];
        pNetInfo->gw[1] = cfg.gw[1];
        pNetInfo->gw[2] = cfg.gw[2];
        pNetInfo->gw[3] = cfg.gw[3];

        pNetInfo->dhcp = NETINFO_STATIC;
    }

    if (pPort != NULL)
    {
        *pPort = cfg.port;
    }
}

/* ------------------- 根据 CFG & EEPROM 得出最终 NetInfo ------------------- */
void NetConfig_GetEffectiveNetInfo(wiz_NetInfo *pNetInfo, uint16_t *pPort)
{
    if (pNetInfo == NULL || pPort == NULL)
        return;

    /* 检查 CGF（PC13）引脚：低电平表示短接到 GND */
    GPIO_PinState cfg_state = HAL_GPIO_ReadPin(CFG_GPIO_Port, CFG_Pin);

    /* 初始化 EEPROM（软件 I2C），保证后面的读写有效 */
    Int_EEPROM_Init();

    if (cfg_state == GPIO_PIN_RESET)
    {
        /* === CFG 短接：强制用默认 IP，不改 EEPROM 内的数据 === */
        NetConfig_FillDefaultNetInfo(pNetInfo, pPort);
    }
    else
    {
        /* === CFG 未短接：优先用 EEPROM 里的配置 === */
        NetConfig_t cfg;
        if (NetConfig_LoadFromEEPROM(&cfg))
        {
            memset(pNetInfo, 0, sizeof(wiz_NetInfo));

            pNetInfo->ip[0] = cfg.ip[0];
            pNetInfo->ip[1] = cfg.ip[1];
            pNetInfo->ip[2] = cfg.ip[2];
            pNetInfo->ip[3] = cfg.ip[3];

            pNetInfo->sn[0] = cfg.sn[0];
            pNetInfo->sn[1] = cfg.sn[1];
            pNetInfo->sn[2] = cfg.sn[2];
            pNetInfo->sn[3] = cfg.sn[3];

            pNetInfo->gw[0] = cfg.gw[0];
            pNetInfo->gw[1] = cfg.gw[1];
            pNetInfo->gw[2] = cfg.gw[2];
            pNetInfo->gw[3] = cfg.gw[3];

            pNetInfo->dhcp = NETINFO_STATIC;

            *pPort = cfg.port;
        }
        else
        {
            /* EEPROM 没有有效配置，退回默认 */
            NetConfig_FillDefaultNetInfo(pNetInfo, pPort);
        }
    }
}
