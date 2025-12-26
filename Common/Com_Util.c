#include "stm32f4xx.h" // 确保包含F4的头文件
#include "Com_Util.h"

// 初始化DWT计数器（需要在系统初始化时调用一次，比如在main函数开头）
void Delay_Init(void)
{
    // 使能DWT外设
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; 
    // 清空计数器
    DWT->CYCCNT = 0;
    // 使能循环计数器
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void Delay_us(__IO uint32_t delay)
{
    uint32_t startTick = DWT->CYCCNT;
    // 168MHz = 168 cycles per us
    uint32_t waitCycles = delay * 168; 

    // 使用循环等待，直到计数器走过足够的周期
    // 这种写法通过计算差值，自动处理了计数器溢出归零的情况
    while ((DWT->CYCCNT - startTick) < waitCycles);
}

void Delay_ms(uint16_t count) {
    while (count--)
    {
        Delay_us(1000);
    }
}

void Delay_s(uint16_t count) {
    while (count--)
    {
        Delay_ms(1000);
    }
}
