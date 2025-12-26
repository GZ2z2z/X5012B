#include "Dri_CS5530.h"
#include "spi.h"
#include "Com_Util.h" // 借用你的延时函数

// 外部 SPI 句柄引用
extern SPI_HandleTypeDef hspi3;

// ============================================================
// 1. 硬件引脚映射 (根据你的描述 PD0-PD4, PD7-PD13)
// ============================================================
typedef struct {
    GPIO_TypeDef* port;
    uint16_t pin;
} CS_Pin_Map_t;

static const CS_Pin_Map_t ADC_CS_PINS[CS5530_COUNT] = {
    {GPIOD, GPIO_PIN_0},  // ADC 0
    {GPIOD, GPIO_PIN_1},  // ADC 1
    {GPIOD, GPIO_PIN_2},  // ADC 2
    {GPIOD, GPIO_PIN_3},  // ADC 3
    {GPIOD, GPIO_PIN_4},  // ADC 4
    {GPIOD, GPIO_PIN_7},  // ADC 5 (注意跳过了 5,6)
    {GPIOD, GPIO_PIN_8},  // ADC 6
    {GPIOD, GPIO_PIN_9},  // ADC 7
    {GPIOD, GPIO_PIN_10}, // ADC 8
    {GPIOD, GPIO_PIN_11}, // ADC 9
    {GPIOD, GPIO_PIN_12}, // ADC 10
    {GPIOD, GPIO_PIN_13}  // ADC 11
};

// ============================================================
// 2. 底层辅助函数
// ============================================================

// 选中指定索引的 ADC (低电平有效)
static void CS5530_Select(uint8_t idx) {
    if (idx < CS5530_COUNT) {
        HAL_GPIO_WritePin(ADC_CS_PINS[idx].port, ADC_CS_PINS[idx].pin, GPIO_PIN_RESET);
    }
}

// 释放指定索引的 ADC (高电平)
static void CS5530_Deselect(uint8_t idx) {
    if (idx < CS5530_COUNT) {
        HAL_GPIO_WritePin(ADC_CS_PINS[idx].port, ADC_CS_PINS[idx].pin, GPIO_PIN_SET);
    }
}

// SPI 读写一个字节
static uint8_t SPI_Exchange(uint8_t data) {
    uint8_t rx_data = 0;
    // 原来是 10ms，改为 1ms (因为我们 1ms 就要跑一次)
    if(HAL_SPI_TransmitReceive(&hspi3, &data, &rx_data, 1, 1) != HAL_OK) {
        return 0xFF; // 发送失败返回 FF
    }
    return rx_data;
}

// 简单的软件延时
static void Soft_Delay(volatile uint32_t count) {
    while(count--) __NOP();
}


// 复位串口 (带打印调试)
void CS5530_Reset_Serial(void) {
    debug_printf("  > Reset Serial Sequence Start...\r\n");
    for (int i = 0; i < CS5530_COUNT; i++) {
        // 打印进度，每处理一个芯片打一次点，防止刷屏
        if(i == 0) debug_printf("    Resetting CH: ");
        debug_printf("%d ", i);
        
        CS5530_Select(i);
        // 发送 15 个 0xFF (Sync1)
        for (int k = 0; k < 15; k++) {
            SPI_Exchange(CMD_SYNC1); 
        }
        // 发送 1 个 0xFE (Sync0)
        SPI_Exchange(CMD_SYNC0);
        CS5530_Deselect(i);
        
        // 替换 Delay_us(10)，改用软延时保证安全
        Soft_Delay(1000); 
    }
    debug_printf("\r\n  > Reset Serial Done.\r\n");
}
// ==========================================
// 修改后的初始化函数 (Dri_CS5530.c)
// ==========================================

// 写入配置寄存器
// CS5530 Config Register: 32-bit
// Cmd: 0x03 (Write Config)
static void CS5530_Write_Config(uint8_t ch_idx, uint32_t cfg_val) {
    CS5530_Select(ch_idx);
    SPI_Exchange(0x03); // Write Config Command
    // 发送 4 字节数据 (MSB First)
    SPI_Exchange((cfg_val >> 24) & 0xFF);
    SPI_Exchange((cfg_val >> 16) & 0xFF);
    SPI_Exchange((cfg_val >> 8)  & 0xFF);
    SPI_Exchange(cfg_val         & 0xFF);
    CS5530_Deselect(ch_idx);
}

// 读取配置寄存器
// Cmd: 0x0B (Read Config)
static uint32_t CS5530_Read_Config(uint8_t ch_idx) {
    uint32_t read_val = 0;
    CS5530_Select(ch_idx);
    SPI_Exchange(0x0B); // Read Config Command
    // 读取 4 字节
    read_val |= ((uint32_t)SPI_Exchange(CMD_NULL) << 24);
    read_val |= ((uint32_t)SPI_Exchange(CMD_NULL) << 16);
    read_val |= ((uint32_t)SPI_Exchange(CMD_NULL) << 8);
    read_val |= ((uint32_t)SPI_Exchange(CMD_NULL));
    CS5530_Deselect(ch_idx);
    return read_val;
}

void CS5530_Init_All(void) {
    debug_printf("  > [INIT] Start Sequence...\r\n");
    
    // 1. 复位串口
    CS5530_Reset_Serial();
    Soft_Delay(10000);

    // 2. 计算 800Hz 配置字
    // RS=0 (System Run)
    // VRS=0 (假设使用 5V 或者是默认 2.5V<VREF<=VA+，如果用 2.5V Ref 且效果不好，可尝试设为 1)
    // FRS=1 (Bit 19, Scale rates by 5/6)
    // WR = 1010 (Bit 14-11, 800Hz @ FRS=1)
    // Val = (1 << 19) | (0xA << 11) = 0x00080000 | 0x00005000 = 0x00085000
    uint32_t config_val = 0x00085000;

    // 3. 自检 (验证通讯是否正常)
    debug_printf("  > [TEST] Writing Config to CH0: 0x%08X\r\n", config_val);
    CS5530_Write_Config(0, config_val);
    Soft_Delay(5000);
    
    uint32_t read_back = CS5530_Read_Config(0);
    debug_printf("  > [TEST] Read Back from CH0:  0x%08X\r\n", read_back);
    
    // 忽略 Bit 28 (RV) 进行比较
    if ((read_back & 0xEFFFFFFF) == (config_val & 0xEFFFFFFF)) {
        debug_printf("  > [PASS] SPI Comm Success! Configured for 800Hz.\r\n");
    } else {
        debug_printf("  > [FAIL] Read mismatch! Check SPI settings.\r\n");
    }

    // 4. 正式对所有芯片写入 800Hz 配置
    debug_printf("  > [INIT] Configuring all 12 chips...\r\n");
    for (int i = 0; i < CS5530_COUNT; i++) {
        CS5530_Write_Config(i, config_val); 
        Soft_Delay(1000);
    }
    
    debug_printf("  > [INIT] Done.\r\n");
}

/**
 * @brief 让所有 ADC 开始连续转换
 */
void CS5530_Start_Continuous(void) {
    for (int i = 0; i < CS5530_COUNT; i++) {
        CS5530_Select(i);
        SPI_Exchange(CMD_CONVERT_CONT); // 发送连续转换命令 0xC0
        CS5530_Deselect(i);
    }
}
// 读取 ADC 数据 (严格遵循 CS5530 的 40 SCLK 时序)
int32_t CS5530_Read_Data(uint8_t ch_index, uint8_t *new_data_flag) {
    uint8_t r[5] = {0}; // 需要5个字节缓冲区
    int32_t result = 0;
    
    *new_data_flag = 0;

    CS5530_Select(ch_index);
    
    Soft_Delay(500);
    
    // 1. 检测 MISO (RDY) 是否变低
    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_11) == GPIO_PIN_RESET) {
        
        // 2. 时序要求：40 SCLKs (5 Bytes)
        // Byte 0: 必须发送 0x00 (NOP) 来清除 SDO 标志
        SPI_Exchange(CMD_NULL); 
        
        // Byte 1-4: 读取 32位 转换数据寄存器
        // [Data 24-bit] + [Status 8-bit]
        r[0] = SPI_Exchange(CMD_NULL); // Data High (MSB)
        r[1] = SPI_Exchange(CMD_NULL); // Data Mid
        r[2] = SPI_Exchange(CMD_NULL); // Data Low (LSB)
        r[3] = SPI_Exchange(CMD_NULL); // Status Byte (包含 OF 溢出位)
        
        // 3. 组装 24位 数据 (丢弃最后的 Status 字节)
        result = ((int32_t)r[0] << 16) | ((int32_t)r[1] << 8) | r[2];
        
        // 4. 符号扩展 (24位转32位有符号整数)
        if (result & 0x800000) {
            result |= 0xFF000000;
        }
        
        *new_data_flag = 1;
    }
    
    CS5530_Deselect(ch_index);
    return result;
}
// 重启指定通道的 ADC (救活死去的芯片)
void ADC_Recover_Channel(uint8_t ch) {
    // 1. 选中芯片
    CS5530_Select(ch); 
    
    // 2. 发送标准同步序列 (30个FF + 1个FE) 强力清洗
    for(int k=0; k<30; k++) SPI_Exchange(0xFF); 
    SPI_Exchange(0xFE); 
    
    // 3. 取消选中
    CS5530_Deselect(ch);
    
    // [优化] 让出 CPU 2ms，让芯片复位，同时允许 Task_Network 运行
    // vTaskDelay(pdMS_TO_TICKS(2));
    Soft_Delay(2000);

    // 4. 重新配置 (0x00085000)
    CS5530_Write_Config(ch, 0x00085000); 
    
    // [优化] 让出 CPU 2ms，等待配置生效
    vTaskDelay(pdMS_TO_TICKS(2)); 
    
    // 5. 重新启动转换
    CS5530_Select(ch);
    SPI_Exchange(CMD_CONVERT_CONT); // 0xC0
    CS5530_Deselect(ch);
}
