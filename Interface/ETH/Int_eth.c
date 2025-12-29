#include "Int_eth.h"
#include "socket.h"


uint8_t gar[4] = {192, 168, 8, 1};                     // 网关
uint8_t subr[4] = {255, 255, 255, 0};                  // 子网掩码
uint8_t mac[6] = {0xDC, 0x08, 0x96, 0x11, 0x22, 0x33}; // MAC地址
uint16_t g_modbus_tcp_port = MODBUS_PORT;

extern wiz_NetInfo g_cached_net_info; // 引用外部
wiz_NetInfo g_net_info_cache = {0};
static SemaphoreHandle_t g_W5500_Mutex = NULL;

void W5500_Critical_Enter(void)
{
    if (g_W5500_Mutex != NULL)
    {
        xSemaphoreTakeRecursive(g_W5500_Mutex, portMAX_DELAY);
    }
}

void W5500_Critical_Exit(void)
{
    if (g_W5500_Mutex != NULL)
    {
        xSemaphoreGiveRecursive(g_W5500_Mutex);
    }
}

volatile bool g_apply_net_change_flag = false;

/**
 * @brief 硬复位 W5500 芯片
 */
static void Int_ETH_Reset(void)
{
    HAL_GPIO_WritePin(W5500_RST_GPIO_Port, W5500_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(W5500_RST_GPIO_Port, W5500_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(200);
}

void Int_ETH_Init(void)
{
    if (g_W5500_Mutex == NULL)
    {
        g_W5500_Mutex = xSemaphoreCreateRecursiveMutex();
    }
    reg_wizchip_cris_cbfunc(W5500_Critical_Enter, W5500_Critical_Exit);
    Int_ETH_Reset();
    user_reg_function();

    uint8_t tx_size[8] = {8, 8, 0, 0, 0, 0, 0, 0};
    uint8_t rx_size[8] = {8, 8, 0, 0, 0, 0, 0, 0};
    wizchip_init(tx_size, rx_size);

    wiz_NetInfo net_info;
    uint16_t port = MODBUS_PORT;
    NetConfig_GetEffectiveNetInfo(&net_info, &port);

    /* 仅填充 MAC（如你暂不保存 MAC），其余 IP/SN/GW 均以 NetConfig 为准 */
    net_info.mac[0] = mac[0];
    net_info.mac[1] = mac[1];
    net_info.mac[2] = mac[2];
    net_info.mac[3] = mac[3];
    net_info.mac[4] = mac[4];
    net_info.mac[5] = mac[5];

    net_info.dhcp = NETINFO_STATIC;
    setRTR(2000);
    setRCR(3);
    setSn_KPALVTR(MODBUS_SOCKET, 10);
    wizchip_setnetinfo(&net_info);
    wizchip_getnetinfo(&g_net_info_cache);
    g_modbus_tcp_port = port;
}

/* 新增：保存成功后在运行态“立即应用” */
void Int_ETH_ApplyNetInfo(const wiz_NetInfo *pNetInfo, uint16_t port)
{
    if (pNetInfo)
    {
        wizchip_setnetinfo((wiz_NetInfo *)pNetInfo);
        wizchip_getnetinfo(&g_net_info_cache);
    }
    g_modbus_tcp_port = port;

    /* 让 W5500 管理任务用新端口重开监听 */
    if (getSn_SR(MODBUS_SOCKET) != SOCK_CLOSED)
    {
        disconnect(MODBUS_SOCKET);
        close(MODBUS_SOCKET);
    }
}
