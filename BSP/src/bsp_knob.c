/**
  ******************************************************************************
  * @file    bsp_knob.c
  * @author  SY
  * @version V3.5.0
  * @date    2015-9-21 16:22:56
  * @brief   旋钮采样驱动
  ******************************************************************************
  * @attention
  *	
  * 
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "bsp_knob.h"

/* Private define ------------------------------------------------------------*/
#define		PORT_KNOB_GPIO 		GPIOC
#define		PIN_KNOB_GPIO		GPIO_Pin_2
#define		RCC_KNOB_GPIO		RCC_APB2Periph_GPIOC	

#define 	ADC1_DR_Address    ((uint32_t)0x4001244C)

#define 	KNOB_DMA_CHANNEL	DMA1_Channel1

#define		KNOB_USE_ADC		ADC1

#define		KNOB_ADC_CHANNEL	ADC_Channel_12

/* Private typedef -----------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint16_t ADCConvertedValue;

/* Private function prototypes -----------------------------------------------*/


/* Private functions ---------------------------------------------------------*/

/*------------------------------------------------------------
 * Function Name  : bsp_InitKnob
 * Description    : 旋钮采样初始化
 * Input          : None
 * Output         : None
 * Return         : None
 *------------------------------------------------------------*/
void bsp_InitKnobSample( void )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_InitTypeDef ADC_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;
	
	/* ADCCLK = PCLK2/6 */
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); 
		
	/* Enable peripheral clocks ------------------------------------------------*/
	/* Enable DMA1 clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	/* Enable ADC1 and GPIOC clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_KNOB_GPIO, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = PIN_KNOB_GPIO;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(PORT_KNOB_GPIO, &GPIO_InitStructure);
	
	/* DMA1 channel1 configuration ----------------------------------------------*/
	DMA_DeInit(KNOB_DMA_CHANNEL);
	DMA_InitStructure.DMA_PeripheralBaseAddr = ADC1_DR_Address;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&ADCConvertedValue;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = 1;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(KNOB_DMA_CHANNEL, &DMA_InitStructure);

	/* Enable DMA1 channel1 */
	DMA_Cmd(KNOB_DMA_CHANNEL, ENABLE);

	/* ADC1 configuration ------------------------------------------------------*/
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(KNOB_USE_ADC, &ADC_InitStructure);

	/* ADC1 regular channel14 configuration */ 
	ADC_RegularChannelConfig(KNOB_USE_ADC, KNOB_ADC_CHANNEL, 1, ADC_SampleTime_239Cycles5);

	/* Enable ADC1 DMA */
	ADC_DMACmd(KNOB_USE_ADC, ENABLE);

	/* Enable ADC1 */
	ADC_Cmd(KNOB_USE_ADC, ENABLE);

	/* Enable ADC1 reset calibration register */   
	ADC_ResetCalibration(KNOB_USE_ADC);
	/* Check the end of ADC1 reset calibration register */
	while(ADC_GetResetCalibrationStatus(KNOB_USE_ADC));

	/* Start ADC1 calibration */
	ADC_StartCalibration(KNOB_USE_ADC);
	/* Check the end of ADC1 calibration */
	while(ADC_GetCalibrationStatus(KNOB_USE_ADC));
	 
	/* Start ADC1 Software Conversion */ 
	ADC_SoftwareStartConvCmd(KNOB_USE_ADC, ENABLE);
}

/*------------------------------------------------------------
 * Function Name  : GetKnobValue
 * Description    : 获取旋钮采样值
 * Input          : None
 * Output         : None
 * Return         : None
 *------------------------------------------------------------*/
uint16_t GetKnobValue( void )
{
	return ADCConvertedValue;
}



/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
