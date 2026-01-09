#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// === 全局运行时数据 ===
typedef struct {
    uint8_t is_linked;      // 网线物理连接状态
    uint8_t is_tcp_connected; // TCP 连接状态
} RuntimeData_t;

// 必须按1字节对齐，防止编译器插入空隙
#pragma pack(push, 1)

typedef struct {
    // === MBAP Header (7 Bytes) ===
    uint16_t trans_id;    // 0,1: 事务ID (大端)
    uint16_t proto_id;    // 2,3: 协议ID (固定0)
    uint16_t length;      // 4,5: 后续长度 (大端)
    uint8_t  unit_id;     // 6:   站号

    // === PDU (Function 04 Response) ===
    uint8_t  func_code;   // 7:   功能码 (0x04)
    uint8_t  byte_count;  // 8:   数据字节数 (0x30 = 48)

    // === Payload (12 Floats = 48 Bytes) ===
    uint8_t  data[48];    // 存放转换大小端后的原始数据
} ModbusFakePacket_t;

#pragma pack(pop)
typedef struct {
    uint32_t error_count;
    uint32_t stuck_count;
    int32_t  last_val;
    uint32_t freeze_count;
} ADC_Health_t;

// 辅助联合体：用于拆解 float 的 4个字节
typedef union {
    float fVal;
    uint8_t bVal[4];
} FloatUnion_t;

// 定义错误阈值 (比如连续 100ms 都不正常，就复位它)
#define ERROR_THRESHOLD  100


extern RuntimeData_t g_runtime_data;
extern QueueHandle_t g_adc_data_queue; 

#endif /* __APP_DATA_H__ */
