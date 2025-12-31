/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- FreeRTOS includes ----------------------------------*/
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

/* ----------------------- Static variables -----------------------------------*/
// 定义一个队列句柄
static QueueHandle_t xMBEventQueue = NULL;
// 定义队列的长度
#define MB_EVENT_QUEUE_LENGTH (50)

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortEventInit(void)
{
    // 创建一个队列，用于存储Modbus事件
    // 队列长度为 MB_EVENT_QUEUE_LENGTH，每个元素的大小是一个 eMBEventType
    xMBEventQueue = xQueueCreate(MB_EVENT_QUEUE_LENGTH, sizeof(eMBEventType));

    if (xMBEventQueue != NULL)
    {
        return TRUE;
    }
    return FALSE;
}

void vMBPortEventClose(void)
{
    if (xMBEventQueue != NULL)
    {
        vQueueDelete(xMBEventQueue);
        xMBEventQueue = NULL;
    }
}

// 从任务中发送事件
BOOL xMBPortEventPost(eMBEventType eEvent)
{
    // portMAX_DELAY 表示如果队列满了，会一直等待直到有空间
    if (xQueueSend(xMBEventQueue, &eEvent, 0) == pdPASS)
    {
        return TRUE;
    }
    return FALSE;
}

// 从中断服务程序(ISR)中发送事件 (比如定时器中断)
BOOL xMBPortEventPostFromISR(eMBEventType eEvent)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // 这是一个特殊的API，用于在中断中安全地发送消息
    if (xQueueSendFromISR(xMBEventQueue, &eEvent, &xHigherPriorityTaskWoken) == pdPASS)
    {
        // 如果这个事件唤醒了一个比当前任务更高优先级的任务，
        // portYIELD_FROM_ISR() 会请求进行一次上下文切换。
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        return TRUE;
    }
    return FALSE;
}

BOOL xMBPortEventGet(eMBEventType *peEvent)
{
    // portMAX_DELAY 表示如果队列是空的，任务会进入阻塞状态，一直等待事件到来
    if (xQueueReceive(xMBEventQueue, peEvent, 0) == pdPASS)
    {
        return TRUE;
    }
    return FALSE;
}
