#include "App_FreeRTOS.h"

// 网络调试任务
// void Task_Debug_Server(void *param);
// #define TASK_DEBUG_STACK_SIZE 256
// #define TASK_DEBUG_PRIORITY (tskIDLE_PRIORITY + 1)
// TaskHandle_t TASK_DEBUG_handle;

void Task_ADC_Verify(void *param);
TaskHandle_t TASK_ADC_VERIFY_handle;

// Modbus核心协议栈任务
void Task_Modbus_TCP(void *param);
#define TASK_MODBUS_STACK_SIZE 512
#define TASK_MODBUS_PRIORITY (tskIDLE_PRIORITY + 2)
TaskHandle_t TASK_MODBUS_handle;

// W5500 Socket管理任务
void Task_W5500_Manager(void *param);
#define TASK_W5500_STACK_SIZE 512
#define TASK_W5500_PRIORITY (tskIDLE_PRIORITY + 2)
TaskHandle_t TASK_W5500_handle;

void Task_LED_Entry(void *param);
#define TASK_LED_STACK_SIZE 128
#define TASK_LED_PRIORITY (tskIDLE_PRIORITY + 1)
TaskHandle_t TASK_LED_handle;

int32_t g_adc_buffer[12] = {0};

void App_freeRTOS_Start(void)
{

    // xTaskCreate(Task_Debug_Server, "Debug_Server", TASK_DEBUG_STACK_SIZE, NULL, TASK_DEBUG_PRIORITY, &TASK_DEBUG_handle);

    xTaskCreate(Task_Modbus_TCP, "Modbus_TCP", TASK_MODBUS_STACK_SIZE, NULL, TASK_MODBUS_PRIORITY, &TASK_MODBUS_handle);

    xTaskCreate(Task_W5500_Manager, "W5500_Manager", TASK_W5500_STACK_SIZE, NULL, TASK_W5500_PRIORITY, &TASK_W5500_handle);
    // 800Hz 比较快，优先级建议设高一点，防止被网络任务打断太久
    xTaskCreate(Task_ADC_Verify, "ADC_Test", 2048, NULL, tskIDLE_PRIORITY + 2, &TASK_ADC_VERIFY_handle);

    xTaskCreate(Task_LED_Entry, "LED_Task", TASK_LED_STACK_SIZE, NULL, TASK_LED_PRIORITY, &TASK_LED_handle);

    // 启动RTOS调度器
    vTaskStartScheduler();
}

// /**
//  * @brief 网络调试服务器任务
//  * @details 这个任务管理一个TCP Socket，用于接收PC客户端连接，并发送调试信息。
//  */
// void Task_Debug_Server(void *param)
// {
//     // 等待网络初始化完成
//     vTaskDelay(pdMS_TO_TICKS(1000));

//     while (1)
//     {
//         switch (getSn_SR(DEBUG_SOCKET))
//         {
//         case SOCK_CLOSED:
//             socket(DEBUG_SOCKET, Sn_MR_TCP, DEBUG_PORT, 0x00);
//             break;

//         case SOCK_INIT:
//             listen(DEBUG_SOCKET);
//             break;

//         case SOCK_LISTEN:
//             vTaskDelay(pdMS_TO_TICKS(100));
//             break;

//         case SOCK_ESTABLISHED:
//             vTaskDelay(pdMS_TO_TICKS(500));
//             break;

//         case SOCK_CLOSE_WAIT:
//             disconnect(DEBUG_SOCKET);
//             break;

//         default:
//             vTaskDelay(pdMS_TO_TICKS(100));
//             break;
//         }
//     }
// }


void Task_Modbus_TCP(void *param)
{
    eMBErrorCode eStatus;

    // 1. 初始化事件队列
    if (!xMBPortEventInit())
        while (1)
            ;

    // 2. 初始化 Modbus TCP 协议栈

    eStatus = eMBTCPInit(g_modbus_tcp_port);
    if (eStatus != MB_ENOERR)
        while (1)
            ;

    eStatus = eMBEnable();
    if (eStatus != MB_ENOERR)
        while (1)
            ;

    for (;;)
    {
        (void)eMBPoll();
        vTaskDelay(1);
    }
}

void Task_W5500_Manager(void *param)
{
    extern volatile bool g_apply_net_change_flag;
    extern uint16_t g_modbus_tcp_port;
    extern void MBTCP_PortResetRx(void);
    const TickType_t SHORT_DELAY = pdMS_TO_TICKS(10);

    static uint32_t disconnect_start_tick = 0;


    Int_ETH_Init(); // 只在上电/重启时做一次初始化

    g_system_state = SYS_STATE_RUNNING_NO_COMM;

    for (;;)
    {
        /* —— 网参热切换：只在收到 0xA55A 后执行 —— */
        if (g_apply_net_change_flag)
        {
            g_apply_net_change_flag = false;
            wiz_NetInfo net_info;
            uint16_t port;
            NetConfig_GetEffectiveNetInfo(&net_info, &port);
            Int_ETH_ApplyNetInfo(&net_info, port); // 内部会断开/重建
            MBTCP_PortResetRx();                   // 清端口层残留
            vTaskDelay(SHORT_DELAY);
            continue;
        }
        uint8_t sn_sr = getSn_SR(MODBUS_SOCKET);
        if (sn_sr == SOCK_ESTABLISHED)
        {
            g_system_state = SYS_STATE_COMM_ESTABLISHED;
            disconnect_start_tick = 0;
        }
        else
        {

            if (g_system_state == SYS_STATE_COMM_ESTABLISHED)
            {
                if (disconnect_start_tick == 0)
                {
                    disconnect_start_tick = xTaskGetTickCount();
                }

                if ((xTaskGetTickCount() - disconnect_start_tick) > pdMS_TO_TICKS(2000))
                {
                    if (g_system_state != SYS_STATE_INIT_ERROR)
                    {
                        g_system_state = SYS_STATE_RUNNING_NO_COMM;
                    }
                }
            }
            else
            {
                // 如果本来就是红灯闪烁状态，保持即可（除非是INIT_ERROR）
                if (g_system_state != SYS_STATE_INIT_ERROR)
                {
                    g_system_state = SYS_STATE_RUNNING_NO_COMM;
                }
            }
        }
        switch (sn_sr)
        {
        case SOCK_CLOSED:
        {
            MBTCP_PortResetRx();

            // 护栏：端口为 0 就回退默认 502
            if (g_modbus_tcp_port == 0)
                g_modbus_tcp_port = 502;

            int8_t sret = socket(MODBUS_SOCKET, Sn_MR_TCP, g_modbus_tcp_port, 0x00);
            if (sret != MODBUS_SOCKET)
                vTaskDelay(pdMS_TO_TICKS(100));
            break;
        }

        case SOCK_INIT:
        {
            static uint32_t init_enter_tick = 0;
            if (init_enter_tick == 0)
                init_enter_tick = HAL_GetTick();

            int8_t lret = listen(MODBUS_SOCKET);
            if (lret != SOCK_OK)
            {
                // 监听失败：短暂退避后继续尝试
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else
            {
                // 调用成功也给芯片一点时间切到 LISTEN
                vTaskDelay(pdMS_TO_TICKS(10));
            }

            // 如果在 INIT 卡了 >= 500ms，说明 listen 没生效或被打断：重建 socket
            if (HAL_GetTick() - init_enter_tick >= 500)
            {
                close(MODBUS_SOCKET);
                MBTCP_PortResetRx(); // 清端口层残留，防止跨连接粘包
                int8_t sret = socket(MODBUS_SOCKET, Sn_MR_TCP, g_modbus_tcp_port, 0x00);
                // 若 socket 开失败，再退避一会
                if (sret != MODBUS_SOCKET)
                    vTaskDelay(pdMS_TO_TICKS(100));
                init_enter_tick = 0; // 重新计时
            }
            break;
        }

        case SOCK_LISTEN:
            vTaskDelay(pdMS_TO_TICKS(500));
            break;

        case SOCK_ESTABLISHED:
        {
            uint16_t rsr = getSn_RX_RSR(MODBUS_SOCKET);
            uint16_t buf = MBTCP_PortBufferedLen();

            if (rsr > 0 || buf > 0)
            {
                // 有新数据或有残留数据需要继续解析，通知 Modbus 任务
                xMBPortEventPost(EV_FRAME_RECEIVED);
                vTaskDelay(pdMS_TO_TICKS(5));
            }
            else
            {
                // 空闲时延时稍微长一点，减少 SPI 总线占用，给 ADC 任务留出带宽
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            break;
        }

        case SOCK_CLOSE_WAIT:
        case SOCK_LAST_ACK:
        case SOCK_FIN_WAIT:
        case SOCK_CLOSING:
        case SOCK_TIME_WAIT:
            disconnect(MODBUS_SOCKET);
            close(MODBUS_SOCKET);
            MBTCP_PortResetRx();
            vTaskDelay(pdMS_TO_TICKS(100));
            break;

        default:
            vTaskDelay(pdMS_TO_TICKS(10));
            break;
        }
    }
}

ADC_Health_t AdcHealth[12] = {0};
void Task_ADC_Verify(void *param)
{
    // 1. 等待网络稳定
    // vTaskDelay(pdMS_TO_TICKS(3000));
    // debug_printf("\r\n=== ADC 800Hz Data View (With Auto-Recovery) ===\r\n");

    // 2. 初始化
    CS5530_Init_All();

    // 3. 启动连续转换
    CS5530_Start_Continuous();


    uint8_t ready_flags[12];
    // uint32_t print_timer = 0;
    // uint32_t print_threshold = 500;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        for (int i = 0; i < 12; i++)
        {

            // 1. 读取数据
            int32_t temp_val = CS5530_Read_Data(i, &ready_flags[i]);

            if (ready_flags[i] == 1)
            {
                // === 读到数据了 (Ready有效) ===

                if (temp_val == 0 || temp_val == 65280)
                {
                    AdcHealth[i].error_count++; // 视为错误累加
                }
                else
                {
                    // === 数据看起来是合法的 ===
                    AdcHealth[i].error_count = 0; // 清除错误计数

                    // 2. 冻结检测 (Freeze Check)
                    // 如果数据和上次完全一样，说明芯片内部逻辑可能锁死
                    if (temp_val == AdcHealth[i].last_val)
                    {
                        AdcHealth[i].freeze_count++;
                    }
                    else
                    {
                        AdcHealth[i].freeze_count = 0;
                        AdcHealth[i].last_val = temp_val; // 更新记录
                    }

                    // 3. 溢出检测 (Stuck Check)
                    // 检测是否卡在最大/最小量程
                    if (temp_val == 8388607 || temp_val == -8388608)
                    {
                        AdcHealth[i].stuck_count++;
                    }
                    else
                    {
                        AdcHealth[i].stuck_count = 0;
                    }

                    // 更新显示缓冲区
                    g_adc_buffer[i] = temp_val;
                }
            }
            else
            {
                // === 没读到数据 (Ready 高电平超时) ===
                AdcHealth[i].error_count++;
            }

            if (AdcHealth[i].error_count > 50 ||
                AdcHealth[i].freeze_count > 50 ||
                AdcHealth[i].stuck_count > 100)
            {
                // 既然判定死了，就显示 0，方便观察
                g_adc_buffer[i] = 0;

                // 执行暴力复位
                ADC_Recover_Channel(i);

                // 清零所有计数器
                AdcHealth[i].error_count = 0;
                AdcHealth[i].freeze_count = 0;
                AdcHealth[i].stuck_count = 0;
                AdcHealth[i].last_val = 0x55AAAA55; // 设个乱码防止误判冻结
            }
        }

        // // ============================================================
        // // 打印逻辑
        // // ============================================================
        // print_timer++;
        // if (print_timer >= print_threshold) // 500ms 打印一次
        // {
        //     print_timer = 0;
        //     debug_printf("\r\n--- ADC Data Snapshot ---\r\n");

        //     debug_printf("Raw[0-5]:  %ld  %ld  %ld  %ld  %ld  %ld\r\n",
        //                  g_adc_buffer[0], g_adc_buffer[1], g_adc_buffer[2],
        //                  g_adc_buffer[3], g_adc_buffer[4], g_adc_buffer[5]);

        //     debug_printf("Raw[6-11]: %ld  %ld  %ld  %ld  %ld  %ld\r\n",
        //                  g_adc_buffer[6], g_adc_buffer[7], g_adc_buffer[8],
        //                  g_adc_buffer[9], g_adc_buffer[10], g_adc_buffer[11]);
        // }

        // 维持 1ms 轮询
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
    }
}
