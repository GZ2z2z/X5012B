#include "App_Commu.h"


// 接收/发送缓冲区 (复用 app_data.h 定义的大小)
static uint8_t  rx_buf[RX_BUF_SIZE];
static uint8_t  tx_buf[RX_BUF_SIZE];

// 上位机地址信息 (收到包时更新，回复时使用)
static uint8_t  remote_ip[4];
static uint16_t remote_port;

// 状态标志
static uint8_t  g_is_streaming = 0; // 0:静默, 1:允许发送实时数据
static uint32_t g_time_tick = 0;

// =============================================================
// 底层发送函数：封装协议头尾 + CRC
// =============================================================
static void Commu_Reply_Packet(uint8_t index, uint16_t cmd, uint8_t status, uint8_t *body_data, uint16_t body_len)
{
    Frame_Header_t *pHead = (Frame_Header_t *)tx_buf;
    uint8_t *pBody = tx_buf + sizeof(Frame_Header_t);
    
    // 1. 计算 Body 总长度 = CMD(2) + Status(2) + Data(n)
    uint16_t total_body_len = 2 + 2 + body_len;

    // 2. 填充 Header
    pHead->start = FRAME_START;
    pHead->addr_pad = 0x00;
    pHead->index = index;    
    pHead->rsv1 = 0x00;
    pHead->len = total_body_len; 
    pHead->rsv2 = 0x00;
    pHead->token = FRAME_TOKEN;

    // 3. 填充 Body 前部 (Cmd + Status)
    *(uint16_t*)(pBody) = cmd;
    *(uint16_t*)(pBody + 2) = status;

    // 4. 填充 Body 数据部分
    if (body_data != NULL && body_len > 0) {
        memcpy(pBody + 4, body_data, body_len);
    }

    // 5. 计算 CRC16 (Header + Body)

    uint16_t crc_cal = Com_CRC16_Calculate(tx_buf, sizeof(Frame_Header_t) + total_body_len, 0);

    // 6. 填充尾部
    uint8_t *pTail = tx_buf + sizeof(Frame_Header_t) + total_body_len;
    *(uint16_t*)pTail = crc_cal;
    *(pTail + 2) = FRAME_END;

    // 7. 通过 UDP 发送
    uint16_t packet_len = sizeof(Frame_Header_t) + total_body_len + 3; // +CRC(2) + END(1)
    sendto(SOCK_UDP_ECHO, tx_buf, packet_len, remote_ip, remote_port);
}

// =============================================================
// 各指令处理函数 (解析 -> 执行 -> 立即回复)
// =============================================================

// 0x0001: 联机握手
static void Handle_SignOn(uint8_t index)
{
    char *dev_name = "STM32F4-Device";
    // 立即回复设备名称
    Commu_Reply_Packet(index, CMD_SIGN_ON, STATUS_OK, (uint8_t*)dev_name, strlen(dev_name));
    debug_printf("[UDP] SignOn OK\r\n");
}

// 0x0002: 获取版本
static void Handle_Version(uint8_t index)
{
    uint8_t ver_info[20] = {0};
    memcpy(ver_info, "X5012B", 6); // 型号
    *(uint16_t*)(ver_info + 16) = 0x0100;   // 版本 V1.0
    
    Commu_Reply_Packet(index, CMD_DEVICE_VERSION, STATUS_OK, ver_info, 18);
}

// 0x0007: 读参数 (EEPROM)
static void Handle_ReadPrm(uint8_t index, uint8_t *req_data, uint16_t req_len)
{
    if (req_len < 4) {
        Commu_Reply_Packet(index, CMD_READ_PRM, STATUS_ERROR_LEN, NULL, 0);
        return;
    }
    
    uint16_t start_addr = *(uint16_t*)req_data;
    uint16_t read_len   = *(uint16_t*)(req_data + 2);

    if (read_len > 128) read_len = 128; // 保护

    // 临时缓存读取数据
    uint8_t temp_read_buf[132];
    *(uint16_t*)temp_read_buf = start_addr;
    *(uint16_t*)(temp_read_buf + 2) = read_len;

    // 执行读取操作
    if (ee_ReadBytes(temp_read_buf + 4, start_addr, read_len)) {
        // 回复：地址(2) + 长度(2) + 数据(N)
        Commu_Reply_Packet(index, CMD_READ_PRM, STATUS_OK, temp_read_buf, read_len + 4);
    } else {
        Commu_Reply_Packet(index, CMD_READ_PRM, STATUS_ERROR_CMD, NULL, 0); // 读取失败
    }
}

// 0x0008: 写参数 (EEPROM)
static void Handle_WritePrm(uint8_t index, uint8_t *req_data, uint16_t req_len)
{
    if (req_len < 4) return;

    uint16_t start_addr = *(uint16_t*)req_data;
    uint16_t write_len  = *(uint16_t*)(req_data + 2);
    uint8_t *p_data     = req_data + 4;

    if (req_len < (4 + write_len)) return;

    // 执行写入操作
    if (ee_WriteBytes(p_data, start_addr, write_len)) {
        // 回复：地址(2) + 长度(2) (表示写入成功)
        Commu_Reply_Packet(index, CMD_WRITE_PRM, STATUS_OK, req_data, 4);
        debug_printf("[UDP] Write EEPROM Addr:0x%04X Len:%d\r\n", start_addr, write_len);
    } else {
        Commu_Reply_Packet(index, CMD_WRITE_PRM, STATUS_ERROR_WRITE, NULL, 0);
    }
}

// 0x0105: 清零/去皮
static void Handle_SetTare(uint8_t index, uint8_t *req_data, uint16_t req_len)
{
    if (req_len < sizeof(Set_Tare_Request_t)) return;

    Set_Tare_Request_t *req = (Set_Tare_Request_t*)req_data;
    
    // TODO: 这里添加你真实的去皮逻辑，例如设置ADC Offset
    debug_printf("[UDP] Tare Channel %d to %ld\r\n", req->tare_num, req->zero_code);

    // 回复：当前的零点码
    Commu_Reply_Packet(index, CMD_SET_TARE, STATUS_OK, (uint8_t*)&req->zero_code, 4);
}

// 0x0101: 开始采集 (Host请求开始)
static void Handle_Start(uint8_t index)
{
    g_is_streaming = 1; // 标记状态，允许发送任务运行
    Commu_Reply_Packet(index, CMD_SEND_START, STATUS_OK, NULL, 0);
    debug_printf("[UDP] Start Streaming\r\n");
}

// 0x0102: 停止采集
static void Handle_Stop(uint8_t index)
{
    g_is_streaming = 0; // 停止发送
    Commu_Reply_Packet(index, CMD_SEND_STOP, STATUS_OK, NULL, 0);
    debug_printf("[UDP] Stop Streaming\r\n");
}

// =============================================================
// UDP 接收解析任务 (带状态机管理)
// =============================================================
void Commu_UDP_Task_Entry(void)
{
    // [C89标准] 变量声明全部放在函数最开头
    uint8_t sn = SOCK_UDP_ECHO; 
    uint8_t status;
    int32_t recv_len;
    
    // 解析用的临时变量
    Frame_Header_t *pHead;
    uint16_t body_len;
    uint16_t calc_crc;
    uint16_t recv_crc;
    uint8_t *pBody;
    uint16_t cmd;
    uint8_t *pCmdData;
    uint16_t data_len;

    // 1. 获取 Socket 状态
    status = getSn_SR(sn);

    // 2. W5500 Socket 状态机
    switch (status)
    {
        // ---------------------------------------------
        // 状态：Socket 关闭 (初始状态或异常断开后)
        // ---------------------------------------------
        case SOCK_CLOSED:
            // [LED] 端口关闭时，熄灭 COM 灯 (假设高电平灭)
            HAL_GPIO_WritePin(LED_COM_GPIO_Port, LED_COM_Pin, GPIO_PIN_SET);

            // 打开 UDP Socket，绑定端口 COMMU_UDP_PORT (9999)
            if (socket(sn, Sn_MR_UDP, COMMU_UDP_PORT, 0) == sn)
            {
                debug_printf("[UDP] Socket Open Success. Port: %d\r\n", COMMU_UDP_PORT);
            }
            break;

        // ---------------------------------------------
        // 状态：UDP 模式 (正常通信状态)
        // ---------------------------------------------
        case SOCK_UDP:
            // [LED] 端口打开成功，点亮 COM 灯 (假设低电平亮)
            // 注意：这里只是设为亮，如果在接收数据时翻转它，就会产生闪烁效果
            // HAL_GPIO_WritePin(LED_COM_GPIO_Port, LED_COM_Pin, GPIO_PIN_RESET);
            
            // 检查接收缓冲区是否有数据
            recv_len = getSn_RX_RSR(sn);
            
            if (recv_len > 0)
            {
                // [LED] 收到数据时，翻转 COM 灯 (产生闪烁效果)
                HAL_GPIO_TogglePin(LED_COM_GPIO_Port, LED_COM_Pin);

                if (recv_len > RX_BUF_SIZE) recv_len = RX_BUF_SIZE;

                // 接收数据
                recvfrom(sn, rx_buf, recv_len, remote_ip, &remote_port);

                // --- 数据包解析逻辑 (保持不变) ---
                if (recv_len < sizeof(Frame_Header_t) + 3) return; 

                pHead = (Frame_Header_t *)rx_buf;

                if (pHead->start != FRAME_START || pHead->token != FRAME_TOKEN) return;

                body_len = pHead->len;
                if (recv_len < (sizeof(Frame_Header_t) + body_len + 3)) return; 

                calc_crc = Com_CRC16_Calculate(rx_buf, sizeof(Frame_Header_t) + body_len, 0);
                recv_crc = *(uint16_t*)(rx_buf + sizeof(Frame_Header_t) + body_len);

                if (calc_crc != recv_crc) 
                {
                    debug_printf("[UDP] CRC Fail\r\n");
                    Commu_Reply_Packet(pHead->index, 0x0000, STATUS_ERROR_CRC, NULL, 0);
                    return;
                }

                pBody = rx_buf + sizeof(Frame_Header_t);
                cmd = *(uint16_t*)pBody; 
                pCmdData = pBody + 2; 
                data_len = body_len - 2;

                switch (cmd) 
                {
                    case CMD_SIGN_ON:       Handle_SignOn(pHead->index); break;
                    case CMD_DEVICE_VERSION:Handle_Version(pHead->index); break;
                    case CMD_READ_PRM:      Handle_ReadPrm(pHead->index, pCmdData, data_len); break;
                    case CMD_WRITE_PRM:     Handle_WritePrm(pHead->index, pCmdData, data_len); break;
                    case CMD_SET_TARE:      Handle_SetTare(pHead->index, pCmdData, data_len); break;
                    case CMD_SEND_START:    Handle_Start(pHead->index); break;
                    case CMD_SEND_STOP:     Handle_Stop(pHead->index); break;
                    default:
                        Commu_Reply_Packet(pHead->index, cmd, STATUS_ERROR_CMD, NULL, 0);
                        break;
                }
            }
            else
            {
                // [LED] 没有数据时，保持 COM 灯常亮，表示 Socket 正常在线
                // 如果你喜欢平时灭、有数据亮，就把这里改成 SET
                HAL_GPIO_WritePin(LED_COM_GPIO_Port, LED_COM_Pin, GPIO_PIN_RESET);
            }
            break;

        // ---------------------------------------------
        // 状态：异常
        // ---------------------------------------------
        default:
            close(sn);
            // [LED] 异常状态灭灯
            HAL_GPIO_WritePin(LED_COM_GPIO_Port, LED_COM_Pin, GPIO_PIN_SET);
            break;
    }
}

// =============================================================
// 实时数据发送 (只在上位机开启后发送)
// =============================================================
void Commu_Send_RealTime_If_Active(void)
{
    // 如果上位机没发 CMD_SEND_START，这里直接返回，不主动发送
    if (g_is_streaming == 0) return;

    // 限制发送频率 (例如 20ms 一次 = 50Hz)
    if ((xTaskGetTickCount() - g_time_tick) < 20) return;
    g_time_tick = xTaskGetTickCount();

    Frame_RealTime_t frame = {0};
    frame.cmd = CMD_REAL_TIME_DATA;
    frame.status = 0x0001; // 运行状态
    frame.time_base = g_time_tick;
    
    // TODO: 这里填入真实的 ADC 数据
    // frame.smpl[0] = g_adc_value[0]; 
    for(int i=0; i<12; i++) frame.smpl[i] = i * 100; // 模拟数据

    // 发送 (实时数据不需要应答，Index 通常固定或自增，这里用 0x80)
    // 注意：Commu_Reply_Packet 会自动加 Header/CRC，所以只传 Body 部分 (去掉 CMD/Status)
    Commu_Reply_Packet(0x80, CMD_REAL_TIME_DATA, 0x00, (uint8_t*)&frame.input, sizeof(Frame_RealTime_t)-4);
}

void Commu_Init(void)
{
    g_is_streaming = 0;
}
