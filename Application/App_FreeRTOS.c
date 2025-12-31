#include "App_FreeRTOS.h"

// 网络调试任务
// void Task_Debug_Server(void *param);
// #define TASK_DEBUG_STACK_SIZE 256
// #define TASK_DEBUG_PRIORITY (tskIDLE_PRIORITY + 1)
// TaskHandle_t TASK_DEBUG_handle;

void Task_ADC_Verify(void *param);
TaskHandle_t TASK_ADC_VERIFY_handle;

// // Modbus核心协议栈任务
// void Task_Modbus_TCP(void *param);
// #define TASK_MODBUS_STACK_SIZE 512
// #define TASK_MODBUS_PRIORITY (tskIDLE_PRIORITY + 5)
// TaskHandle_t TASK_MODBUS_handle;

// // W5500 Socket管理任务
// void Task_W5500_Manager(void *param);
// #define TASK_W5500_STACK_SIZE 512
// #define TASK_W5500_PRIORITY (tskIDLE_PRIORITY + 3)
// TaskHandle_t TASK_W5500_handle;

void Task_Network_Core(void *param);
#define TASK_NET_CORE_STACK_SIZE 2048                 // 合并后适当增大栈空间
#define TASK_NET_CORE_PRIORITY (tskIDLE_PRIORITY + 4) // 保持较高优先级
TaskHandle_t TASK_NET_CORE_handle;

void Task_LED_Entry(void *param);
#define TASK_LED_STACK_SIZE 128
#define TASK_LED_PRIORITY (tskIDLE_PRIORITY + 3)
TaskHandle_t TASK_LED_handle;

// === 全局共享数据区 ===
ModbusFakePacket_t g_shared_packet_buffer; // 共享缓冲区
volatile bool g_has_new_data = false;      // 数据更新标志

// 引入临界区保护，防止写一半被读走
#define LOCK_DATA() taskENTER_CRITICAL()
#define UNLOCK_DATA() taskEXIT_CRITICAL()

int32_t g_adc_buffer[12] = {0};
extern int32_t g_tare_offset[12];
extern uint8_t g_coil_states[12];
extern volatile bool g_is_streaming_mode;
extern volatile bool g_apply_net_change_flag;
extern uint16_t g_modbus_tcp_port;
extern void MBTCP_PortResetRx(void);

// 声明队列句柄
QueueHandle_t g_adc_data_queue = NULL;

void App_freeRTOS_Start(void)
{

    Int_EEPROM_Init();
    Calib_Init();
    g_adc_data_queue = xQueueCreate(10, sizeof(ModbusFakePacket_t));
    if (g_adc_data_queue == NULL)
    {
        // 队列创建失败处理，卡死或报错
        while (1)
            ;
    }

    // xTaskCreate(Task_Debug_Server, "Debug_Server", TASK_DEBUG_STACK_SIZE, NULL, TASK_DEBUG_PRIORITY, &TASK_DEBUG_handle);

    // xTaskCreate(Task_Modbus_TCP, "Modbus_TCP", TASK_MODBUS_STACK_SIZE, NULL, TASK_MODBUS_PRIORITY, &TASK_MODBUS_handle);

    // xTaskCreate(Task_W5500_Manager, "W5500_Manager", TASK_W5500_STACK_SIZE, NULL, TASK_W5500_PRIORITY, &TASK_W5500_handle);

    // --- 创建合并后的网络核心任务 ---
    xTaskCreate(Task_Network_Core, "Net_Core", TASK_NET_CORE_STACK_SIZE, NULL, TASK_NET_CORE_PRIORITY, &TASK_NET_CORE_handle);

    // 800Hz 比较快，优先级建议设高一点，防止被网络任务打断太久
    xTaskCreate(Task_ADC_Verify, "ADC_Test", 2048, NULL, tskIDLE_PRIORITY + 5, &TASK_ADC_VERIFY_handle);

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
void Task_Network_Core(void *param)
{
    eMBErrorCode eStatus;
    const TickType_t SHORT_DELAY = pdMS_TO_TICKS(10);
    // 状态机与按键相关变量
    static uint32_t disconnect_start_tick = 0;
    static uint32_t cfg_btn_press_start = 0;
    static bool cfg_btn_processed = false;
    // 本地发送缓存 (从全局区拷贝到这里，再发给W5500)
    ModbusFakePacket_t tx_packet_local;

    uint32_t tx_jammed_start_tick = 0;
    // 1. 硬件初始化 (W5500)
    Int_ETH_Init();
    // 2. 读取网络配置并应用
    wiz_NetInfo net_info;
    uint16_t saved_port;
    NetConfig_GetEffectiveNetInfo(&net_info, &saved_port);
    g_modbus_tcp_port = (saved_port == 0) ? 502 : saved_port;
    Int_ETH_ApplyNetInfo(&net_info, g_modbus_tcp_port);

    // 3. Modbus 协议栈初始化
    // 注意：务必确保 portevent.c 中的 xMBPortEventGet/Post 是非阻塞的！
    if (!xMBPortEventInit())
    {
        // 队列创建失败，致命错误
        while (1)
            ;
    }

    eStatus = eMBTCPInit(g_modbus_tcp_port);
    if (eStatus != MB_ENOERR)
        while (1)
            ;
    eStatus = eMBEnable();
    if (eStatus != MB_ENOERR)
        while (1)
            ;
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

        // === 修复点：网络参数热重载处理 ===
        if (g_apply_net_change_flag)
        {
            g_apply_net_change_flag = false;
            wiz_NetInfo net_info;
            uint16_t port;
            
            // 1. 从 EEPROM 读取新参数
            NetConfig_GetEffectiveNetInfo(&net_info, &port);

            // 2. 【关键修复】更新全局端口变量
            // 确保后续 socket() 重连时使用新端口
            if (port != 0) {
                g_modbus_tcp_port = port;
            } else {
                g_modbus_tcp_port = 502;
            }

            // 3. 应用到 W5500 (内部会触发断开)
            Int_ETH_ApplyNetInfo(&net_info, g_modbus_tcp_port); 
            
            MBTCP_PortResetRx();
            // 清端口层残留
            vTaskDelay(SHORT_DELAY);
            // 4. 跳过本次循环，进入状态机重新建立 Socket
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
                    disconnect_start_tick = xTaskGetTickCount();
                if ((xTaskGetTickCount() - disconnect_start_tick) > pdMS_TO_TICKS(1000))
                    if (g_system_state != SYS_STATE_INIT_ERROR)
                        g_system_state = SYS_STATE_RUNNING_NO_COMM;
            }
            else if (g_system_state != SYS_STATE_INIT_ERROR)
            {
                g_system_state = SYS_STATE_RUNNING_NO_COMM;
            }
        }

        switch (sn_sr)
        {
        case SOCK_CLOSED:
            MBTCP_PortResetRx();
            // 保护端口号不为0
            if (g_modbus_tcp_port == 0)
                g_modbus_tcp_port = 502;
            
            // 使用 (刚刚可能已经更新过的) g_modbus_tcp_port 打开 Socket
            if (socket(MODBUS_SOCKET, Sn_MR_TCP, g_modbus_tcp_port, 0x00) != MODBUS_SOCKET)
            {
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            break;
        case SOCK_INIT:
            listen(MODBUS_SOCKET);
            break;
        case SOCK_LISTEN:
            // 监听中，不需要高频轮询，稍微让出CPU
            vTaskDelay(pdMS_TO_TICKS(10));
            break;

        case SOCK_ESTABLISHED:
        {
            g_system_state = SYS_STATE_COMM_ESTABLISHED;
            // === [A] 发送逻辑 (带堵塞保护) ===
            if (g_has_new_data)
            {
                // 获取剩余空间
                uint16_t free_size = getSn_TX_FSR(MODBUS_SOCKET);
                if (free_size >= sizeof(tx_packet_local))
                {
                    // 缓冲区健康，清除堵塞计时
                    tx_jammed_start_tick = 0;
                    // 1. 取数据 (临界区)
                    LOCK_DATA();
                    memcpy(&tx_packet_local, &g_shared_packet_buffer, sizeof(ModbusFakePacket_t));
                    g_has_new_data = false;
                    UNLOCK_DATA();

                    // === 保护措施 2: 发送前再次确认连接状态 ===
                    // 防止刚才那几微秒内连接断开了，导致 send 卡死
                    if (getSn_SR(MODBUS_SOCKET) == SOCK_ESTABLISHED)
                    {
                      
                        send(MODBUS_SOCKET, (uint8_t *)&tx_packet_local, sizeof(tx_packet_local));
                    }
                }
                else
                {
                    // 缓冲区满（说明网络堵了）

                    // === 保护措施 3: 堵塞看门狗 
                    // ===
                    if (tx_jammed_start_tick == 0)
                    {
                        tx_jammed_start_tick = xTaskGetTickCount();
                    }
                    else
                    {
                        // 如果连续堵塞超过 3000ms (3秒)
                        // 说明连接已经死掉了（PC端断开但没发FIN，或者网线拔了）
        
                        if ((xTaskGetTickCount() - tx_jammed_start_tick) > pdMS_TO_TICKS(3000))
                        {
                            // 强制关闭 Socket，触发重连
                            close(MODBUS_SOCKET);
                            tx_jammed_start_tick = 0;
                            // 跳出本次 switch，进入下一次循环处理 CLOSED
                            continue;
                        }
                    }
                }
            }
            else
            {

            }

            if (getSn_RX_RSR(MODBUS_SOCKET) > 0)
            {
                xMBPortEventPost(EV_FRAME_RECEIVED);
            }

            // === [C] 协议栈轮询 ===
            eMBPoll();
            // 调度策略 (配合之前修改的 ADC 优先级高策略)
            // 如果 ADC 优先级高，这里用 yield 或 delay 都可以，yield 响应更快
            taskYIELD();
            break;
        }
        case SOCK_CLOSE_WAIT:
            disconnect(MODBUS_SOCKET);
            // 发送 FIN
            close(MODBUS_SOCKET);
            // 释放资源
            MBTCP_PortResetRx();
            g_system_state = SYS_STATE_RUNNING_NO_COMM;
            break;
        case SOCK_FIN_WAIT:
        case SOCK_CLOSING:
        case SOCK_TIME_WAIT:
        case SOCK_LAST_ACK:
            close(MODBUS_SOCKET);
            MBTCP_PortResetRx();
            g_system_state = SYS_STATE_RUNNING_NO_COMM;
            break;

        default:
            vTaskDelay(pdMS_TO_TICKS(10));
            break;
        }
        if (!g_has_new_data && getSn_RX_RSR(MODBUS_SOCKET) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        else
        {
            taskYIELD();
        }
    }
}
// void Task_Modbus_TCP(void *param)
// {
//     eMBErrorCode eStatus;

//     // 1. 初始化事件队列
//     if (!xMBPortEventInit())
//         while (1)
//             ;

//     // 2. 初始化 Modbus TCP 协议栈

//     eStatus = eMBTCPInit(g_modbus_tcp_port);
//     if (eStatus != MB_ENOERR)
//         while (1)
//             ;

//     eStatus = eMBEnable();
//     if (eStatus != MB_ENOERR)
//         while (1)
//             ;

//     for (;;)
//     {
//         (void)eMBPoll();
//         vTaskDelay(1);
//     }
// }

// void Task_W5500_Manager(void *param)
// {

//     const TickType_t SHORT_DELAY = pdMS_TO_TICKS(10);

//     static uint32_t disconnect_start_tick = 0;

//     static uint32_t cfg_btn_press_start = 0;
//     static bool cfg_btn_processed = false;
//     // 接收队列数据的缓存
//     ModbusFakePacket_t tx_packet;
//     Int_ETH_Init(); // 只在上电/重启时做一次初始化

//     g_system_state = SYS_STATE_RUNNING_NO_COMM;

//     for (;;)
//     {
//         // === CFG 按键长按检测 ===
//         if (HAL_GPIO_ReadPin(CFG_GPIO_Port, CFG_Pin) == GPIO_PIN_RESET)
//         {
//             if (cfg_btn_press_start == 0)
//             {
//                 cfg_btn_press_start = HAL_GetTick();
//             }
//             else
//             {
//                 // 长按超过 1000ms
//                 if ((HAL_GetTick() - cfg_btn_press_start > 1000) && !cfg_btn_processed)
//                 {
//                     // 1. 切换 LED 状态为快闪，提示用户“已触发复位”
//                     g_system_state = SYS_STATE_NET_RESET;

//                     // 2. 阻塞延时 1.5秒，让 LED 闪烁
//                     vTaskDelay(pdMS_TO_TICKS(1500));

//                     // 3. 执行真正的复位操作
//                     Restore_Network_Defaults();

//                     // 4. 标记已处理
//                     cfg_btn_processed = true;
//                 }
//             }
//         }
//         else
//         {
//             // 按键抬起
//             cfg_btn_press_start = 0;
//             cfg_btn_processed = false;
//         }
//         if (g_apply_net_change_flag)
//         {
//             g_apply_net_change_flag = false;
//             wiz_NetInfo net_info;
//             uint16_t port;
//             NetConfig_GetEffectiveNetInfo(&net_info, &port);
//             Int_ETH_ApplyNetInfo(&net_info, port); // 内部会断开/重建
//             MBTCP_PortResetRx();                   // 清端口层残留
//             vTaskDelay(SHORT_DELAY);
//             continue;
//         }
//         uint8_t sn_sr = getSn_SR(MODBUS_SOCKET);
//         if (sn_sr == SOCK_ESTABLISHED)
//         {
//             g_system_state = SYS_STATE_COMM_ESTABLISHED;
//             disconnect_start_tick = 0;
//         }
//         else
//         {

//             if (g_system_state == SYS_STATE_COMM_ESTABLISHED)
//             {
//                 if (disconnect_start_tick == 0)
//                 {
//                     disconnect_start_tick = xTaskGetTickCount();
//                 }

//                 if ((xTaskGetTickCount() - disconnect_start_tick) > pdMS_TO_TICKS(1000))
//                 {
//                     if (g_system_state != SYS_STATE_INIT_ERROR)
//                     {
//                         g_system_state = SYS_STATE_RUNNING_NO_COMM;
//                     }
//                 }
//             }
//             else
//             {
//                 // 如果本来就是红灯闪烁状态，保持即可（除非是INIT_ERROR）
//                 if (g_system_state != SYS_STATE_INIT_ERROR)
//                 {
//                     g_system_state = SYS_STATE_RUNNING_NO_COMM;
//                 }
//             }
//         }
//         switch (sn_sr)
//         {
//         case SOCK_CLOSED:
//             MBTCP_PortResetRx();
//             if (g_modbus_tcp_port == 0)
//                 g_modbus_tcp_port = 502; // 保护

//             // 打开 Socket
//             if (socket(MODBUS_SOCKET, Sn_MR_TCP, g_modbus_tcp_port, 0x00) != MODBUS_SOCKET)
//             {
//                 vTaskDelay(pdMS_TO_TICKS(100)); // 失败退避
//             }
//             break;

//         case SOCK_INIT:
//             // 执行 Listen
//             if (listen(MODBUS_SOCKET) != SOCK_OK)
//             {
//                 vTaskDelay(pdMS_TO_TICKS(50));
//             }
//             else
//             {
//                 vTaskDelay(pdMS_TO_TICKS(10)); // 等待状态翻转
//             }
//             break;

//         case SOCK_LISTEN:
//             // 监听中，定期检查连接请求，稍微延时释放CPU
//             vTaskDelay(pdMS_TO_TICKS(100));
//             break;

//         case SOCK_ESTABLISHED:
//         {

//             // 1. 从队列获取 ADC 数据 (超时 10ms，如果没有数据就继续循环)
//             if (xQueueReceive(g_adc_data_queue, &tx_packet, pdMS_TO_TICKS(10)) == pdTRUE)
//             {
//                 // 2. 检查 W5500 发送缓冲区剩余空间
//                 // 只有当缓冲区空间 > 数据包大小时才发送，防止堵死
//                 if (getSn_TX_FSR(MODBUS_SOCKET) >= sizeof(tx_packet))
//                 {
//                     // 3. 执行发送
//                     // 注意：这里发送的是 raw binary 数据。
//                     // 包含：Head(2) + Float*12(48) + Tail(2) = 52 字节
//                     send(MODBUS_SOCKET, (uint8_t *)&tx_packet, sizeof(tx_packet));
//                 }
//                 else
//                 {
//                     // 缓冲区已满，这里选择丢包（实时性优先），或者你可以做重试逻辑
//                     // 打印调试信息: Buffer Full
//                 }
//             }

//             // 4. 处理接收数据 (保持 Modbus 协议栈兼容性，防止 RX 溢出)
//             uint16_t rsr = getSn_RX_RSR(MODBUS_SOCKET);
//             if (rsr > 0)
//             {
//                 // 如果收到上位机指令（如修改参数），通知 Modbus 任务处理
//                 // 注意：如果只是纯流模式，这里可以只做 recv() 并丢弃，或者解析配置指令
//                 xMBPortEventPost(EV_FRAME_RECEIVED);
//                 vTaskDelay(pdMS_TO_TICKS(2));
//             }
//             break;
//         }

//         case SOCK_CLOSE_WAIT:
//         case SOCK_LAST_ACK:
//         case SOCK_FIN_WAIT:
//         case SOCK_CLOSING:
//         case SOCK_TIME_WAIT:
//             // 收到断开请求，关闭 Socket
//             disconnect(MODBUS_SOCKET);
//             close(MODBUS_SOCKET);
//             MBTCP_PortResetRx();
//             vTaskDelay(pdMS_TO_TICKS(100));
//             break;

//         default:
//             vTaskDelay(pdMS_TO_TICKS(10));
//             break;
//         }
//     }
// }
void Task_ADC_Verify(void *param)
{
    // 1. 硬件初始化
    vTaskDelay(pdMS_TO_TICKS(500));
    CS5530_Init_All();
    CS5530_Start_Continuous();

    uint8_t ready_flags[12];

    // 局部临时包，用于拼装数据
    ModbusFakePacket_t temp_packet;

    // === 预填充固定头部 ===
    temp_packet.trans_id = 0x0000;
    temp_packet.proto_id = 0x0000;
    temp_packet.length = 0x3300; // 51 bytes (Big Endian)
    temp_packet.unit_id = 0x01;
    temp_packet.func_code = 0x04;
    temp_packet.byte_count = 48;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        // 1. 始终读取硬件，维持滤波器
        for (int i = 0; i < 12; i++)
        {
            int32_t raw = CS5530_Read_Data(i, &ready_flags[i]);

            if (ready_flags[i] == 1)
            {
                g_adc_buffer[i] = raw;
            }
            else
            {
                raw = g_adc_buffer[i];
            }

            // 仅在流模式下拼装数据
            if (g_is_streaming_mode)
            {
                int32_t final_val = raw;
                if (g_coil_states[i] == 1)
                    final_val -= g_tare_offset[i];

                int base = i * 4;
                temp_packet.data[base + 0] = (final_val >> 24) & 0xFF;
                temp_packet.data[base + 1] = (final_val >> 16) & 0xFF;
                temp_packet.data[base + 2] = (final_val >> 8) & 0xFF;
                temp_packet.data[base + 3] = (final_val) & 0xFF;
            }
        }

        // 2. 如果已连接且在流模式，直接更新全局数据
        if (g_is_streaming_mode && (getSn_SR(MODBUS_SOCKET) == SOCK_ESTABLISHED))
        {
            // === 关键：进入临界区，快速拷贝 ===
            // 只要几微秒，保证数据完整性
            LOCK_DATA();
            memcpy(&g_shared_packet_buffer, &temp_packet, sizeof(ModbusFakePacket_t));
            g_has_new_data = true; // 告诉网络任务：有最新鲜的数据了
            UNLOCK_DATA();
        }

        // 800Hz 周期控制
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1)); // 或者适当改为 2ms 降低一点压力
    }
}
