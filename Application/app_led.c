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

// 文件：app_led.c

void Task_LED_Entry(void *param)
{
    // 上电初期稍微延时
    vTaskDelay(pdMS_TO_TICKS(100));

    for(;;)
    {
        switch(g_system_state)
        {
            case SYS_STATE_INIT_ERROR:
                // 初始化错误：红灯常亮 (绿灯灭)
                LED_Set(1, 0);
                vTaskDelay(pdMS_TO_TICKS(200)); 
                break;

            case SYS_STATE_RUNNING_NO_COMM:
                // 正常运行，无连接：
                // 绿灯常亮 (表示系统在跑)
                // 红灯不亮 (表示没连接)
                LED_Set(0, 1); 
                
                // 延时释放 CPU，虽然是常亮，但也需要循环检测状态变化
                vTaskDelay(pdMS_TO_TICKS(200));
                break;

            case SYS_STATE_COMM_ESTABLISHED:
                // Modbus 已连接：
                // 绿灯常亮 (系统还在跑)
                // 红灯常亮 (连接已建立)
                LED_Set(1, 1); 
                
                vTaskDelay(pdMS_TO_TICKS(200));
                break;

            case SYS_STATE_NET_RESET:
                // 网络复位中：保持红灯快闪逻辑
                // 红亮 绿灭
                LED_Set(1, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
                
                // 提高响应速度
                if(g_system_state != SYS_STATE_NET_RESET) break;

                // 全灭
                LED_Set(0, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            default:
                // 默认全灭
                LED_Set(0, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
        }
    }
}
