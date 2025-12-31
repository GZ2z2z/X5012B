#ifndef __APP_FREERTOS_H__
#define __APP_FREERTOS_H__

#include "FreeRTOS.h"
#include "task.h"
#include "mb.h"      // 包含FreeModbus主头文件
#include "mbport.h"  // 包含FreeModbus移植层头文件
#include "stdio.h"
#include "main.h"
#include "Com_Debug.h"
#include "Com_Util.h"
#include "Int_EEPROM.h"
#include "app_data.h" 
#include "net_config.h"
#include <string.h>   // 为了使用 memset
#include <math.h>
#include "w5500.h"
#include "Dri_CS5530.h"
#include "app_led.h"
#include "modbus_app.h"
#include "Int_ETH.h"
#include "App_Calib.h"
// RuntimeData_t g_runtime;


extern uint16_t MBTCP_PortBufferedLen(void);


void App_freeRTOS_Start(void);


#endif /* __APP_FREERTOS_H__ */
