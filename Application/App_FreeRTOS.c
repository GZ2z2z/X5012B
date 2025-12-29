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
#define TASK_MODBUS_PRIORITY (tskIDLE_PRIORITY + 3)
TaskHandle_t TASK_MODBUS_handle;

// W5500 Socket管理任务
void Task_W5500_Manager(void *param);
#define TASK_W5500_STACK_SIZE 512
#define TASK_W5500_PRIORITY (tskIDLE_PRIORITY + 2)
TaskHandle_t TASK_W5500_handle;

void Task_LED_Entry(void *param);
#define TASK_LED_STACK_SIZE 128
#define TASK_LED_PRIORITY (tskIDLE_PRIORITY + 2)
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

static void Restore_Network_Defaults(void)
{
    // 1. 生成默认参数
    wiz_NetInfo default_net = {
        .ip = {192, 168, 0, 234}, // 你的默认IP
        .sn = {255, 255, 255, 0},
        .gw = {192, 168, 0, 1},
        .dhcp = NETINFO_STATIC};
    uint16_t default_port = 502;

    // 2. 写入 EEPROM
    NetConfig_SaveParams(default_net.ip, default_net.sn, default_net.gw, default_port);

    // 3. 触发热重载
    g_apply_net_change_flag = true;

    // debug_printf("!! Network Reset to Defaults !!\r\n");
}

void Task_W5500_Manager(void *param)
{
    extern volatile bool g_apply_net_change_flag;
    extern uint16_t g_modbus_tcp_port;
    extern void MBTCP_PortResetRx(void);
    const TickType_t SHORT_DELAY = pdMS_TO_TICKS(10);

    static uint32_t disconnect_start_tick = 0;

    static uint32_t cfg_btn_press_start = 0;
    static bool cfg_btn_processed = false;
    Int_ETH_Init(); // 只在上电/重启时做一次初始化

    g_system_state = SYS_STATE_RUNNING_NO_COMM;

    for (;;)
    {
        // === CFG 按键长按检测 ===
        if (HAL_GPIO_ReadPin(CFG_GPIO_Port, CFG_Pin) == GPIO_PIN_RESET)
        {
            if (cfg_btn_press_start == 0)
            {
                cfg_btn_press_start = HAL_GetTick();
            }
            else
            {
                // 长按超过 1000ms
                if ((HAL_GetTick() - cfg_btn_press_start > 1000) && !cfg_btn_processed)
                {
                    // 1. 切换 LED 状态为快闪，提示用户“已触发复位”
                    g_system_state = SYS_STATE_NET_RESET;

                    // 2. 阻塞延时 1.5秒，让 LED 闪烁
                    vTaskDelay(pdMS_TO_TICKS(1500));

                    // 3. 执行真正的复位操作
                    Restore_Network_Defaults();

                    // 4. 标记已处理
                    cfg_btn_processed = true;
                }
            }
        }
        else
        {
            // 按键抬起
            cfg_btn_press_start = 0;
            cfg_btn_processed = false;
        }
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

                if ((xTaskGetTickCount() - disconnect_start_tick) > pdMS_TO_TICKS(1000))
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
    vTaskDelay(pdMS_TO_TICKS(500));
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
        if (g_system_state != SYS_STATE_COMM_ESTABLISHED)
        {
            // 如果没连接，就让任务睡 200ms，让出 CPU
            vTaskDelay(pdMS_TO_TICKS(200));

            // 关键：重置时间基准
            // 否则当你一旦连上，vTaskDelayUntil 会为了“追赶”之前的时间而瞬间执行很多次
            xLastWakeTime = xTaskGetTickCount();
            continue; // 跳过本次循环，不执行下面的采样
        }
        for (int i = 0; i < 12; i++)
        {
            // 这里的采样逻辑保持你之前修改后的 (使用 CS5530_Read_Data_Optimized)
            int32_t temp_val = CS5530_Read_Data(i, &ready_flags[i]);

            if (ready_flags[i] == 1)
            {
                g_adc_buffer[i] = temp_val;
                AdcHealth[i].error_count = 0; // 读到数据就清零错误计数
            }
            else
            {
                // 没读到数据 (超时)，不做处理，或者仅做简单统计
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
