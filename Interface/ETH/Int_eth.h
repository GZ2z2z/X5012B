#ifndef __INT_ETH_H__
#define __INT_ETH_H__

#include "wizchip_conf.h"
#include "socket.h"
#include "httpServer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "mb.h"
#include "net_config.h"
#include <stdbool.h>
#include "semphr.h"

#define MODBUS_SOCKET 0
#define MODBUS_PORT   502
#define DEBUG_SOCKET  1
#define DEBUG_PORT    8888

void Int_ETH_Init(void);

/* 公开最终使用的 Modbus 端口与“立即应用”接口 */
extern uint16_t g_modbus_tcp_port;
extern volatile bool g_apply_net_change_flag; 
extern wiz_NetInfo g_net_info_cache; 
void Int_ETH_ApplyNetInfo(const wiz_NetInfo *pNetInfo, uint16_t port);

#endif /* __INT_ETH_H__ */
