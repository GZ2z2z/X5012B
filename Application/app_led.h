#ifndef __APP_LED_H__
#define __APP_LED_H__

#include "main.h"

// 系统运行状态枚举
typedef enum {
    SYS_STATE_INIT_ERROR,      
    SYS_STATE_RUNNING_NO_COMM,  
    SYS_STATE_COMM_ESTABLISHED, 
    SYS_STATE_NET_RESET         
} SystemState_t;

// 全局状态变量
extern volatile SystemState_t g_system_state;

// LED 任务入口
void Task_LED_Entry(void *param);

#endif /* __APP_LED_H__ */
