#include "App_FreeRTOS.h"

// W5500 Socket管理任务
void Task_Network(void *param);
#define TASK_NETWORK_STACK_SIZE 1024
#define TASK_NETWORK_PRIORITY (tskIDLE_PRIORITY + 2)
TaskHandle_t TASK_NETWORK_handle;

// 网络调试任务
void Task_Debug_Server(void *param);
#define TASK_DEBUG_STACK_SIZE 256
#define TASK_DEBUG_PRIORITY (tskIDLE_PRIORITY + 1)
TaskHandle_t TASK_DEBUG_handle;

// void Task_EEPROM_Test(void *param);
// TaskHandle_t TASK_EEPROM_handle;

void Task_ADC_Verify(void *param);
TaskHandle_t TASK_ADC_VERIFY_handle;

// void Task_ADC_Sample(void *param);
// #define TASK_ADC_SAMPLE_STACK_SIZE 512
// #define TASK_ADC_SAMPLE_PRIORITY (tskIDLE_PRIORITY + 3)
// TaskHandle_t TASK_ADC_SAMPLE_handle;

void App_freeRTOS_Start(void)
{

    xTaskCreate(Task_Network, "Network", TASK_NETWORK_STACK_SIZE, NULL, TASK_NETWORK_PRIORITY, &TASK_NETWORK_handle);

    // xTaskCreate(Task_ADC_Sample, "ADC_Sample", TASK_ADC_SAMPLE_STACK_SIZE, NULL, TASK_ADC_SAMPLE_PRIORITY, &TASK_ADC_SAMPLE_handle);
    xTaskCreate(Task_Debug_Server, "Debug_Server", TASK_DEBUG_STACK_SIZE, NULL, TASK_DEBUG_PRIORITY, &TASK_DEBUG_handle);

    // xTaskCreate(Task_EEPROM_Test, "EEPROM_Test", 512, NULL, tskIDLE_PRIORITY + 1, &TASK_EEPROM_handle);

    // 800Hz 比较快，优先级建议设高一点，防止被网络任务打断太久
    xTaskCreate(Task_ADC_Verify, "ADC_Test", 2048, NULL, tskIDLE_PRIORITY + 1, &TASK_ADC_VERIFY_handle);

    // 启动RTOS调度器
    vTaskStartScheduler();
}

/**
 * @brief 网络调试服务器任务
 * @details 这个任务管理一个TCP Socket，用于接收PC客户端连接，并发送调试信息。
 */
void Task_Debug_Server(void *param)
{
    // 等待网络初始化完成
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1)
    {
        switch (getSn_SR(SOCK_DEBUG))
        {
        case SOCK_CLOSED:
            socket(SOCK_DEBUG, Sn_MR_TCP, DEFAULT_DEBUG_PORT, 0x00);
            break;

        case SOCK_INIT:
            listen(SOCK_DEBUG);
            break;

        case SOCK_LISTEN:
            vTaskDelay(pdMS_TO_TICKS(100));
            break;

        case SOCK_ESTABLISHED:
            vTaskDelay(pdMS_TO_TICKS(500));
            break;

        case SOCK_CLOSE_WAIT:
            disconnect(SOCK_DEBUG);
            break;

        default:
            vTaskDelay(pdMS_TO_TICKS(100));
            break;
        }
    }
}

ADC_Health_t AdcHealth[12] = {0};
void Task_ADC_Verify(void *param)
{
    // 1. 等待网络稳定
    vTaskDelay(pdMS_TO_TICKS(3000));
    debug_printf("\r\n=== ADC 800Hz Data View (With Auto-Recovery) ===\r\n");

    // 2. 初始化
    CS5530_Init_All();

    // 3. 启动连续转换
    CS5530_Start_Continuous();

    // 4. 进入主循环
    int32_t adc_buffer[12] = {0};
    uint8_t ready_flags[12];
    uint32_t print_timer = 0;
    uint32_t print_threshold = 500;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        for(int i=0; i<12; i++) {
            
            // ============================================================
            // [新增] 0. 冷却期检查
            // 如果刚复位过，暂时不要去骚扰它，给 SPI 总线一点喘息时间
            // ============================================================
            // if (AdcHealth[i].cooldown_timer > 0) {
            //     AdcHealth[i].cooldown_timer--;
            //     adc_buffer[i] = 0; // 冷却期间显示 0 (或者 -999 以示区别)
            //     continue; // 直接跳过本次循环，不读数据，不触发复位
            // }

            // 1. 读取数据
            int32_t temp_val = CS5530_Read_Data(i, &ready_flags[i]);
            
            if(ready_flags[i] == 1) {
                // === 读到数据了 (Ready有效) ===
                
                // ============================================================
                // [新增] 1. 黑名单检测 (0 和 65280)
                // 0      = MISO 死锁低电平
                // 65280  = 0x00FF00，典型线路干扰或虚接值
                // ============================================================
                if (temp_val == 0 || temp_val == 65280) {
                     AdcHealth[i].error_count++; // 视为错误累加
                } 
                else {
                     // === 数据看起来是合法的 ===
                     AdcHealth[i].error_count = 0; // 清除错误计数
                     
                     // 2. 冻结检测 (Freeze Check)
                     // 如果数据和上次完全一样，说明芯片内部逻辑可能锁死
                     if (temp_val == AdcHealth[i].last_val) {
                         AdcHealth[i].freeze_count++;
                     } else {
                         AdcHealth[i].freeze_count = 0;
                         AdcHealth[i].last_val = temp_val; // 更新记录
                     }
                     
                     // 3. 溢出检测 (Stuck Check)
                     // 检测是否卡在最大/最小量程
                     if (temp_val == 8388607 || temp_val == -8388608) {
                          AdcHealth[i].stuck_count++;
                     } else {
                          AdcHealth[i].stuck_count = 0;
                     }

                     // 更新显示缓冲区
                     adc_buffer[i] = temp_val;
                }

            } else {
                // === 没读到数据 (Ready 高电平超时) ===
                AdcHealth[i].error_count++;
            }
            
            // ============================================================
            // [终极救护逻辑] 触发条件：
            // 1. 通讯超时/黑名单值 > 50次 (50ms)
            // 2. 数值冻结 > 50次 (50ms 数据完全没变)
            // 3. 数值溢出 > 100次 (插拔瞬间可能短暂溢出，稍微宽容点)
            // ============================================================
            if (AdcHealth[i].error_count > 50 || 
                AdcHealth[i].freeze_count > 50 || 
                AdcHealth[i].stuck_count > 100) 
            {
                // 既然判定死了，就显示 0，方便观察
                adc_buffer[i] = 0;
                
                // 打印调试信息 (可选)
                // debug_printf("Recovering CH%d (Err:%d Frz:%d)...\r\n", i, AdcHealth[i].error_count, AdcHealth[i].freeze_count);

                // 执行暴力复位
                ADC_Recover_Channel(i);
                
                // 清零所有计数器
                AdcHealth[i].error_count = 0;
                AdcHealth[i].freeze_count = 0;
                AdcHealth[i].stuck_count = 0;
                AdcHealth[i].last_val = 0x55AAAA55; // 设个乱码防止误判冻结
                
                // [新增] 设置冷却时间 (例如 2000ms)
                // 复位后如果还没好，让它休息 2秒 再试，防止 SPI 总线被复位命令占满
                // AdcHealth[i].cooldown_timer = 2000; 
            }
        }

        // ============================================================
        // 打印逻辑
        // ============================================================
        print_timer++;
        if (print_timer >= print_threshold) // 500ms 打印一次
        {
            print_timer = 0;
            debug_printf("\r\n--- ADC Data Snapshot ---\r\n");

            debug_printf("Raw[0-5]:  %ld  %ld  %ld  %ld  %ld  %ld\r\n",
                         adc_buffer[0], adc_buffer[1], adc_buffer[2],
                         adc_buffer[3], adc_buffer[4], adc_buffer[5]);

            debug_printf("Raw[6-11]: %ld  %ld  %ld  %ld  %ld  %ld\r\n",
                         adc_buffer[6], adc_buffer[7], adc_buffer[8],
                         adc_buffer[9], adc_buffer[10], adc_buffer[11]);
        }

        // 维持 1ms 轮询
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
    }
}

// /**
//  * @brief EEPROM 读写验证任务
//  */
// void Task_EEPROM_Test(void *param)
// {
//     // ==========================================
//     // [新增] 1. 必须先初始化 I2C GPIO 和总线状态
//     // ==========================================
//     bsp_InitI2C();

//     // 稍微延时让电平稳定
//     vTaskDelay(pdMS_TO_TICKS(50));

//     // 1. 等待网络就绪
//     // 注意：你的 debug_printf 只有在 SOCK_DEBUG 建立连接(ESTABLISHED)后才会发送数据
//     // 所以这里我们需要等待你用电脑连接上 TCP Server (默认端口 5002)
//     vTaskDelay(pdMS_TO_TICKS(2000));

//     // 可选：死等直到调试端口连接上，保证你能看到打印信息
//     // 如果不想死等，可以注释掉下面这个 while 循环
//     while(getSn_SR(SOCK_DEBUG) != SOCK_ESTABLISHED)
//     {
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }

//     debug_printf("\r\n");
//     debug_printf("==================================\r\n");
//     debug_printf("    STM32 FreeRTOS EEPROM Test    \r\n");
//     debug_printf("==================================\r\n");

//     // 2. 检查设备是否存在
//     debug_printf("[Step 1] Checking Device I2C Connection...\r\n");
//     if (ee_CheckOk() == 1)
//     {
//         debug_printf(" -> Device Found (OK)\r\n");
//     }
//     else
//     {
//         debug_printf(" -> Device NOT Found (FAIL) - Check Wiring!\r\n");
//         vTaskDelete(NULL); // 硬件错误，直接结束任务
//     }

//     // 3. 准备测试数据
//     uint16_t test_addr = 0x0010; // 选择一个测试地址
//     uint8_t write_buf[] = "FreeRTOS EEPROM Driver Test 2025";
//     uint8_t read_buf[64] = {0};
//     uint16_t len = strlen((char *)write_buf) + 1; // 包含结束符 \0

//     // 4. 写入测试
//     debug_printf("[Step 2] Writing %d bytes to Addr 0x%04X...\r\n", len, test_addr);
//     debug_printf(" -> Data to write: %s\r\n", write_buf);

//     if (ee_WriteBytes(write_buf, test_addr, len))
//     {
//         debug_printf(" -> Write Operation Success\r\n");
//     }
//     else
//     {
//         debug_printf(" -> Write Operation Failed\r\n");
//     }

//     // 稍作延时，虽然驱动里有轮询等待，但应用层加一点延时更稳妥
//     vTaskDelay(pdMS_TO_TICKS(50));

//     // 5. 读取测试
//     debug_printf("[Step 3] Reading back...\r\n");
//     // 先清空缓冲区，确保读到的是真实的
//     memset(read_buf, 0, sizeof(read_buf));

//     if (ee_ReadBytes(read_buf, test_addr, len))
//     {
//         debug_printf(" -> Read Operation Success\r\n");
//         debug_printf(" -> Read Content: %s\r\n", read_buf);
//     }
//     else
//     {
//         debug_printf(" -> Read Operation Failed\r\n");
//     }

//     // 6. 数据校验
//     debug_printf("[Step 4] Verifying...\r\n");
//     if (memcmp(write_buf, read_buf, len) == 0)
//     {
//         debug_printf(" -> RESULT: PASS (Data Matches Exactly)\r\n");
//     }
//     else
//     {
//         debug_printf(" -> RESULT: FAIL (Data Mismatch)\r\n");
//     }

//     debug_printf("==================================\r\n");
//     debug_printf("Test Finished. Task Deleting...\r\n");

//     // 测试完成，删除自身任务
//     vTaskDelete(NULL);
// }

// /**
//  * @brief  ADC 数据采集任务
//  * @note   运行频率由 ADS1220 的 SPS 设置和 vTaskDelay 共同决定
//  */
// void Task_ADC_Sample(void *param)
// {
//     while (1)
//     {
//         // HAL_GPIO_WritePin(LED_ST_GPIO_Port, LED_ST_Pin, GPIO_PIN_RESET);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }

// }
