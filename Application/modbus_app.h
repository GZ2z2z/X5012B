#ifndef __MODBUS_APP_H__
#define __MODBUS_APP_H__

#include "mb.h"
#include "stm32f4xx_hal.h"
#include "wizchip_conf.h"
#include "Int_eth.h"
#include "net_config.h"
#include "mbutils.h"
#include "main.h"
#include "string.h"
#include "app_data.h" 
#include "Int_EEPROM.h"
#include <math.h>
#include "App_FreeRTOS.h"
#include "App_Calib.h"

// ==============================================================================
// 1. 线圈 (Coils) - 功能码 01/05/15
// ==============================================================================
// 解决 warning #47-D: incompatible redefinition
#ifdef REG_COILS_SIZE
#undef REG_COILS_SIZE
#endif

#ifdef REG_COILS_START
#undef REG_COILS_START
#endif

// 地址 00-11: 对应通道 1-12 的清零(去皮)操作
#define REG_COILS_START       1    
#define REG_COILS_SIZE        12   

// ==============================================================================
// 2. 离散输入 (Discrete Inputs) - 功能码 02
// ==============================================================================
#ifdef REG_DISC_SIZE
#undef REG_DISC_SIZE
#endif

#ifdef REG_DISC_START
#undef REG_DISC_START
#endif

#define REG_DISC_START        1
#define REG_DISC_SIZE         0    

// ==============================================================================
// 3. 输入寄存器 (Input Registers) - 功能码 04
// ==============================================================================
#ifdef REG_INPUT_START
#undef REG_INPUT_START
#endif
#ifdef REG_INPUT_NREGS
#undef REG_INPUT_NREGS
#endif

#define REG_INPUT_START       1    
#define REG_INPUT_NREGS       100  

// ==============================================================================
// 4. 保持寄存器 (Holding Registers) - 功能码 03/06/16
// ==============================================================================
#ifdef REG_HOLDING_START
#undef REG_HOLDING_START
#endif
#ifdef REG_HOLDING_NREGS
#undef REG_HOLDING_NREGS
#endif

#define REG_HOLDING_START     1    
#define REG_HOLDING_NREGS     400  

// --- 网络参数 (10-28) ---
#define REG_NET_IP_START      10
#define REG_NET_SN_START      14
#define REG_NET_GW_START      18
#define REG_NET_PORT          22
#define REG_NET_MAC_START     23

// --- 标定参数 (31-390) ---
#define REG_CAL_START_ADDR    31   
#define REG_CAL_BLOCK_SIZE    30   

// --- 流模式控制 (95) ---
#define REG_STREAM_CTRL       95

void User_Modbus_Register_Init(void);

#endif /* __MODBUS_APP_H__ */
