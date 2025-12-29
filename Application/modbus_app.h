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



//定义eeprom第二页起始地址
#define EEPROM_PAGE2_ADDR 0x0020

/* 输入寄存器起始地址和数量 */
#define REG_INPUT_START 11
#define REG_INPUT_NREGS 50


/* -------------------- 保持寄存器定义 -------------------- */
/*
 *  保持寄存器布局：
 *
 *  0 : DAC0 输出原始值（0~65535） -> 对应 DAC8532 通道 A
 *  1 : DAC1 输出原始值（0~65535） -> 对应 DAC8532 通道 B
 *
 * 
 *  10~13 : IP 地址，每个寄存器低 8 位为一个字节 (IP0..IP3)
 *  14~17 : 子网掩码 SN0..SN3
 *  18~21 : 网关 GW0..GW3
 *  22    : Modbus TCP 端口
 *  23    : 网络配置命令寄存器：
 *          - 写入 0xA55A：将 10~22 寄存器的值保存到 EEPROM
 *          - 读：
 *              0x0000：未保存或未执行
 *              0x0001：上次保存成功
 *              0xFFFF：上次保存失败
 */

#define REG_HOLDING_START 0
#define REG_HOLDING_NREGS 100

#define REG_HOLD_DAC0_OUT 0
#define REG_HOLD_DAC1_OUT 1
#define REG_HOLD_AI0_MODE 2   // 0=电压模式, 1=电流模式
#define REG_HOLD_AI1_MODE 3   // 0=电压模式, 1=电流模式

#define REG_HOLD_NET_IP0 10
#define REG_HOLD_NET_IP1 11
#define REG_HOLD_NET_IP2 12
#define REG_HOLD_NET_IP3 13

#define REG_HOLD_NET_SN0 14
#define REG_HOLD_NET_SN1 15
#define REG_HOLD_NET_SN2 16
#define REG_HOLD_NET_SN3 17

#define REG_HOLD_NET_GW0 18
#define REG_HOLD_NET_GW1 19
#define REG_HOLD_NET_GW2 20
#define REG_HOLD_NET_GW3 21

#define REG_HOLD_NET_PORT 22

#define REG_HOLD_NET_MAC0 23
#define REG_HOLD_NET_MAC1 24
#define REG_HOLD_NET_MAC2 25
#define REG_HOLD_NET_MAC3 26
#define REG_HOLD_NET_MAC4 27
#define REG_HOLD_NET_MAC5 28



/* -------------------- Coil（2 路 DO）定义 -------------------- */
/*
 * DO0 : 线圈地址 0
 * DO1 : 线圈地址 1
 * 注意：硬件如果是低电平有效（吸合），这里会做映射：
 *   GPIO_PIN_RESET -> Coil = 1 (ON)
 *   GPIO_PIN_SET   -> Coil = 0 (OFF)
 */

#define REG_COILS_START   1
#define REG_COILS_NCOILS  12

/* -------------------- Discrete Input（2 路 DI）定义 -------------------- */
/*
 * DI0 : 离散输入地址 0
 * DI1 : 离散输入地址 1
 */
#define REG_DISCRETE_START 1
#define REG_DISCRETE_NDISCRETES 2



void User_Modbus_Register_Init(void);

#endif /* __MODBUS_APP_H__ */
