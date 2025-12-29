#include "app_led.h"
#include "FreeRTOS.h"
#include "task.h"

// 默认初始状态为无通信
volatile SystemState_t g_system_state = SYS_STATE_RUNNING_NO_COMM;

/**
 * @brief 设置LED状态辅助函数
 * @param r_on: 1=红灯亮, 0=红灯灭
 * @param g_on: 1=绿灯亮, 0=绿灯灭
 */
static void LED_Set(uint8_t r_on, uint8_t g_on)
{
    // 假设是低电平点亮 (Active Low)
    HAL_GPIO_WritePin(LED_COM_GPIO_Port, LED_COM_Pin, r_on ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_ST_GPIO_Port, LED_ST_Pin, g_on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void Task_LED_Entry(void *param)
{
    // 上电初期稍微延时
    vTaskDelay(pdMS_TO_TICKS(100));

    for(;;)
    {
        switch(g_system_state)
        {
            case SYS_STATE_INIT_ERROR:
                // 红灯常亮，绿灯灭
                LED_Set(1, 0);
                vTaskDelay(pdMS_TO_TICKS(200)); 
                break;

            case SYS_STATE_RUNNING_NO_COMM:
                // 红绿交替闪烁 1Hz (周期1000ms -> 红500ms, 绿500ms)
                
                // 红亮 绿灭
                LED_Set(1, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                
                // 状态检查：如果状态变了，立即切出去，增加响应速度
                if(g_system_state != SYS_STATE_RUNNING_NO_COMM) break;

                // 红灭 绿亮
                LED_Set(0, 1);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;

            case SYS_STATE_COMM_ESTABLISHED:

                // 红灭 绿亮
                LED_Set(0, 1);
                vTaskDelay(pdMS_TO_TICKS(200));
                break;

            default:
                // 默认全灭
                LED_Set(0, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
        }
    }
}
