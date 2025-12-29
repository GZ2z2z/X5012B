#ifndef __NET_CONFIG_H__
#define __NET_CONFIG_H__

#include "wizchip_conf.h"
#include "Int_EEPROM.h"
#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* EEPROM 中网络配置起始地址，可以根据自己布局调整 */
#define NETCFG_EEPROM_ADDR      0x0000

/* 魔数和版本号 */
#define NETCFG_MAGIC            0x4E455431UL   // 'N''E''T''1'
#define NETCFG_VERSION          0x01

/* 默认网络配置（符合你项目要求） */
#define NETCFG_DEFAULT_IP0      192
#define NETCFG_DEFAULT_IP1      168
#define NETCFG_DEFAULT_IP2      0
#define NETCFG_DEFAULT_IP3      234

#define NETCFG_DEFAULT_SN0      255
#define NETCFG_DEFAULT_SN1      255
#define NETCFG_DEFAULT_SN2      255
#define NETCFG_DEFAULT_SN3      0

#define NETCFG_DEFAULT_GW0      192
#define NETCFG_DEFAULT_GW1      168
#define NETCFG_DEFAULT_GW2      0
#define NETCFG_DEFAULT_GW3      1

#define NETCFG_DEFAULT_PORT     502   // Modbus TCP 默认端口



/* 网络配置结构体（packed，避免编译器自动填充） */
#pragma pack(push, 1)
typedef struct
{
    uint32_t magic;     // 固定为 NETCFG_MAGIC
    uint8_t  version;   // 版本号 NETCFG_VERSION
    uint8_t  reserved1[3];

    uint8_t  ip[4];
    uint8_t  sn[4];
    uint8_t  gw[4];

    uint16_t port;      // Modbus TCP 端口

    uint8_t  reserved2[6];  // 预留将来用

    uint16_t crc16;     // CRC16 校验（不含 crc16 字段本身）
} NetConfig_t;
#pragma pack(pop)

/* 加载/保存接口 */
bool NetConfig_LoadFromEEPROM(NetConfig_t *cfg);
bool NetConfig_SaveToEEPROM(const NetConfig_t *cfg);

/* 新增：直接用参数构造并保存 */
bool NetConfig_SaveParams(uint8_t ip[4], uint8_t sn[4], uint8_t gw[4], uint16_t port);

/* 根据 CGF 和 EEPROM 决定最终的 wiz_NetInfo */
void NetConfig_GetEffectiveNetInfo(wiz_NetInfo *pNetInfo, uint16_t *pPort);

#endif /* __NET_CONFIG_H__ */
