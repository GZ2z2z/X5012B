#include "Dri_CS5530.h"
#include "spi.h"

// 外部 SPI 句柄引用
extern SPI_HandleTypeDef hspi3;
#define READ_TIMEOUT_LIMIT  5000

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
    {GPIOD, GPIO_PIN_7},  // ADC 5 
    {GPIOD, GPIO_PIN_8},  // ADC 6
    {GPIOD, GPIO_PIN_9},  // ADC 7
    {GPIOD, GPIO_PIN_10}, // ADC 8
    {GPIOD, GPIO_PIN_11}, // ADC 9
    {GPIOD, GPIO_PIN_12}, // ADC 10
    {GPIOD, GPIO_PIN_13}  // ADC 11
};


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


// 复位串口 
void CS5530_Reset_Serial(void) {
    debug_printf("  > Reset Serial Sequence Start...\r\n");
    for (int i = 0; i < CS5530_COUNT; i++) {

        CS5530_Select(i);
        // 发送 15 个 0xFF (Sync1)
        for (int k = 0; k < 15; k++) {
            SPI_Exchange(CMD_SYNC1); 
        }
        // 发送 1 个 0xFE (Sync0)
        SPI_Exchange(CMD_SYNC0);
        CS5530_Deselect(i);
        
        Soft_Delay(1000); 
    }
}
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

#define CYCLES_PER_MS  40000 

static void Delay_ms(uint32_t ms) {
    Soft_Delay(ms * CYCLES_PER_MS);
}

void CS5530_Init_All(void) 
{
    debug_printf("  > [INIT] Start Sequence...\r\n");

    // 1. 上电等待：给芯片一点时间让晶振起振
    Delay_ms(100);

    for (int i = 0; i < CS5530_COUNT; i++) 
    {
        // === 第一步：串口复位 (Serial Port Initialization) ===
        // 发送 15 个 0xFF 和 1 个 0xFE
        // 作用：无论芯片当前在干什么（比如正在连续转换），
        // 这个序列都能让它退回到等待命令的状态。
        CS5530_Select(i);
        for (int k = 0; k < 15; k++) SPI_Exchange(0xFF);
        SPI_Exchange(0xFE);
        CS5530_Deselect(i);
        
        // 稍微等一下让它反应过来
        Delay_ms(10); 

        // === 第二步：系统复位 (System Reset via RS bit) ===
        // 写入配置寄存器，将 RS 位 (Bit 29) 置 1
        // 0x20000000 对应 RS=1
        CS5530_Write_Config(i, 0x20000000); 
        Delay_ms(50);
        // 再次写入，将 RS 位清零，让芯片开始工作
        // 0x00000000
        CS5530_Write_Config(i, 0x00000000);
        Delay_ms(10);

        // === 第三步：写入实际配置 (800Hz) ===
        // 0x00085000: RS=0, 800Hz
        CS5530_Write_Config(i, 0x00085000);
    }

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
// 读取 ADC 数据 
int32_t CS5530_Read_Data(uint8_t ch_index, uint8_t *new_data_flag) 
{
    uint8_t tx_dummy = 0x00;
    uint8_t rx_buf[4] = {0};
    int32_t result = 0;
    uint32_t timeout_counter = 0;
    
    *new_data_flag = 0;

    // 1. 选中芯片
    CS5530_Select(ch_index);
    
    // 等待 MISO (SDO) 变低，表示数据准备好了
    while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_11) == GPIO_PIN_SET) 
    {
        timeout_counter++;
        // 简单的延时，防止查太快
        for(volatile int k=0; k<10; k++) __NOP(); 
        
        //  超时重置机制 
        if (timeout_counter > READ_TIMEOUT_LIMIT) 
        {
            // 复位
            CS5530_Deselect(ch_index);
            
            // 执行单通道复位
            ADC_Recover_Channel(ch_index); 
            
            return 0; // 直接返回
        }
    }

    
    // 步骤 1: 发送 1 字节 NOP (0x00) 清除 SDO 标志
    if (HAL_SPI_Transmit(&hspi3, &tx_dummy, 1, 10) != HAL_OK) {
        CS5530_Deselect(ch_index);
        return 0;
    }

    // 步骤 2: 连续读取 4 个字节 (High, Mid, Low, Status)
    if (HAL_SPI_Receive(&hspi3, rx_buf, 4, 10) != HAL_OK) {
        CS5530_Deselect(ch_index);
        return 0;
    }

    CS5530_Deselect(ch_index);

    // rx_buf[0] 是高位 (High Byte)
    // rx_buf[1] 是中位 (Mid Byte)
    // rx_buf[2] 是低位 (Low Byte)
    // rx_buf[3] 是状态位 
    // 拼接 24位 有符号数据
    result = ((int32_t)rx_buf[0] << 16) | 
             ((int32_t)rx_buf[1] << 8)  | 
             (int32_t)rx_buf[2];

    // 符号扩展 (24bit -> 32bit)
    if (result & 0x800000) {
        result |= 0xFF000000;
    }

    *new_data_flag = 1;
    return result;
}
// 重启指定通道的 ADC 
void ADC_Recover_Channel(uint8_t ch) {
    // 1. 选中芯片
    CS5530_Select(ch); 
    
    // 2. 发送标准同步序列 (30个FF + 1个FE) 强力清洗
    for(int k=0; k<30; k++) SPI_Exchange(0xFF); 
    SPI_Exchange(0xFE); 
    
    // 3. 取消选中
    CS5530_Deselect(ch);

    Soft_Delay(2000);

    // 4. 重新配置 (0x00085000)
    CS5530_Write_Config(ch, 0x00085000); 

    Soft_Delay(2000);
    
    // 5. 重新启动转换
    CS5530_Select(ch);
    SPI_Exchange(CMD_CONVERT_CONT); // 0xC0
    CS5530_Deselect(ch);
}
