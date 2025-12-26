#ifndef __APP_COMMU_H__
#define __APP_COMMU_H__

#include <stdint.h>
#include "app_data.h"
#include "Com_CRC.h"
#include "socket.h"
#include "Int_EEPROM.h"
#include "Com_Debug.h"
#include <string.h>
#include "main.h"

// 协议端口
#define COMMU_UDP_PORT          9999

// 帧格式常量 (参考 Low.doc)
#define FRAME_START             0x1B
#define FRAME_TOKEN             0x0E
#define FRAME_END               0x16

// 命令码定义
#define CMD_SIGN_ON             0x0001
#define CMD_DEVICE_VERSION      0x0002
#define CMD_READ_PRM            0x0007
#define CMD_WRITE_PRM           0x0008
#define CMD_SEND_START          0x0101
#define CMD_SEND_STOP           0x0102
#define CMD_SET_TARE            0x0105
#define CMD_REAL_TIME_DATA      0x01C1

// 状态码
#define STATUS_OK               0x00
#define STATUS_ERROR_CRC        0x82
#define STATUS_ERROR_CMD        0x80
#define STATUS_ERROR_LEN        0x84
#define STATUS_ERROR_ADDR       0x86
#define STATUS_ERROR_WRITE      0x88 

// 数据结构对齐
#pragma pack(push, 1)

// UDP帧头
typedef struct {
    uint8_t  start;       // 0x1B
    uint8_t  addr_pad;    // 0x00 (UDP不需要Addr)
    uint8_t  index;       // 索引/流水号
    uint8_t  rsv1;        // 0x00
    uint16_t len;         // Body长度 (Little Endian)
    uint8_t  rsv2;        // 0x00
    uint8_t  token;       // 0x0E
} Frame_Header_t;

// 实时数据结构 (上传)
typedef struct {
    uint16_t cmd;         // 0x01C1
    uint16_t status;
    uint16_t input;
    uint16_t output;
    uint16_t sw_rw;
    int32_t  ctrl_val;
    uint32_t time_base;
    uint16_t rsv;
    int32_t  smpl[12];    // 12通道数据
} Frame_RealTime_t;

// 去皮请求
typedef struct {
    uint8_t  tare_num;
    uint8_t  tare_sr;
    int32_t  zero_code;
} Set_Tare_Request_t;

#pragma pack(pop)

// 初始化
void Commu_Init(void);
// UDP处理函数 (需要在任务中循环调用)
void Commu_UDP_Task_Entry(void);
// 如果开启了流模式，调用此函数发送数据
void Commu_Send_RealTime_If_Active(void);

#endif
