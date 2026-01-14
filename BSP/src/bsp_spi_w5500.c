/*
*********************************************************************************************************
* @file    	bsp_spi_w5500.c
* @author  	SY
* @version 	V1.0.0
* @date    	2016-7-10 09:42:53
* @IDE	 	V4.70.0.0
* @Chip    	STM32F107VC
* @brief   	SPI访问W5500源文件
*********************************************************************************************************
* @attention
*		W5500芯片最大支持SPI速度：80MHz
* 
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private Includes
*********************************************************************************************************
*/
#include "bsp.h"
#include "wizchip_conf.h"

/*
*********************************************************************************************************
*                              				Private define
*********************************************************************************************************
*/

#define SPI_W5500					SPI1
#define ENABLE_SPI_RCC() 			do {\
										__HAL_RCC_SPI1_CLK_ENABLE();\
									} while (0)
#define ENABLE_SPI_GPIO_RCC()		do {\
										__HAL_RCC_GPIOA_CLK_ENABLE();\
										__HAL_RCC_GPIOB_CLK_ENABLE();\
									} while (0)
#define SPI_BAUD					SPI_BAUDRATEPRESCALER_2
#define SPI_W5500_AF				GPIO_AF5_SPI1
#define SPI_W5500_RW_TIMEOUT_MS		(5)

/* GPIO端口  */
#define W5500_CS_GPIO				GPIOB			
#define W5500_CS_PIN				GPIO_PIN_10

#define W5500_RESET_GPIO			GPIOB
#define W5500_RESET_PIN				GPIO_PIN_11

#define W5500_MISO_GPIO				GPIOA
#define W5500_MISO_PIN				GPIO_PIN_6

#define W5500_MOSI_GPIO				GPIOA
#define W5500_MOSI_PIN				GPIO_PIN_7

#define W5500_SCK_GPIO				GPIOA
#define W5500_SCK_PIN				GPIO_PIN_5

#define W5500_INT_GPIO				GPIOA
#define W5500_INT_PIN				GPIO_PIN_4


/* 片选口线置低选中  */
#define W5500_CS_LOW()       		HAL_GPIO_WritePin(W5500_CS_GPIO, W5500_CS_PIN, GPIO_PIN_RESET)

/* 片选口线置高不选中 */
#define W5500_CS_HIGH()      		HAL_GPIO_WritePin(W5500_CS_GPIO, W5500_CS_PIN, GPIO_PIN_SET)

/* 复位引脚置低  */
#define W5500_RESET_LOW()       	HAL_GPIO_WritePin(W5500_RESET_GPIO, W5500_RESET_PIN, GPIO_PIN_RESET)

/* 复位引脚置高 */
#define W5500_RESET_HIGH()      	HAL_GPIO_WritePin(W5500_RESET_GPIO, W5500_RESET_PIN, GPIO_PIN_SET)

/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private constants
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private macro
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private function prototypes
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                              				Private functions
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_CfgSPIForW5500
* Description	: 配置SPI硬件
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void bsp_CfgSPIForW5500(SPI_HandleTypeDef *handle)
{
	/* 打开SPI时钟 */
	ENABLE_SPI_RCC();
	
	SPI_InitTypeDef *initHandle = &handle->Init;
	initHandle->Mode = SPI_MODE_MASTER;
	initHandle->Direction = SPI_DIRECTION_2LINES;
	initHandle->DataSize = SPI_DATASIZE_8BIT;
	initHandle->CLKPolarity = SPI_POLARITY_LOW;
	initHandle->CLKPhase = SPI_PHASE_1EDGE;
	initHandle->NSS = SPI_NSS_SOFT;
	initHandle->BaudRatePrescaler = SPI_BAUD;
	initHandle->FirstBit = SPI_FIRSTBIT_MSB;
	initHandle->TIMode = SPI_TIMODE_DISABLE;
	initHandle->CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	initHandle->CRCPolynomial = 7;
	if (HAL_SPI_DeInit(handle) != HAL_OK) {
		BSP_ErrorHandler();
	}
	if (HAL_SPI_Init(handle) != HAL_OK) {
		BSP_ErrorHandler();
	}

	__HAL_SPI_ENABLE(handle);
}

/*
*********************************************************************************************************
* Function Name : bsp_InitSPI_W5500
* Description	: 初始化W5500 SPI接口
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitSPI_W5500(SPI_HandleTypeDef *handle)
{
	/* 使能GPIO 时钟 */
	ENABLE_SPI_GPIO_RCC();
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = W5500_MOSI_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStructure.Alternate = SPI_W5500_AF;		
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(W5500_MOSI_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = W5500_MISO_PIN;
	HAL_GPIO_Init(W5500_MISO_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = W5500_SCK_PIN;
	HAL_GPIO_Init(W5500_SCK_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = W5500_CS_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(W5500_CS_GPIO, &GPIO_InitStructure);
	
	W5500_CS_HIGH();		
	W5500_RESET_HIGH();	

	/* 配置SPI硬件参数用于访问串行Flash */
	bsp_CfgSPIForW5500(handle);
	
	ECHO(DEBUG_BSP_INIT, "[BSP] W5500初始化 .......... OK");
}

/*
*********************************************************************************************************
* Function Name : W5500_WriteByte
* Description	: W5500写数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void W5500_WriteByte(SPI_HandleTypeDef *handle, uint8_t _ucValue)
{
	if (HAL_SPI_Transmit(handle, &_ucValue, sizeof(_ucValue), SPI_W5500_RW_TIMEOUT_MS) != HAL_OK) {
		BSP_ErrorHandler();
	}
}

/*
*********************************************************************************************************
* Function Name : W5500_ReadByte
* Description	: W5500读取数据
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
uint8_t W5500_ReadByte(SPI_HandleTypeDef *handle)
{
	uint8_t rdData = 0;
	if (HAL_SPI_Receive(handle, &rdData, sizeof(rdData), SPI_W5500_RW_TIMEOUT_MS) != HAL_OK) {
		BSP_ErrorHandler();
	}
	return rdData;
}

/*
*********************************************************************************************************
* Function Name : W5500_Set_CS_High
* Description	: W5500拉高片选
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void W5500_Set_CS_High( void )
{
	W5500_CS_HIGH();
}

/*
*********************************************************************************************************
* Function Name : W5500_Set_CS_Low
* Description	: W5500拉低片选
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void W5500_Set_CS_Low( void )
{
	W5500_CS_LOW();
}

/*
*********************************************************************************************************
* Function Name : W5500_SetResetHigh
* Description	: W5500设置复位高电平
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void W5500_SetResetHigh( void )
{
	W5500_RESET_HIGH();
}

/*
*********************************************************************************************************
* Function Name : W5500_SetResetLow
* Description	: W5500设置复位低电平
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void W5500_SetResetLow( void )
{
	W5500_RESET_LOW();
}

/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
