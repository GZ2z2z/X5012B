#ifndef __APP_FREERTOS_H__
#define __APP_FREERTOS_H__

#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"
#include "main.h"
#include "Com_Debug.h"
#include "Com_Util.h"
#include "Int_EEPROM.h"
#include "app_data.h" 
// #include "net_config.h"
#include <string.h>   // 为了使用 memset
#include <math.h>
#include "w5500.h"
#include "Dri_CS5530.h"

// RuntimeData_t g_runtime;

// 简单的自定义协议帧头
#define CMD_GET_ALL_DATA  0x01
#define CMD_TARE          0x02

void App_freeRTOS_Start(void);


#endif /* __APP_FREERTOS_H__ */
