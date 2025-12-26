#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include <stdint.h>

// === 默认网络参数 (硬编码) ===
#define DEFAULT_MAC_ADDR   {0x00, 0x08, 0xDC, 0x11, 0x22, 0x33}
#define DEFAULT_IP_ADDR    {192, 168, 0, 234}
#define DEFAULT_SN_MASK    {255, 255, 255, 0}
#define DEFAULT_GW_ADDR    {192, 168, 0, 1}
#define DEFAULT_LOCAL_PORT 5000  // TCP Server 监听端口
#define DEFAULT_UDP_PORT   5001  // UDP Echo 监听端口
#define DEFAULT_DEBUG_PORT 5002  // Debug Server 监听端口

// === Socket ID 定义 ===
#define SOCK_TCP_SERVER    0
#define SOCK_UDP_ECHO      1
// 定义调试用 Socket ID
#define SOCK_DEBUG         2
// === 接收缓冲区大小 ===
#define RX_BUF_SIZE        2048 

// === 网络配置结构体 (预留给未来EEPROM使用) ===
#pragma pack(push, 1)
typedef struct {
    uint32_t magic_head; // 标志位
    uint8_t  mac[6];
    uint8_t  ip[4];
    uint8_t  sn[4];
    uint8_t  gw[4];
    uint16_t port;
    uint16_t crc16;      // 校验和
} NetConfig_t;
#pragma pack(pop)

// === 全局运行时数据 ===
typedef struct {
    uint8_t is_linked;      // 网线物理连接状态
    uint8_t is_tcp_connected; // TCP 连接状态
    // 这里可以加传感器数据等...
} RuntimeData_t;

typedef struct {
    uint32_t error_count;    // 通讯错误计数 (Ready 不来)
    uint32_t stuck_count;    // 溢出计数 (0x7FFFFF)
    
    // [新增] 
    int32_t  last_val;       // 上一次读到的值
    uint32_t freeze_count;   // 数值冻结(不变)计数
} ADC_Health_t;

// 定义错误阈值 (比如连续 100ms 都不正常，就复位它)
#define ERROR_THRESHOLD  100

// 声明外部变量
extern NetConfig_t   g_net_config;
extern RuntimeData_t g_runtime_data;
extern uint8_t       g_eth_rx_buf[RX_BUF_SIZE];

#endif /* __APP_DATA_H__ */
