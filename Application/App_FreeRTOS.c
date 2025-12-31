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
#define TASK_MODBUS_PRIORITY (tskIDLE_PRIORITY + 5)
TaskHandle_t TASK_MODBUS_handle;

// W5500 Socket管理任务
void Task_W5500_Manager(void *param);
#define TASK_W5500_STACK_SIZE 512
#define TASK_W5500_PRIORITY (tskIDLE_PRIORITY + 3)
TaskHandle_t TASK_W5500_handle;

void Task_LED_Entry(void *param);
#define TASK_LED_STACK_SIZE 128
#define TASK_LED_PRIORITY (tskIDLE_PRIORITY + 3)
TaskHandle_t TASK_LED_handle;

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

    xTaskCreate(Task_Modbus_TCP, "Modbus_TCP", TASK_MODBUS_STACK_SIZE, NULL, TASK_MODBUS_PRIORITY, &TASK_MODBUS_handle);

    xTaskCreate(Task_W5500_Manager, "W5500_Manager", TASK_W5500_STACK_SIZE, NULL, TASK_W5500_PRIORITY, &TASK_W5500_handle);
    // 800Hz 比较快，优先级建议设高一点，防止被网络任务打断太久
    xTaskCreate(Task_ADC_Verify, "ADC_Test", 2048, NULL, tskIDLE_PRIORITY + 3, &TASK_ADC_VERIFY_handle);

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

    const TickType_t SHORT_DELAY = pdMS_TO_TICKS(10);

    static uint32_t disconnect_start_tick = 0;

    static uint32_t cfg_btn_press_start = 0;
    static bool cfg_btn_processed = false;
    // 接收队列数据的缓存
    ModbusFakePacket_t tx_packet;
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
            MBTCP_PortResetRx();
            if (g_modbus_tcp_port == 0)
                g_modbus_tcp_port = 502; // 保护

            // 打开 Socket
            if (socket(MODBUS_SOCKET, Sn_MR_TCP, g_modbus_tcp_port, 0x00) != MODBUS_SOCKET)
            {
                vTaskDelay(pdMS_TO_TICKS(100)); // 失败退避
            }
            break;

        case SOCK_INIT:
            // 执行 Listen
            if (listen(MODBUS_SOCKET) != SOCK_OK)
            {
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else
            {
                vTaskDelay(pdMS_TO_TICKS(10)); // 等待状态翻转
            }
            break;

        case SOCK_LISTEN:
            // 监听中，定期检查连接请求，稍微延时释放CPU
            vTaskDelay(pdMS_TO_TICKS(100));
            break;

        case SOCK_ESTABLISHED:
        {

            // 1. 从队列获取 ADC 数据 (超时 10ms，如果没有数据就继续循环)
            if (xQueueReceive(g_adc_data_queue, &tx_packet, pdMS_TO_TICKS(10)) == pdTRUE)
            {
                // 2. 检查 W5500 发送缓冲区剩余空间
                // 只有当缓冲区空间 > 数据包大小时才发送，防止堵死
                if (getSn_TX_FSR(MODBUS_SOCKET) >= sizeof(tx_packet))
                {
                    // 3. 执行发送
                    // 注意：这里发送的是 raw binary 数据。
                    // 包含：Head(2) + Float*12(48) + Tail(2) = 52 字节
                    send(MODBUS_SOCKET, (uint8_t *)&tx_packet, sizeof(tx_packet));
                }
                else
                {
                    // 缓冲区已满，这里选择丢包（实时性优先），或者你可以做重试逻辑
                    // 打印调试信息: Buffer Full
                }
            }

            // 4. 处理接收数据 (保持 Modbus 协议栈兼容性，防止 RX 溢出)
            uint16_t rsr = getSn_RX_RSR(MODBUS_SOCKET);
            if (rsr > 0)
            {
                // 如果收到上位机指令（如修改参数），通知 Modbus 任务处理
                // 注意：如果只是纯流模式，这里可以只做 recv() 并丢弃，或者解析配置指令
                xMBPortEventPost(EV_FRAME_RECEIVED);
                vTaskDelay(pdMS_TO_TICKS(2));
            }
            break;
        }

        case SOCK_CLOSE_WAIT:
        case SOCK_LAST_ACK:
        case SOCK_FIN_WAIT:
        case SOCK_CLOSING:
        case SOCK_TIME_WAIT:
            // 收到断开请求，关闭 Socket
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

void Task_ADC_Verify(void *param)
{
    // 1. 初始化
    vTaskDelay(pdMS_TO_TICKS(500));
    CS5530_Init_All();
    CS5530_Start_Continuous();

    uint8_t ready_flags[12];
    ModbusFakePacket_t packet;

    // === 2. 预填充固定头部 ===
    packet.trans_id   = 0x0000; // 由发送任务填充
    packet.proto_id   = 0x0000;
    packet.length     = 0x3300; // 0x0033 (51 bytes) 的网络序
    packet.unit_id    = 0x01;
    packet.func_code  = 0x04;   // 模拟读输入寄存器响应
    packet.byte_count = 48;     // 12通道 * 4字节 = 48字节

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        // === 3. 连接状态检查 ===
        // 只有当 Modbus 端口处于连接状态时，才进行高频采样和发送
        // 如果断开连接，则降低频率轮询，等待重连
        if (getSn_SR(MODBUS_SOCKET) != SOCK_ESTABLISHED)
        {
            // vTaskDelay(pdMS_TO_TICKS(200)); 
            continue; 
        }

        // 1. 必须始终读取硬件，保持滤波器稳定
        for (int i = 0; i < 12; i++)
        {
            int32_t raw = CS5530_Read_Data(i, &ready_flags[i]);
            
            // 2. 无论什么模式，都更新全局缓存 g_adc_buffer
            // 这样 eMBRegInputCB 随时能取到最新值
            if(ready_flags[i] == 1) {
                g_adc_buffer[i] = raw;
            } else {
                raw = g_adc_buffer[i]; // 没准备好就用旧值
            }

            // 3. 只有在流模式下，才准备发送包
            if (g_is_streaming_mode)
            {
                int32_t final_val = raw;
                // 去皮
                if (g_coil_states[i] == 1) final_val -= g_tare_offset[i];
                
                // 填包 (Big Endian)
                int base = i*4;
                packet.data[base+0] = (final_val >> 24) & 0xFF;
                packet.data[base+1] = (final_val >> 16) & 0xFF;
                packet.data[base+2] = (final_val >> 8)  & 0xFF;
                packet.data[base+3] = (final_val)       & 0xFF;
            }
        }

        // 4. 门控发送：只有开关打开，才入队
        if (g_is_streaming_mode)
        {
            xQueueSend(g_adc_data_queue, &packet, 0);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
    }
}
