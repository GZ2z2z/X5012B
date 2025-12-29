#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include <stdint.h>

// === 全局运行时数据 ===
typedef struct {
    uint8_t is_linked;      // 网线物理连接状态
    uint8_t is_tcp_connected; // TCP 连接状态
    // 这里可以加传感器数据等...
} RuntimeData_t;

typedef struct {
    uint32_t error_count;
    uint32_t stuck_count;
    int32_t  last_val;
    uint32_t freeze_count;
} ADC_Health_t;

// 定义错误阈值 (比如连续 100ms 都不正常，就复位它)
#define ERROR_THRESHOLD  100


extern RuntimeData_t g_runtime_data;

#endif /* __APP_DATA_H__ */
