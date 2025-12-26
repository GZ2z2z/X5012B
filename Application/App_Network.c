#include "App_Network.h"

// =============================================================
// 1. 全局变量定义
// =============================================================
NetConfig_t g_net_config;
RuntimeData_t g_runtime_data = {0};
uint8_t g_eth_rx_buf[RX_BUF_SIZE]; // 接收缓冲区

// W5500 互斥量 (防止多任务同时操作寄存器)
static SemaphoreHandle_t g_W5500_Mutex = NULL;

void W5500_Critical_Enter(void)
{
    if (g_W5500_Mutex != NULL)
        xSemaphoreTakeRecursive(g_W5500_Mutex, portMAX_DELAY);
}

void W5500_Critical_Exit(void)
{
    if (g_W5500_Mutex != NULL)
        xSemaphoreGiveRecursive(g_W5500_Mutex);
}

// =============================================================
// 4. 配置加载函数 (合并了原来的 App_NetConfig.c)
// =============================================================
static void App_Load_Net_Config(void)
{
    // === 暂时不读 EEPROM，直接加载默认值 ===
    // 你可以在这里加 if (HAL_GPIO_ReadPin(CFG) == ...) 的逻辑

    uint8_t def_mac[6] = DEFAULT_MAC_ADDR;
    uint8_t def_ip[4] = DEFAULT_IP_ADDR;
    uint8_t def_sn[4] = DEFAULT_SN_MASK;
    uint8_t def_gw[4] = DEFAULT_GW_ADDR;

    // 填充到全局配置结构体
    memcpy(g_net_config.mac, def_mac, 6);
    memcpy(g_net_config.ip, def_ip, 4);
    memcpy(g_net_config.sn, def_sn, 4);
    memcpy(g_net_config.gw, def_gw, 4);
    g_net_config.port = DEFAULT_LOCAL_PORT;

    // 写入 W5500 寄存器
    wiz_NetInfo net_info = {0};
    net_info.dhcp = NETINFO_STATIC;
    memcpy(net_info.mac, g_net_config.mac, 6);
    memcpy(net_info.ip, g_net_config.ip, 4);
    memcpy(net_info.sn, g_net_config.sn, 4);
    memcpy(net_info.gw, g_net_config.gw, 4);

    wizchip_setnetinfo(&net_info);
}

// =============================================================
// 5. 初始化流程
// =============================================================
static void App_W5500_Init(void)
{
    // 创建互斥量
    if (g_W5500_Mutex == NULL)
        g_W5500_Mutex = xSemaphoreCreateRecursiveMutex();

    // 1. 硬件复位
    HAL_GPIO_WritePin(W5500_RST_GPIO_Port, W5500_RST_Pin, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(50));
    HAL_GPIO_WritePin(W5500_RST_GPIO_Port, W5500_RST_Pin, GPIO_PIN_SET);
    vTaskDelay(pdMS_TO_TICKS(200));

    // 2. 注册回调
    // reg_wizchip_cs_cbfunc(W5500_CS_Select, W5500_CS_Deselect);
    // reg_wizchip_spi_cbfunc(W5500_ReadByte, W5500_WriteByte);
    user_reg_function();
    reg_wizchip_cris_cbfunc(W5500_Critical_Enter, W5500_Critical_Exit);

    // 3. 分配内存 (TX:2K, RX:2K)
    uint8_t mem_size[2][8] = {{2, 2, 2, 2, 2, 2, 2, 2}, {2, 2, 2, 2, 2, 2, 2, 2}};
    if (wizchip_init(mem_size[0], mem_size[1]) == -1)
    {
        // 初始化失败：LED_COM 狂闪
        while (1)
        {
            HAL_GPIO_TogglePin(LED_COM_GPIO_Port, LED_COM_Pin);
            vTaskDelay(100);
        }
    }

    // 4. 加载并写入 IP 配置
    App_Load_Net_Config();

    // 5. === 关键：回读校验 ===
    wiz_NetInfo read_back;
    wizchip_getnetinfo(&read_back);

    // 比较 IP 的第1个和最后1个字节
    if (read_back.ip[0] != g_net_config.ip[0] || read_back.ip[3] != g_net_config.ip[3])
    {
        // 校验失败（SPI问题）：LED_COM 慢闪报警
        while (1)
        {
            HAL_GPIO_TogglePin(LED_COM_GPIO_Port, LED_COM_Pin);
            vTaskDelay(500);
        }
    }
}

// =============================================================
// 6. TCP Server 状态机 (Echo)
// =============================================================
void Process_TCP_Server(void)
{
    int32_t len;
    uint8_t sn = SOCK_TCP_SERVER;
    uint16_t port = g_net_config.port;

    switch (getSn_SR(sn))
    {
    case SOCK_ESTABLISHED:
        if (getSn_IR(sn) & Sn_IR_CON)
        {
            setSn_IR(sn, Sn_IR_CON); // 清除中断
            g_runtime_data.is_tcp_connected = 1;
            HAL_GPIO_WritePin(LED_COM_GPIO_Port, LED_COM_Pin, GPIO_PIN_RESET); // 连上亮灯
        }

        // 检查接收数据
        if ((len = getSn_RX_RSR(sn)) > 0)
        {
            if (len > RX_BUF_SIZE)
                len = RX_BUF_SIZE;
            recv(sn, g_eth_rx_buf, len);
            send(sn, g_eth_rx_buf, len); // Echo 回显
        }
        break;

    case SOCK_CLOSE_WAIT:
        disconnect(sn);
        g_runtime_data.is_tcp_connected = 0;
        HAL_GPIO_WritePin(LED_COM_GPIO_Port, LED_COM_Pin, GPIO_PIN_SET); // 断开灭灯
        break;

    case SOCK_INIT:
        listen(sn);
        break;

    case SOCK_CLOSED:
        socket(sn, Sn_MR_TCP, port, 0);
        break;
    }
}

// =============================================================
// 7. UDP Echo 状态机
// =============================================================
void Process_UDP_Echo(void)
{
    int32_t len;
    uint8_t sn = SOCK_UDP_ECHO;
    uint16_t port = DEFAULT_UDP_PORT;
    uint8_t dest_ip[4];
    uint16_t dest_port;

    switch (getSn_SR(sn))
    {
    case SOCK_UDP:
        if ((len = getSn_RX_RSR(sn)) > 0)
        {
            if (len > RX_BUF_SIZE)
                len = RX_BUF_SIZE;
            recvfrom(sn, g_eth_rx_buf, len, dest_ip, &dest_port);
            sendto(sn, g_eth_rx_buf, len, dest_ip, dest_port); // 原路发回
        }
        break;

    case SOCK_CLOSED:
        socket(sn, Sn_MR_UDP, port, 0);
        break;
    }
}

// =============================================================
// 8. 主任务
// =============================================================
void Task_Network(void *param)
{
    // 1. 初始化
    App_W5500_Init();
    Commu_Init(); // 初始化通讯状态
    // 2. 初始化成功指示：LED_ST 闪烁 3 次
    for (int i = 0; i < 3; i++)
    {
        HAL_GPIO_WritePin(LED_ST_GPIO_Port, LED_ST_Pin, GPIO_PIN_RESET);
        vTaskDelay(200);
        HAL_GPIO_WritePin(LED_ST_GPIO_Port, LED_ST_Pin, GPIO_PIN_SET);
        vTaskDelay(200);
    }
    // 保持 LED_ST 亮起，表示任务运行中
    HAL_GPIO_WritePin(LED_ST_GPIO_Port, LED_ST_Pin, GPIO_PIN_RESET);

    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(5); // 5ms 循环周期
    xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        if (wizphy_getphylink() == PHY_LINK_ON)
        {
            g_runtime_data.is_linked = 1;
            HAL_GPIO_WritePin(LED_ST_GPIO_Port, LED_ST_Pin, GPIO_PIN_RESET);

            // 1. 处理 TCP Server 
            Process_TCP_Server();

            // 2.处理下位机通讯协议
            // Commu_UDP_Task_Entry();
            Process_UDP_Echo();

            // 3. 只有在收到 START 指令后，才发送实时数据
            Commu_Send_RealTime_If_Active();
        }
        else
        {
            g_runtime_data.is_linked = 0;
            g_runtime_data.is_tcp_connected = 0;
            HAL_GPIO_WritePin(LED_ST_GPIO_Port, LED_ST_Pin, GPIO_PIN_SET);   // 无网线：灭
            HAL_GPIO_WritePin(LED_COM_GPIO_Port, LED_COM_Pin, GPIO_PIN_SET); // 灭
        }

        // C. 精准延时
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
