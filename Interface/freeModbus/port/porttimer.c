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

 /* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "tim.h"
/* ----------------------- static functions ---------------------------------*/


/* ----------------------- Start implementation -----------------------------*/
// 在 eMBInit() 时被调用，我们在这里启动定时器
BOOL xMBPortTimersInit(USHORT usTim1Timerout50us)
{
    // 在这里一次性启动定时器。对于TCP模式，让它一直运行是最简单的。
    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
    {
        return FALSE; // 如果启动失败，返回错误
    }
    return TRUE;
}

// 因为定时器一直运行，所以这两个函数可以为空
void vMBPortTimersEnable()
{
    // Do nothing. Timer is always running.
}

void vMBPortTimersDisable()
{
    // Do nothing. Timer is always running.
}

