#ifndef __APP_NETWORK_H__
#define __APP_NETWORK_H__

#include "main.h"
#include "app_data.h"
#include "wizchip_conf.h"
#include "socket.h"
#include "spi.h"          // 必须包含，以获取 hspi1 或 hspi2
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <string.h>
#include "Com_Debug.h"
#include "App_Commu.h" // 包含新的头文件


// 任务函数声明
void Task_Network(void *param);
void W5500_Critical_Enter(void);
void W5500_Critical_Exit(void);

#endif /* __APP_NETWORK_H__ */
