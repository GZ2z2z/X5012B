/* ======================= porttcp.c ======================== */
#include "mb.h"
#include "mbport.h"
#include "wizchip_conf.h"
#include "socket.h"
#include "Int_eth.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef MODBUS_SOCKET
#define MODBUS_SOCKET 0
#endif

/* 增大接收缓冲区，防止两个 TCP 包连发时溢出 (Modbus ADU 最大 260，两个就是 520) */
#define MB_TCP_RXBUF_SIZE 1024 
static uint8_t  s_rxBuf[MB_TCP_RXBUF_SIZE];
static uint16_t s_rxLen        = 0;
static uint16_t s_lastFrameLen = 0;

void MBTCP_PortResetRx(void)
{
    /* 进入临界区清空，防止与另一任务并发访问 */
    taskENTER_CRITICAL();
    s_rxLen        = 0;
    s_lastFrameLen = 0;
    taskEXIT_CRITICAL();
}

BOOL xMBTCPPortInit(USHORT usTCPPort)
{
    (void)usTCPPort;
    MBTCP_PortResetRx();
    return TRUE;
}

void vMBTCPPortClose(void)   { }
void vMBTCPPortDisable(void) { }

BOOL xMBTCPPortAccept(int iSocket)
{
    (void)iSocket;
    return TRUE;
}

/* ------- 安全拉取 RX 数据 ------- */
static void w5500_pull_rx(uint8_t sock)
{
    while (1)
    {
        /* 1. 获取芯片内有多少数据 (getSn_RX_RSR 已经由 Int_eth.c 的互斥锁保护了) */
        uint16_t rsz = getSn_RX_RSR(sock);
        if (rsz == 0)
            break;

        /* 2. 计算缓冲区剩余空间 */
        taskENTER_CRITICAL();
        uint16_t curLen = s_rxLen;
        taskEXIT_CRITICAL();

        if (curLen >= MB_TCP_RXBUF_SIZE)
            break; // 缓冲满，停止接收

        uint16_t room = (uint16_t)(MB_TCP_RXBUF_SIZE - curLen);
        uint16_t n    = (rsz < room) ? rsz : room;

        /* 3. 接收数据 */
        int ret = recv(sock, s_rxBuf + curLen, n);
        if (ret <= 0)
            break; // 读取出错或无数据
        
        taskENTER_CRITICAL();
        s_rxLen = (uint16_t)(s_rxLen + (uint16_t)ret);
        taskEXIT_CRITICAL();

        /* 芯片里还有更多数据就先让上层处理一轮，再说 */
        if (n < rsz)
            break;
    }
}

/* 根据 MBAP 头判断第一帧完整长度 */
static uint16_t mbap_first_frame_len(void)
{
    if (s_rxLen < 7)
        return 0;

    // MBAP Header: Transaction(2) + Protocol(2) + Len(2) + UID(1) + PDU...
    // Len 字段(偏移4,5) 包含 UID(1字节) + PDU 的长度
    uint16_t len_field = ((uint16_t)s_rxBuf[4] << 8) | s_rxBuf[5];
    uint16_t total     = (uint16_t)(6 + len_field); 

    // 安全检查：如果长度异常大或小于最小帧，认为是垃圾数据，需要复位
    if (total < 7 || total > MB_TCP_RXBUF_SIZE)
    {
        // 严重错误：协议头损坏，清空缓冲区
        s_rxLen = 0;
        return 0;
    }
    
    // 半包：暂时不交给协议栈
    return (s_rxLen >= total) ? total : 0;
}

BOOL xMBTCPPortGetRequest(UCHAR **ppucMBTCPFrame, USHORT *usTCPLength)
{
    if (!ppucMBTCPFrame || !usTCPLength)
        return FALSE;

    uint8_t sr = getSn_SR(MODBUS_SOCKET);
    /* ☆ 修改：允许在 CLOSE_WAIT 状态也处理最后一帧 */
    if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT)
        return FALSE;

    /* 如果上一帧还没被协议栈处理完（Response 还没发），不要读新数据，保护 s_rxBuf */
    if (s_lastFrameLen != 0)
        return FALSE;

    /* 尝试从 W5500 拉取数据 */
    w5500_pull_rx(MODBUS_SOCKET);

    uint16_t fsz = mbap_first_frame_len();
    if (fsz == 0)
        return FALSE;

    *ppucMBTCPFrame = s_rxBuf;
    *usTCPLength    = fsz;
    
    // 标记当前帧长度，表明正忙于处理此帧
    s_lastFrameLen = fsz; 
    return TRUE;
}

uint16_t MBTCP_PortBufferedLen(void)
{
    return s_rxLen;
}

/* 内部：分片发送 + 总超时控制，避免一次性要求 TX_FSR >= 整帧 */
static BOOL w5500_send_all(uint8_t sock,
                           const uint8_t *buf,
                           uint16_t len,
                           uint32_t total_timeout_ms)
{
    uint32_t deadline = HAL_GetTick() + total_timeout_ms;
    uint16_t sent     = 0;

    while (sent < len)
    {
        uint8_t sr = getSn_SR(sock);
        if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT)
            return FALSE;

        if (HAL_GetTick() > deadline)
            return FALSE;

        uint16_t tx_fsr = getSn_TX_FSR(sock);
        if (tx_fsr == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(2));
            continue;
        }

        uint16_t remains = (uint16_t)(len - sent);
        uint16_t chunk   = (tx_fsr < remains) ? tx_fsr : remains;

        int ret = send(sock, (uint8_t *)(buf + sent), chunk);
        if (ret <= 0)
        {
            vTaskDelay(pdMS_TO_TICKS(2));
            continue;
        }

        sent = (uint16_t)(sent + (uint16_t)ret);
    }

    return TRUE;
}

BOOL xMBTCPPortSendResponse(const UCHAR *pucMBTCPFrame, USHORT usTCPLength)
{
    if (!pucMBTCPFrame || usTCPLength == 0)
        return FALSE;

    uint8_t sr = getSn_SR(MODBUS_SOCKET);
    if (sr != SOCK_ESTABLISHED && sr != SOCK_CLOSE_WAIT)
        return FALSE;

    /* ☆ 修改：分片发送 + 2 秒总超时，更抗网络抖动 / 缓冲不够的情况 */
    BOOL ok = w5500_send_all(MODBUS_SOCKET,
                             (const uint8_t *)pucMBTCPFrame,
                             usTCPLength,
                             2000);

    if (!ok)
    {
        /* 发送失败：断开连接并清理 RX 状态，避免挂死在一个坏连接上 */
        disconnect(MODBUS_SOCKET);
        close(MODBUS_SOCKET);
        MBTCP_PortResetRx();
        return FALSE;
    }

    /* 发送成功：处理粘包（移除已处理的帧） */
    if (s_lastFrameLen && s_lastFrameLen <= s_rxLen)
    {
        uint16_t left = (uint16_t)(s_rxLen - s_lastFrameLen);
        if (left > 0)
        {
            /* 将剩余数据搬移到头部，保证下一帧从 0 开始 */
            memmove(s_rxBuf, s_rxBuf + s_lastFrameLen, left);
        }
        s_rxLen = left;
    }
    else
    {
        /* 异常情况或刚好处理完 */
        s_rxLen = 0;
    }

    s_lastFrameLen = 0; // 解除忙碌标记
    return TRUE;
}
