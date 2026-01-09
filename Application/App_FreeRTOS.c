#include "App_FreeRTOS.h"

void Task_ADC_Verify(void *param);
TaskHandle_t TASK_ADC_VERIFY_handle;


void Task_Network_Core(void *param);
#define TASK_NET_CORE_STACK_SIZE 2048                
#define TASK_NET_CORE_PRIORITY (tskIDLE_PRIORITY + 4)
TaskHandle_t TASK_NET_CORE_handle;

void Task_LED_Entry(void *param);
#define TASK_LED_STACK_SIZE 128
#define TASK_LED_PRIORITY (tskIDLE_PRIORITY + 4)
TaskHandle_t TASK_LED_handle;


ModbusFakePacket_t g_shared_packet_buffer; 
volatile bool g_has_new_data = false;      

// 引入临界区保护，防止写一半被读走
#define LOCK_DATA() taskENTER_CRITICAL()
#define UNLOCK_DATA() taskEXIT_CRITICAL()

int32_t g_adc_buffer[12] = {0};
extern volatile bool g_is_streaming_mode;
extern volatile bool g_apply_net_change_flag;
extern uint16_t g_modbus_tcp_port;
extern void MBTCP_PortResetRx(void);


void App_freeRTOS_Start(void)
{

    Int_EEPROM_Init();
    Calib_Init();

    xTaskCreate(Task_Network_Core, "Net_Core", TASK_NET_CORE_STACK_SIZE, NULL, TASK_NET_CORE_PRIORITY, &TASK_NET_CORE_handle);

    xTaskCreate(Task_ADC_Verify, "ADC_Test", 2048, NULL, tskIDLE_PRIORITY + 5, &TASK_ADC_VERIFY_handle);

    xTaskCreate(Task_LED_Entry, "LED_Task", TASK_LED_STACK_SIZE, NULL, TASK_LED_PRIORITY, &TASK_LED_handle);


    // 启动RTOS调度器
    vTaskStartScheduler();
}


static void Restore_Network_Defaults(void)
{
    // 1. 生成默认参数
    wiz_NetInfo default_net = {
        .ip = {192, 168, 0, 234},
        .sn = {255, 255, 255, 0},
        .gw = {192, 168, 0, 1},
        .mac = {0xDC, 0x08, 0x96, 0x11, 0x22, 0x33},
        .dhcp = NETINFO_STATIC};
    uint16_t default_port = 502;

    // 2. 写入 EEPROM
    NetConfig_SaveParams(default_net.ip, default_net.sn, default_net.gw, default_net.mac, default_port);

    // 3. 触发热重载
    g_apply_net_change_flag = true;

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
    if (!xMBPortEventInit())
    {
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

        if (g_apply_net_change_flag)
        {
            g_apply_net_change_flag = false;
            wiz_NetInfo net_info;
            uint16_t port;
            
            // 1. 从 EEPROM 读取新参数
            NetConfig_GetEffectiveNetInfo(&net_info, &port);

            // 2. 更新全局端口变量
            if (port != 0) {
                g_modbus_tcp_port = port;
            } else {
                g_modbus_tcp_port = 502;
            }

            // 3. 应用到 W5500 
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
                    if (getSn_SR(MODBUS_SOCKET) == SOCK_ESTABLISHED)
                    {
                      
                        send(MODBUS_SOCKET, (uint8_t *)&tx_packet_local, sizeof(tx_packet_local));
                    }
                }
                else
                {
                    if (tx_jammed_start_tick == 0)
                    {
                        tx_jammed_start_tick = xTaskGetTickCount();
                    }
                    else
                    {
                        if ((xTaskGetTickCount() - tx_jammed_start_tick) > pdMS_TO_TICKS(3000))
                        {
                            close(MODBUS_SOCKET);
                            tx_jammed_start_tick = 0;
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

            eMBPoll();
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
    temp_packet.length = 0x3300;
    temp_packet.unit_id = 0x01;
    temp_packet.func_code = 0x04;
    temp_packet.byte_count = 48;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        // 1. 始终读取硬件，维持滤波器
        for (int i = 0; i < 12; i++)
        {
            // 读取绝对原始码
            int32_t raw = CS5530_Read_Data(i, &ready_flags[i]);
            
            if (ready_flags[i] == 1) {
                Calib_Process(i, raw);
            }

            if (g_is_streaming_mode)
            {
                int32_t val_to_send = g_RuntimeData[i].rel_raw;
                
                int base = i * 4;
                temp_packet.data[base + 0] = (val_to_send >> 24) & 0xFF;
                temp_packet.data[base + 1] = (val_to_send >> 16) & 0xFF;
                temp_packet.data[base + 2] = (val_to_send >> 8) & 0xFF;
                temp_packet.data[base + 3] = (val_to_send) & 0xFF;
            }
        }

        // 2. 如果已连接且在流模式，更新共享数据给网络任务
        if (g_is_streaming_mode && (getSn_SR(MODBUS_SOCKET) == SOCK_ESTABLISHED))
        {
            LOCK_DATA();
            memcpy(&g_shared_packet_buffer, &temp_packet, sizeof(ModbusFakePacket_t));
            g_has_new_data = true; 
            UNLOCK_DATA();
        }

        // 800Hz 周期控制
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
    }
}
  