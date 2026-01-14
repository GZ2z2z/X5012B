/*
*********************************************************************************************************
* @file    	bsp_cs553x.c
* @author  	SY
* @version 	V1.0.5
* @date    	2020-5-30 10:09:23
* @IDE	 	Keil V4.72.10.0
* @Chip    	STM32F407VE
* @brief   	CS553X ADCоƬ����Դ�ļ�
*********************************************************************************************************
* @attention
*
* ---------------------------------------------------------
* �汾��V1.0.0 	�޸��ˣ�SY	�޸����ڣ�2018-4-10 09:10:16	
* 
* 1. ������ʼ�汾
* -------------------------------------------------------------------------------------------------------
*
* ---------------------------------------------------------
* �汾��V1.0.1 	�޸��ˣ�SY	�޸����ڣ�2018-4-18 09:11:35	
* 
* 1. ����Ӳ��SPI��д��ʽ
* 2. ���Ӷ�ȡ��ʱʱ��
* 3. ����CS5530ת������
* -------------------------------------------------------------------------------------------------------
*
* ---------------------------------------------------------
* �汾��V1.0.2 	�޸��ˣ�SY	�޸����ڣ�2018-5-2 14:15:38
* 
* 1. ����ADC_ReadConv() ADC��ѹ�Ƚ�����
* 2. �����ṹ����ʾ��ʽ�������豸���
* 3. �������ȳ�ʼ������ͨ����GPIO����Ƭѡ���ߡ�
* 4. CS5530/CS5532���SPI�ٶ�Ϊ2MHz���������������ֵ�����ϴ�������λ��
* -------------------------------------------------------------------------------------------------------
*
* ---------------------------------------------------------
* �汾��V1.0.3 	�޸��ˣ�SY	�޸����ڣ�2018-5-12 10:10:17
* 
* 1. ���Ӵ��������Ӽ��
* 2. ���ٲ���ʱ����ֵ�����ϴ����Ӳ����˲�����
* -------------------------------------------------------------------------------------------------------
*
* ---------------------------------------------------------
* �汾��V1.0.4 	�޸��ˣ�SY	�޸����ڣ�2018-5-21 19:33:31
* 
* 1. ���ⲿ��ADC�������Ƹ�Ϊ������ʱ�����ܴ���ƫ��У׼�����򣬵�δ���봫��������ʱ����ȡ����ֵ����ȷ��
* -------------------------------------------------------------------------------------------------------
*
* ---------------------------------------------------------
* �汾��V1.0.5 	�޸��ˣ�SY	�޸����ڣ�2020-5-30 10:09:23
* 
* 1. �Զ�ʶ��CS5530/CS5532оƬ��������оƬҲ��������ʹ�ã��м�·Ƭѡ�źž�Ҫʹ�ܼ�·ͨ����
* -------------------------------------------------------------------------------------------------------
*
*
* 
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                              				Private Includes
*********************************************************************************************************
*/
#include "bsp.h"
#include "delay.h"

/*
*********************************************************************************************************
*                              				Private define
*********************************************************************************************************
*/

/* ��ʱ*/
#define bsp_DelayUS(x)						delay_us(x)
#define bsp_DelayMS(x)						delay_ms(x)

#ifndef htonl
#define htonl(x)    __REV(x)
#endif

/* 12通道adc */
#define ADC1_EN								(true)
#define ADC1_CHIP							(true)							
#define ADC1_TYPE							(TYPE_AUTO)

#define ADC2_EN								(true)
#define ADC2_CHIP							(true)
#define ADC2_TYPE							(TYPE_AUTO)

#define ADC3_EN								(true)
#define ADC3_CHIP							(true)
#define ADC3_TYPE							(TYPE_AUTO)

#define ADC4_EN								(true)
#define ADC4_CHIP							(true)
#define ADC4_TYPE							(TYPE_AUTO)

#define ADC5_EN								(true)
#define ADC5_CHIP							(true)
#define ADC5_TYPE							(TYPE_AUTO)

#define ADC6_EN								(true)
#define ADC6_CHIP							(true)
#define ADC6_TYPE							(TYPE_AUTO)

#define ADC7_EN								(true)
#define ADC7_CHIP							(true)
#define ADC7_TYPE							(TYPE_AUTO)

#define ADC8_EN								(true)
#define ADC8_CHIP							(true)
#define ADC8_TYPE							(TYPE_AUTO)

#define ADC9_EN								(true)
#define ADC9_CHIP							(true)
#define ADC9_TYPE							(TYPE_AUTO)

#define ADC10_EN							(true)
#define ADC10_CHIP							(true)
#define ADC10_TYPE							(TYPE_AUTO)

#define ADC11_EN							(true)
#define ADC11_CHIP							(true)
#define ADC11_TYPE							(TYPE_AUTO)

#define ADC12_EN							(true)
#define ADC12_CHIP							(true)
#define ADC12_TYPE							(TYPE_AUTO)



/* ʹ��Ӳ��SPI */
#define CS553x_USE_HARDWARE_SPI				(true)
#define SPI_Dev								hspi3

/* ���� */
#define MAX_READ_TIMEOUT_US					(50*1000)	//����ȡ���ݳ�ʱ
#define ADC_SAMPLE_FREQ						(RATE_50)	//����Ƶ��
#define DEFAULT_SEND_FREQ					(CTRL_FREQ)
#define WIRE_TYPE							(FourWire)
#define DEFAULT_SETUP						(SETUP_2)

#define ENABLE_ADC_GPIO_RCC()				do {\
												__HAL_RCC_GPIOC_CLK_ENABLE();\
												__HAL_RCC_GPIOD_CLK_ENABLE();\
											} while (0)

#define	ADC_CS1_GPIO_PORT					GPIOD
#define	ADC_CS1_GPIO_PIN					GPIO_PIN_0
#define	ADC_CS2_GPIO_PORT					GPIOD
#define	ADC_CS2_GPIO_PIN					GPIO_PIN_1
#define	ADC_CS3_GPIO_PORT					GPIOD
#define	ADC_CS3_GPIO_PIN					GPIO_PIN_2	
#define	ADC_CS4_GPIO_PORT					GPIOD
#define	ADC_CS4_GPIO_PIN					GPIO_PIN_3
#define	ADC_CS5_GPIO_PORT					GPIOD
#define	ADC_CS5_GPIO_PIN					GPIO_PIN_4
#define	ADC_CS6_GPIO_PORT					GPIOD
#define	ADC_CS6_GPIO_PIN					GPIO_PIN_7
#define	ADC_CS7_GPIO_PORT					GPIOD
#define	ADC_CS7_GPIO_PIN					GPIO_PIN_8
#define	ADC_CS8_GPIO_PORT					GPIOD
#define	ADC_CS8_GPIO_PIN					GPIO_PIN_9
#define	ADC_CS9_GPIO_PORT					GPIOD
#define	ADC_CS9_GPIO_PIN					GPIO_PIN_10
#define	ADC_CS10_GPIO_PORT					GPIOD
#define	ADC_CS10_GPIO_PIN					GPIO_PIN_11
#define	ADC_CS11_GPIO_PORT					GPIOD
#define	ADC_CS11_GPIO_PIN					GPIO_PIN_12
#define	ADC_CS12_GPIO_PORT					GPIOD
#define	ADC_CS12_GPIO_PIN					GPIO_PIN_13



#define	ADC_MISO_GPIO_PORT					GPIOC
#define	ADC_MISO_GPIO_PIN					GPIO_PIN_11
#define	ADC_MOSI_GPIO_PORT					GPIOC
#define	ADC_MOSI_GPIO_PIN					GPIO_PIN_12
#define	ADC_SCK_GPIO_PORT					GPIOC
#define	ADC_SCK_GPIO_PIN					GPIO_PIN_10											

#define	SET_ADC_MOSI()		 				HAL_GPIO_WritePin(ADC_MOSI_GPIO_PORT, ADC_MOSI_GPIO_PIN, GPIO_PIN_SET) 
#define	CLR_ADC_MOSI()		 				HAL_GPIO_WritePin(ADC_MOSI_GPIO_PORT, ADC_MOSI_GPIO_PIN, GPIO_PIN_RESET)
#define	SET_ADC_SCK()		 				HAL_GPIO_WritePin(ADC_SCK_GPIO_PORT, ADC_SCK_GPIO_PIN, GPIO_PIN_SET)	
#define	CLR_ADC_SCK()		 				HAL_GPIO_WritePin(ADC_SCK_GPIO_PORT, ADC_SCK_GPIO_PIN, GPIO_PIN_RESET)
#define	READ_ADC_MISO()						(HAL_GPIO_ReadPin(ADC_MISO_GPIO_PORT, ADC_MISO_GPIO_PIN))
#define	SET_ADC_CS1()	 					HAL_GPIO_WritePin(ADC_CS1_GPIO_PORT, ADC_CS1_GPIO_PIN, GPIO_PIN_SET)
#define	CLR_ADC_CS1()	 					HAL_GPIO_WritePin(ADC_CS1_GPIO_PORT, ADC_CS1_GPIO_PIN, GPIO_PIN_RESET)
#define	SET_ADC_CS2()	 					HAL_GPIO_WritePin(ADC_CS2_GPIO_PORT, ADC_CS2_GPIO_PIN, GPIO_PIN_SET)		
#define	CLR_ADC_CS2()	 					HAL_GPIO_WritePin(ADC_CS2_GPIO_PORT, ADC_CS2_GPIO_PIN, GPIO_PIN_RESET)
#define	SET_ADC_CS3()	 					HAL_GPIO_WritePin(ADC_CS3_GPIO_PORT, ADC_CS3_GPIO_PIN, GPIO_PIN_SET)			
#define	CLR_ADC_CS3()	 					HAL_GPIO_WritePin(ADC_CS3_GPIO_PORT, ADC_CS3_GPIO_PIN, GPIO_PIN_RESET)
#define	SET_ADC_CS4()	 					HAL_GPIO_WritePin(ADC_CS4_GPIO_PORT, ADC_CS4_GPIO_PIN, GPIO_PIN_SET)			
#define	CLR_ADC_CS4()	 					HAL_GPIO_WritePin(ADC_CS4_GPIO_PORT, ADC_CS4_GPIO_PIN, GPIO_PIN_RESET)
#define	SET_ADC_CS5()	 					HAL_GPIO_WritePin(ADC_CS5_GPIO_PORT, ADC_CS5_GPIO_PIN, GPIO_PIN_SET)			
#define	CLR_ADC_CS5()	 					HAL_GPIO_WritePin(ADC_CS5_GPIO_PORT, ADC_CS5_GPIO_PIN, GPIO_PIN_RESET)
#define	SET_ADC_CS6()	 					HAL_GPIO_WritePin(ADC_CS6_GPIO_PORT, ADC_CS6_GPIO_PIN, GPIO_PIN_SET)			
#define	CLR_ADC_CS6()	 					HAL_GPIO_WritePin(ADC_CS6_GPIO_PORT, ADC_CS6_GPIO_PIN, GPIO_PIN_RESET)
#define	SET_ADC_CS7()	 					HAL_GPIO_WritePin(ADC_CS7_GPIO_PORT, ADC_CS7_GPIO_PIN, GPIO_PIN_SET)			
#define	CLR_ADC_CS7()	 					HAL_GPIO_WritePin(ADC_CS7_GPIO_PORT, ADC_CS7_GPIO_PIN, GPIO_PIN_RESET)
#define	SET_ADC_CS8()	 					HAL_GPIO_WritePin(ADC_CS8_GPIO_PORT, ADC_CS8_GPIO_PIN, GPIO_PIN_SET)			
#define	CLR_ADC_CS8()	 					HAL_GPIO_WritePin(ADC_CS8_GPIO_PORT, ADC_CS8_GPIO_PIN, GPIO_PIN_RESET)
#define	SET_ADC_CS9()	 					HAL_GPIO_WritePin(ADC_CS9_GPIO_PORT, ADC_CS9_GPIO_PIN, GPIO_PIN_SET)			
#define	CLR_ADC_CS9()	 					HAL_GPIO_WritePin(ADC_CS9_GPIO_PORT, ADC_CS9_GPIO_PIN, GPIO_PIN_RESET)
#define	SET_ADC_CS10()	 					HAL_GPIO_WritePin(ADC_CS10_GPIO_PORT, ADC_CS10_GPIO_PIN, GPIO_PIN_SET)			
#define	CLR_ADC_CS10()	 					HAL_GPIO_WritePin(ADC_CS10_GPIO_PORT, ADC_CS10_GPIO_PIN, GPIO_PIN_RESET)
#define	SET_ADC_CS11()	 					HAL_GPIO_WritePin(ADC_CS11_GPIO_PORT, ADC_CS11_GPIO_PIN, GPIO_PIN_SET)			
#define	CLR_ADC_CS11()	 					HAL_GPIO_WritePin(ADC_CS11_GPIO_PORT, ADC_CS11_GPIO_PIN, GPIO_PIN_RESET)
#define	SET_ADC_CS12()	 					HAL_GPIO_WritePin(ADC_CS12_GPIO_PORT, ADC_CS12_GPIO_PIN, GPIO_PIN_SET)			
#define	CLR_ADC_CS12()	 					HAL_GPIO_WritePin(ADC_CS12_GPIO_PORT, ADC_CS12_GPIO_PIN, GPIO_PIN_RESET)

//���üĴ���
#define	CMD_WR_CFG							0x03		//д���üĴ���
#define	CMD_RD_CFG							0x0B		//�����üĴ���
#define	CMD_RD_ADC							0x0C		//����SETUP1ִ������ת��
#define	CMD_WR_CSR1							0x05		//дͨ�����üĴ���1
#define	CMD_WR_CSR2							0x15		//дͨ�����üĴ���2
#define	CMD_WR_CSR3							0x25		//дͨ�����üĴ���3
#define	CMD_WR_CSR4							0x35		//дͨ�����üĴ���4
#define	CMD_CONV_CTN						0xC0		//����ת��

/*
*********************************************************************************************************
*                              				Private typedef
*********************************************************************************************************
*/
/* ADC���� */
typedef enum {
	TYPE_NULL = 0,
	TYPE_CS5530, 
	TYPE_CS5532,
	TYPE_AUTO,
}ADC_TYPE_ENUM;

/* ADC���� */
typedef enum
{
	GAIN_1X = 0,
	GAIN_2X, 
	GAIN_4X,
	GAIN_8X,
	GAIN_16X,
	GAIN_32X,
	GAIN_64X, 
}ADC_GAIN_ENUM;

/* ADC���Ƶ�� */
typedef enum
{
	RATE_100 = 0,
	RATE_50,
	RATE_25,
	RATE_12P5,	
	RATE_6P25,
	RATE_3200 = 8,
	RATE_1600,
	RATE_800,
	RATE_400,
	RATE_200,
}ADC_RATE_ENUM;

/* ADת���������� */
typedef enum {
	POLAR_BIPOLAR = 0,		//˫����
	POLAR_UNIPOLAR,  		//������
}ADC_POLAR_TypeDef; 

typedef enum {
	REF_VOLTAGE_3P3 = 0,	//3.3V�ο���ѹ
	REF_VOLTAGE_2P5,		//2.5V�ο���ѹ
}ADC_REF_VOLTAGE;

enum WireType {
	FourWire = 0,			//�����ƽ���
	SixWire,				//�����ƽ���
};

struct CS553X_Drv {
	ADC_DevEnum dev;					//�豸
	ADC_TYPE_ENUM type;					//ADC����
	ADC_GAIN_ENUM gain;					//�Ŵ���,	ֻ����CS5532�������CS550��оƬ���Ըò���
	ADC_RATE_ENUM rate;					//�������	���ʹ̶�Ϊ50HZ����ʱ��������
	ADC_POLAR_TypeDef polarity;			//��˫���Ի��ǵ�����
	ADC_REF_VOLTAGE vref;				//�ο���ѹֵ
	ADC_SETUP_TypeDef setup;			//��Ե���оƬ��������������
	enum WireType wireType;
	uint32_t readTimeout;				//����ʱʱ��[us]
	bool overflow;						//���
	bool chgConvFreq;					//ת��Ƶ��
	uint32_t filterNums;				//�˲�����
	bool chip;							//������
	
	bool (*init)(struct CS553X_Drv *this);
	bool (*read)(struct CS553X_Drv *this, int32_t *code);
	bool (*isDataReady)(void);
	void (*setCS)(bool en);
};

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
#if (ADC1_EN)
static __INLINE void SetCS1_CallBack(bool en)
{
	if (en == true)
	{
		CLR_ADC_CS1();
	} 
	else 
	{
		SET_ADC_CS1();
	}
}
#endif

#if (ADC2_EN)
static __INLINE void SetCS2_CallBack(bool en)
{
	if (en == true)
	{
		CLR_ADC_CS2();
	} 
	else 
	{
		SET_ADC_CS2();
	}
}
#endif

#if (ADC3_EN)
static __INLINE void SetCS3_CallBack(bool en)
{
	if (en == true)
	{
		CLR_ADC_CS3();
	} 
	else 
	{
		SET_ADC_CS3();
	}
}
#endif

#if (ADC4_EN)
static __INLINE void SetCS4_CallBack(bool en)
{
	if (en == true)
	{
		CLR_ADC_CS4();
	} 
	else 
	{
		SET_ADC_CS4();
	}
}
#endif

#if (ADC5_EN)
static __INLINE void SetCS5_CallBack(bool en)
{
	if (en == true)
	{
		CLR_ADC_CS5();
	} 
	else 
	{
		SET_ADC_CS5();
	}
}
#endif

#if (ADC6_EN)
static __INLINE void SetCS6_CallBack(bool en)
{
	if (en == true)
	{
		CLR_ADC_CS6();
	} 
	else 
	{
		SET_ADC_CS6();
	}
}
#endif

#if (ADC7_EN)
static __INLINE void SetCS7_CallBack(bool en)
{
	if (en == true)
	{
		CLR_ADC_CS7();
	} 
	else 
	{
		SET_ADC_CS7();
	}
}
#endif

#if (ADC8_EN)
static __INLINE void SetCS8_CallBack(bool en)
{
	if (en == true)
	{
		CLR_ADC_CS8();
	} 
	else 
	{
		SET_ADC_CS8();
	}
}
#endif

#if (ADC9_EN)
static __INLINE void SetCS9_CallBack(bool en)
{
	if (en == true)
	{
		CLR_ADC_CS9();
	} 
	else 
	{
		SET_ADC_CS9();
	}
}
#endif

#if (ADC10_EN)
static __INLINE void SetCS10_CallBack(bool en)
{
	if (en == true)
	{
		CLR_ADC_CS10();
	} 
	else 
	{
		SET_ADC_CS10();
	}
}
#endif

#if (ADC11_EN)
static __INLINE void SetCS11_CallBack(bool en)
{
	if (en == true)
	{
		CLR_ADC_CS11();
	} 
	else 
	{
		SET_ADC_CS11();
	}
}
#endif

#if (ADC12_EN)
static __INLINE void SetCS12_CallBack(bool en)
{
	if (en == true)
	{
		CLR_ADC_CS12();
	} 
	else 
	{
		SET_ADC_CS12();
	}
}
#endif

/*
*********************************************************************************************************
*                              				Private function prototypes
*********************************************************************************************************
*/
#if (ADC1_EN)
static bool ADC1_Init(struct CS553X_Drv *this);
#endif
#if (ADC2_EN)
static bool ADC2_Init(struct CS553X_Drv *this);
#endif
#if (ADC3_EN)
static bool ADC3_Init(struct CS553X_Drv *this);
#endif
#if (ADC4_EN)
static bool ADC4_Init(struct CS553X_Drv *this);
#endif
#if (ADC5_EN)
static bool ADC5_Init(struct CS553X_Drv *this);
#endif
#if (ADC5_EN)
static bool ADC5_Init(struct CS553X_Drv *this);
#endif
#if (ADC6_EN)
static bool ADC6_Init(struct CS553X_Drv *this);
#endif
#if (ADC7_EN)
static bool ADC7_Init(struct CS553X_Drv *this);
#endif
#if (ADC8_EN)
static bool ADC8_Init(struct CS553X_Drv *this);
#endif
#if (ADC9_EN)
static bool ADC9_Init(struct CS553X_Drv *this);
#endif
#if (ADC10_EN)
static bool ADC10_Init(struct CS553X_Drv *this);
#endif
#if (ADC11_EN)
static bool ADC11_Init(struct CS553X_Drv *this);
#endif
#if (ADC12_EN)
static bool ADC12_Init(struct CS553X_Drv *this);
#endif

static bool __ADC_Init(struct CS553X_Drv *this);
static bool ADC_ReadConv(struct CS553X_Drv *this, int32_t *code);
static bool ADC_ReadOrigin(struct CS553X_Drv *this, int32_t *code);

/*
*********************************************************************************************************
*                              				Private variables
*********************************************************************************************************
*/
static struct CS553X_Drv g_ADCDev[] = {
#if (ADC1_EN)
{
	.dev				= DEV_ADC1,	
	.chip				= ADC1_CHIP,
	.type				= ADC1_TYPE,
	.vref				= REF_VOLTAGE_3P3,
	.gain				= GAIN_32X,
	.rate 				= ADC_SAMPLE_FREQ,
	.polarity			= POLAR_BIPOLAR,
	.setup				= DEFAULT_SETUP,
	.wireType			= WIRE_TYPE,
	.readTimeout		= MAX_READ_TIMEOUT_US,
	.init				= ADC1_Init,
	.read				= ADC_ReadConv,
	.setCS				= SetCS1_CallBack,
},
#endif

#if (ADC2_EN)
{
	.dev				= DEV_ADC2,
	.chip				= ADC2_CHIP,
	.type				= ADC2_TYPE,
	.vref				= REF_VOLTAGE_3P3,
	.gain				= GAIN_32X,
	.rate 				= ADC_SAMPLE_FREQ,
	.polarity			= POLAR_BIPOLAR,
	.setup				= DEFAULT_SETUP,
	.wireType			= WIRE_TYPE,
	.readTimeout		= MAX_READ_TIMEOUT_US,
	.init				= ADC2_Init,
	.read				= ADC_ReadConv,
	.setCS				= SetCS2_CallBack,
},
#endif

#if (ADC3_EN)
{
	.dev				= DEV_ADC3,
	.chip				= ADC3_CHIP,
	.type				= ADC3_TYPE,
	.vref				= REF_VOLTAGE_3P3,
	.gain				= GAIN_32X,
	.rate 				= ADC_SAMPLE_FREQ,
	.polarity			= POLAR_BIPOLAR,
	.setup				= DEFAULT_SETUP,
	.wireType			= WIRE_TYPE,
	.readTimeout		= MAX_READ_TIMEOUT_US,
	.init				= ADC3_Init,
	.read				= ADC_ReadConv,
	.setCS				= SetCS3_CallBack,
},
#endif

#if (ADC4_EN)
{
	.dev				= DEV_ADC4,
	.chip				= ADC4_CHIP,
	.type				= ADC4_TYPE,
	.vref				= REF_VOLTAGE_3P3,
	.gain				= GAIN_32X,
	.rate 				= ADC_SAMPLE_FREQ,
	.polarity			= POLAR_BIPOLAR,
	.setup				= DEFAULT_SETUP,
	.wireType			= WIRE_TYPE,
	.readTimeout		= MAX_READ_TIMEOUT_US,
	.init				= ADC4_Init,
	.read				= ADC_ReadConv,
	.setCS				= SetCS4_CallBack,
},
#endif
#if (ADC5_EN)
{
	.dev				= DEV_ADC5,
	.chip				= ADC5_CHIP,
	.type				= ADC5_TYPE,
	.vref				= REF_VOLTAGE_3P3,
	.gain				= GAIN_32X,
	.rate 				= ADC_SAMPLE_FREQ,
	.polarity			= POLAR_BIPOLAR,
	.setup				= DEFAULT_SETUP,
	.wireType			= WIRE_TYPE,
	.readTimeout		= MAX_READ_TIMEOUT_US,
	.init				= ADC5_Init,
	.read				= ADC_ReadConv,
	.setCS				= SetCS5_CallBack,
},
#endif
#if (ADC6_EN)
{
	.dev				= DEV_ADC6,
	.chip				= ADC6_CHIP,
	.type				= ADC6_TYPE,
	.vref				= REF_VOLTAGE_3P3,
	.gain				= GAIN_32X,
	.rate 				= ADC_SAMPLE_FREQ,
	.polarity			= POLAR_BIPOLAR,
	.setup				= DEFAULT_SETUP,
	.wireType			= WIRE_TYPE,
	.readTimeout		= MAX_READ_TIMEOUT_US,
	.init				= ADC6_Init,
	.read				= ADC_ReadConv,
	.setCS				= SetCS6_CallBack,
},
#endif
#if (ADC7_EN)
{
	.dev				= DEV_ADC7,
	.chip				= ADC7_CHIP,
	.type				= ADC7_TYPE,
	.vref				= REF_VOLTAGE_3P3,
	.gain				= GAIN_32X,
	.rate 				= ADC_SAMPLE_FREQ,
	.polarity			= POLAR_BIPOLAR,
	.setup				= DEFAULT_SETUP,
	.wireType			= WIRE_TYPE,
	.readTimeout		= MAX_READ_TIMEOUT_US,
	.init				= ADC7_Init,
	.read				= ADC_ReadConv,
	.setCS				= SetCS7_CallBack,
},
#endif
#if (ADC8_EN)
{
	.dev				= DEV_ADC8,
	.chip				= ADC8_CHIP,
	.type				= ADC8_TYPE,
	.vref				= REF_VOLTAGE_3P3,
	.gain				= GAIN_32X,
	.rate 				= ADC_SAMPLE_FREQ,
	.polarity			= POLAR_BIPOLAR,
	.setup				= DEFAULT_SETUP,
	.wireType			= WIRE_TYPE,
	.readTimeout		= MAX_READ_TIMEOUT_US,
	.init				= ADC8_Init,
	.read				= ADC_ReadConv,
	.setCS				= SetCS8_CallBack,
},
#endif
#if (ADC9_EN)
{
	.dev				= DEV_ADC9,
	.chip				= ADC9_CHIP,
	.type				= ADC9_TYPE,
	.vref				= REF_VOLTAGE_3P3,
	.gain				= GAIN_32X,
	.rate 				= ADC_SAMPLE_FREQ,
	.polarity			= POLAR_BIPOLAR,
	.setup				= DEFAULT_SETUP,
	.wireType			= WIRE_TYPE,
	.readTimeout		= MAX_READ_TIMEOUT_US,
	.init				= ADC9_Init,
	.read				= ADC_ReadConv,
	.setCS				= SetCS9_CallBack,
},
#endif
#if (ADC10_EN)
{
	.dev				= DEV_ADC10,
	.chip				= ADC10_CHIP,
	.type				= ADC10_TYPE,
	.vref				= REF_VOLTAGE_3P3,
	.gain				= GAIN_32X,
	.rate 				= ADC_SAMPLE_FREQ,
	.polarity			= POLAR_BIPOLAR,
	.setup				= DEFAULT_SETUP,
	.wireType			= WIRE_TYPE,
	.readTimeout		= MAX_READ_TIMEOUT_US,
	.init				= ADC10_Init,
	.read				= ADC_ReadConv,
	.setCS				= SetCS10_CallBack,
},
#endif
#if (ADC11_EN)
{
	.dev				= DEV_ADC11,
	.chip				= ADC11_CHIP,
	.type				= ADC11_TYPE,
	.vref				= REF_VOLTAGE_3P3,
	.gain				= GAIN_32X,
	.rate 				= ADC_SAMPLE_FREQ,
	.polarity			= POLAR_BIPOLAR,
	.setup				= DEFAULT_SETUP,
	.wireType			= WIRE_TYPE,
	.readTimeout		= MAX_READ_TIMEOUT_US,
	.init				= ADC11_Init,
	.read				= ADC_ReadConv,
	.setCS				= SetCS11_CallBack,
},
#endif
#if (ADC12_EN)
{
	.dev				= DEV_ADC12,
	.chip				= ADC12_CHIP,
	.type				= ADC12_TYPE,
	.vref				= REF_VOLTAGE_3P3,
	.gain				= GAIN_32X,
	.rate 				= ADC_SAMPLE_FREQ,
	.polarity			= POLAR_BIPOLAR,
	.setup				= DEFAULT_SETUP,
	.wireType			= WIRE_TYPE,
	.readTimeout		= MAX_READ_TIMEOUT_US,
	.init				= ADC12_Init,
	.read				= ADC_ReadConv,
	.setCS				= SetCS12_CallBack,
},
#endif

};



/*
*********************************************************************************************************
*                              				Private functions
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : GetDevHandle
* Description	: ��ȡ�豸���
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static struct CS553X_Drv *GetDevHandle(ADC_DevEnum dev)
{
	struct CS553X_Drv *handle = NULL;
	
	for (uint8_t i=0; i<ARRAY_SIZE(g_ADCDev); ++i) {
		if (dev == g_ADCDev[i].dev) {
			handle = &g_ADCDev[i];
			break;
		}
	}
	
	return handle;
}

#if (CS553x_USE_HARDWARE_SPI)
/*
*********************************************************************************************************
* Function Name : CS553x_WriteByte
* Description	: CS553xд����
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void CS553x_WriteByte(uint8_t _ucValue)
{
	// SPI_WriteByte(SPI_Dev, _ucValue);
	 HAL_SPI_Transmit(&SPI_Dev, &_ucValue, 1, 10);
}
#if false
/*
*********************************************************************************************************
* Function Name : CS553x_ReadByte
* Description	: CS553x��ȡ����
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static uint8_t CS553x_ReadByte(void)
{
	return SPI_ReadByte(SPI_Dev);
}
#endif
/*
*********************************************************************************************************
* Function Name : CS553x_WriteByte
* Description	: CS553xд����
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void CS553x_WriteBytes(uint8_t *_ucValue, uint32_t length)
{
	// SPI_WriteBytes(SPI_Dev, _ucValue, length);
	 HAL_SPI_Transmit(&SPI_Dev, _ucValue, length, 10);
}

/*
*********************************************************************************************************
* Function Name : CS553x_ReadByte
* Description	: CS553x��ȡ����
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void CS553x_ReadBytes(uint8_t *_ucValue, uint32_t length)
{
	// SPI_ReadBytes(SPI_Dev, _ucValue, length);
	 HAL_SPI_Receive(&SPI_Dev, _ucValue, length, 10);
}
#else
/*
*********************************************************************************************************
* Function Name : CS553x_WriteByte
* Description	: д��һ�ֽ�����
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void CS553x_WriteByte(uint8_t dat)
{
	uint8_t i;
	for(i=0;i<8;i++)
	{
		if(dat&0x80)
		SET_ADC_MOSI();
		else
		CLR_ADC_MOSI(); 
		bsp_DelayUS(1); 
		SET_ADC_SCK();
		bsp_DelayUS(1); 		
		CLR_ADC_SCK();
		dat<<=1;
		bsp_DelayUS(1); 
	}
	SET_ADC_MOSI();
}

/*
*********************************************************************************************************
* Function Name : CS553x_ReadByte
* Description	: ����һ�ֽ�����
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static uint8_t CS553x_ReadByte(void)
{
	uint8_t i;
	uint8_t temp=0;
	for(i=0;i<8;i++)
	{
		temp<<=1;
		SET_ADC_SCK();
		bsp_DelayUS(1);   
		if(READ_ADC_MISO())
		temp|=1;
		CLR_ADC_SCK();
		bsp_DelayUS(1);  
	}
	return temp;
}

/*
*********************************************************************************************************
* Function Name : CS553x_ReadByte
* Description	: CS553x��ȡ����
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void CS553x_ReadBytes(uint8_t *_ucValue, uint32_t length)
{
	for (uint32_t i=0; i<length; ++i) {
		_ucValue[i] = CS553x_ReadByte();
	}
}

/*
*********************************************************************************************************
* Function Name : CS553x_WriteByte
* Description	: CS553xд����
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void CS553x_WriteBytes(uint8_t *_ucValue, uint32_t length)
{
	for (uint32_t i=0; i<length; ++i) {
		CS553x_WriteByte(_ucValue[i]);
	}
}
#endif

/*
*********************************************************************************************************
* Function Name : CS553x_WriteU32
* Description	: д����������
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void CS553x_WriteU32(uint32_t dat)
{
	dat = htonl(dat);
	CS553x_WriteBytes((uint8_t *)&dat, sizeof(dat));
}

/*
*********************************************************************************************************
* Function Name : CS553x_ReadU32
* Description	: ��ȡ��������
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static uint32_t CS553x_ReadU32(void)
{
	uint32_t reg = 0;
	CS553x_ReadBytes((uint8_t *)&reg, sizeof(reg));
	return htonl(reg);
}

/*
*********************************************************************************************************
* Function Name : CS553x_WriteReg
* Description	: ��Ĵ���д����
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void CS553x_WriteReg(uint8_t cmd, uint32_t dat)
{
	CS553x_WriteByte(cmd);
	CS553x_WriteU32(dat);	 
}

/*
*********************************************************************************************************
* Function Name : CS553x_ReadReg
* Description	: ���Ĵ���
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static uint32_t CS553x_ReadReg(uint8_t cmd)
{
	CS553x_WriteByte(cmd);
	return CS553x_ReadU32();
}

/*
*********************************************************************************************************
* Function Name : CS553x_SoftwareReset
* Description	: ������λ
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void CS553x_SoftwareReset(void)
{
	uint8_t i;
	for(i=0;i<15;i++)
	{
		CS553x_WriteByte(0xFF);
	}
	CS553x_WriteByte(0xFE);
}

/*
*********************************************************************************************************
* Function Name : ADC_GpioInit
* Description	: ADC��ʼ��GPIO
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void ADC_GpioInit(struct CS553X_Drv *this)
{
	ENABLE_ADC_GPIO_RCC();

#if (!CS553x_USE_HARDWARE_SPI)
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = ADC_MOSI_GPIO_PIN;	
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;		
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(ADC_MOSI_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = ADC_SCK_GPIO_PIN;
	HAL_GPIO_Init(ADC_SCK_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.Pin = ADC_MISO_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	HAL_GPIO_Init(ADC_MISO_GPIO_PORT, &GPIO_InitStructure);
#endif
}

#if (ADC1_EN)
/*
*********************************************************************************************************
* Function Name : ADC1_Init
* Description	: ADC1��ʼ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ADC1_Init(struct CS553X_Drv *this)
{
	ADC_GpioInit(this);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = ADC_CS1_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(ADC_CS1_GPIO_PORT, &GPIO_InitStructure);

	SET_ADC_CS1();
	CLR_ADC_MOSI();
	CLR_ADC_SCK();
	return true;
}
#endif

#if (ADC2_EN)
/*
*********************************************************************************************************
* Function Name : ADC2_Init
* Description	: ADC2��ʼ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ADC2_Init(struct CS553X_Drv *this)
{
	ADC_GpioInit(this);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = ADC_CS2_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(ADC_CS2_GPIO_PORT, &GPIO_InitStructure);

	SET_ADC_CS2();
	CLR_ADC_MOSI();
	CLR_ADC_SCK();
	return true;
}
#endif

#if (ADC3_EN)
/*
*********************************************************************************************************
* Function Name : ADC3_Init
* Description	: ADC3��ʼ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ADC3_Init(struct CS553X_Drv *this)
{
	ADC_GpioInit(this);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = ADC_CS3_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(ADC_CS3_GPIO_PORT, &GPIO_InitStructure);

	SET_ADC_CS3();
	CLR_ADC_MOSI();
	CLR_ADC_SCK();
	return true;
}
#endif

#if (ADC4_EN)
/*
*********************************************************************************************************
* Function Name : ADC4_Init
* Description	: ADC4��ʼ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ADC4_Init(struct CS553X_Drv *this)
{
	ADC_GpioInit(this);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = ADC_CS4_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(ADC_CS4_GPIO_PORT, &GPIO_InitStructure);

	SET_ADC_CS4();	
	CLR_ADC_MOSI();
	CLR_ADC_SCK();
	return true;
}
#endif

#if (ADC5_EN)
/*
*********************************************************************************************************
* Function Name : ADC5_Init
* Description	: ADC5��ʼ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ADC5_Init(struct CS553X_Drv *this)
{
	ADC_GpioInit(this);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = ADC_CS5_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(ADC_CS5_GPIO_PORT, &GPIO_InitStructure);

	SET_ADC_CS5();	
	CLR_ADC_MOSI();
	CLR_ADC_SCK();
	return true;
}
#endif

#if (ADC6_EN)
/*
*********************************************************************************************************
* Function Name : ADC6_Init
* Description	: ADC6��ʼ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ADC6_Init(struct CS553X_Drv *this)
{
	ADC_GpioInit(this);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = ADC_CS6_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(ADC_CS6_GPIO_PORT, &GPIO_InitStructure);

	SET_ADC_CS6();	
	CLR_ADC_MOSI();
	CLR_ADC_SCK();
	return true;
}
#endif

#if (ADC7_EN)
/*
*********************************************************************************************************
* Function Name : ADC7_Init
* Description	: ADC7��ʼ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ADC7_Init(struct CS553X_Drv *this)
{
	ADC_GpioInit(this);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = ADC_CS7_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(ADC_CS7_GPIO_PORT, &GPIO_InitStructure);

	SET_ADC_CS7();	
	CLR_ADC_MOSI();
	CLR_ADC_SCK();
	return true;
}
#endif

#if (ADC8_EN)
/*
*********************************************************************************************************
* Function Name : ADC8_Init
* Description	: ADC8��ʼ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ADC8_Init(struct CS553X_Drv *this)
{
	ADC_GpioInit(this);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = ADC_CS8_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(ADC_CS8_GPIO_PORT, &GPIO_InitStructure);
	SET_ADC_CS8();	
	CLR_ADC_MOSI();
	CLR_ADC_SCK();
	return true;
}
#endif

#if (ADC9_EN)
/*
*********************************************************************************************************
* Function Name : ADC9_Init
* Description	: ADC9��ʼ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ADC9_Init(struct CS553X_Drv *this)
{
	ADC_GpioInit(this);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = ADC_CS9_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(ADC_CS9_GPIO_PORT, &GPIO_InitStructure);
	SET_ADC_CS9();	
	CLR_ADC_MOSI();
	CLR_ADC_SCK();
	return true;
}
#endif

#if (ADC10_EN)
/*
*********************************************************************************************************
* Function Name : ADC10_Init
* Description	: ADC10��ʼ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ADC10_Init(struct CS553X_Drv *this)
{
	ADC_GpioInit(this);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = ADC_CS10_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(ADC_CS10_GPIO_PORT, &GPIO_InitStructure);
	SET_ADC_CS10();	
	CLR_ADC_MOSI();
	CLR_ADC_SCK();
	return true;
}
#endif

#if (ADC11_EN)
/*
*********************************************************************************************************
* Function Name : ADC11_Init
* Description	: ADC11��ʼ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ADC11_Init(struct CS553X_Drv *this)
{
	ADC_GpioInit(this);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = ADC_CS11_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(ADC_CS11_GPIO_PORT, &GPIO_InitStructure);
	SET_ADC_CS11();	
	CLR_ADC_MOSI();
	CLR_ADC_SCK();
	return true;
}
#endif

#if (ADC12_EN)
/*
*********************************************************************************************************
* Function Name : ADC12_Init
* Description	: ADC12��ʼ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ADC12_Init(struct CS553X_Drv *this)
{
	ADC_GpioInit(this);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = ADC_CS12_GPIO_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;	
	HAL_GPIO_Init(ADC_CS12_GPIO_PORT, &GPIO_InitStructure);
	SET_ADC_CS12();	
	CLR_ADC_MOSI();
	CLR_ADC_SCK();
	return true;
}
#endif

/*
*********************************************************************************************************
* Function Name : ChangeRateHandler
* Description	: �޸�ת��Ƶ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void ChangeRateHandler(struct CS553X_Drv *this)
{
	if (this->type == TYPE_CS5530)
	{		
		uint32_t reg = 0;
		/* �ο���ѹ */
		reg |= this->vref << 25;
		/* ת������ */
		reg |= this->rate << 11;
		/* ���� */
		reg |= this->polarity << 10;		
		CS553x_WriteReg(CMD_WR_CFG, reg);
	}
	else if (this->type == TYPE_CS5532)
	{		
		/* ����ģʽ����ͨ�����üĴ��� */
		CS553x_WriteByte(0x45);
		for (uint8_t i=0; i<4; i++) {
			uint32_t reg = 0;
			/* Ƭѡ */
			reg |= 0x1 << 30;
			/* ���� */
			reg |= this->gain << 27;
			/* ת������ */
			reg |= this->rate << 23;
			/* ���� */
			reg |= this->polarity << 22;
			
			/* Ƭѡ */
			reg |= 0x0 << 14;
			/* ���� */
			reg |= this->gain << 11;
			/* ת������ */
			reg |= this->rate << 7;
			/* ���� */
			reg |= this->polarity << 6;
			CS553x_WriteU32(reg);
		}
		if (this->wireType == FourWire) {
			bsp_DelayUS(1);
			CS553x_WriteByte(0x81);		//��ƫ��У׼
			bsp_DelayUS(10);
			while(READ_ADC_MISO());	
		}
	}
}

/*
*********************************************************************************************************
* Function Name : __ADC_Init
* Description	: ADC��ʼ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool __ADC_Init(struct CS553X_Drv *this)
{				
	bsp_DelayMS(20);				//�ȴ�ʱ���ȶ�	
	/* ϵͳ��λ */
	this->setCS(true);	
	bsp_DelayMS(1); 
	
	CS553x_SoftwareReset();					//������λ
	CS553x_WriteReg(CMD_WR_CFG,0x20000000);	//ϵͳ��λ��RS=1
	bsp_DelayMS(50);									 
	CS553x_WriteReg(CMD_WR_CFG,0x00000000);	//RS=0
	{		
		uint32_t reg = 0;
		uint32_t i=0;
		do {
			reg = CS553x_ReadReg(CMD_RD_CFG);		//��ȡ���üĴ���
			i++;
			if (i>=1000)
			{				
				return false;
			}	
		} while ((reg >> 24) & 0x10);						//�ȴ�RV����0
	}
	//�����з�ʽ���Ĵ����������CS5532���ط�-1�������CS5530����-1�����������Ϊ-1
	if (this->type == TYPE_AUTO) {
		uint32_t reg = CS553x_ReadReg(0x4D);
		if (reg == 0xFFFFFFFF) {
			reg = CS553x_ReadReg(CMD_RD_CFG);
			if (reg == 0xFFFFFFFF) {
				this->chip = false;
				this->type = TYPE_NULL;
			} else {
				this->type = TYPE_CS5530;
			}			
		} else {
			this->type = TYPE_CS5532;
		}
		return __ADC_Init(this);
	}
	
	ChangeRateHandler(this);
	bsp_DelayUS(10);
	CS553x_WriteByte(CMD_CONV_CTN + this->setup); 			//ִ������ת��
	this->setCS(false);
	bsp_DelayUS(1);
	return true;
}

/*
*********************************************************************************************************
* Function Name : ConvRate
* Description	: ת��Ƶ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static ADC_RATE_ENUM ConvRate(struct CS553X_Drv *this, float rate)
{
	ADC_RATE_ENUM eRate = this->rate;
	rate *= 100;
	if (rate < 1250) {
		eRate = RATE_6P25;
	} else if (rate < 2500) {
		eRate = RATE_12P5;
	} else if (rate < 5000) {
		eRate = RATE_25;
	} else if (rate < 10000) {
		eRate = RATE_50;
	} else if (rate < 20000) {
		eRate = RATE_100;
	} else if (rate < 40000) {
		eRate = RATE_200;
	} else if (rate < 80000) {
		eRate = RATE_400;
	} else if (rate < 160000) {
		eRate = RATE_800;
	} else if (rate < 320000) {
		eRate = RATE_1600;
	} else {
		eRate = RATE_3200;
	}
	return eRate;
}

/*
*********************************************************************************************************
* Function Name : __RateToNums
* Description	: ��Ƶ��ת��Ϊ����
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static float __RateToNums(ADC_RATE_ENUM rate)
{
	switch (rate) {
	case RATE_6P25:
		return 6.25f;
	case RATE_12P5:
		return 12.5f;
	case RATE_25:
		return 25;
	case RATE_50:
		return 50;
	case RATE_100:
		return 100;
	case RATE_200:
		return 200;
	case RATE_400:
		return 400;
	case RATE_800:
		return 800;
	case RATE_1600:
		return 1600;
	case RATE_3200:
		return 3200;
	default:
		return DEFAULT_SEND_FREQ;
	}
}

/*
*********************************************************************************************************
* Function Name : __RateDec
* Description	: ��Ƶ�ʵݼ�
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static ADC_RATE_ENUM __RateDec(ADC_RATE_ENUM rate)
{
	switch (rate) {
	case RATE_6P25:
		return RATE_6P25;
	case RATE_12P5:
		return RATE_6P25;
	case RATE_25:
		return RATE_12P5;
	case RATE_50:
		return RATE_25;
	case RATE_100:
		return RATE_50;
	case RATE_200:
		return RATE_100;
	case RATE_400:
		return RATE_200;
	case RATE_800:
		return RATE_400;
	case RATE_1600:
		return RATE_800;
	case RATE_3200:
		return RATE_1600;
	default:
		return RATE_6P25;
	}
}

/*
*********************************************************************************************************
* Function Name : PrintRate
* Description	: ��ӡƵ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static char *PrintRate(struct CS553X_Drv *this)
{
	switch (this->rate) {
	case RATE_6P25:
		return "6.25Hz";
	case RATE_12P5:
		return "12.5Hz";
	case RATE_25:
		return "25Hz";
	case RATE_50:
		return "50Hz";
	case RATE_100:
		return "100Hz";
	case RATE_200:
		return "200Hz";
	case RATE_400:
		return "400Hz";
	case RATE_800:
		return "800Hz";
	case RATE_1600:
		return "1600Hz";
	case RATE_3200:
		return "3200Hz";
	default:
		return "null Hz";
	}
}

/*
*********************************************************************************************************
* Function Name : __SetRate
* Description	: ����ת��Ƶ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static void __SetRate(struct CS553X_Drv *this, float rate)
{	
	ADC_RATE_ENUM eRate = ConvRate(this, rate);
	if (eRate != this->rate) {
		this->rate = eRate;
		this->chgConvFreq = true;
		// ECHO(ECHO_DEBUG, "[BSP] ADC[%d] ��ʼ��, ת��Ƶ�� = %s", this->dev+1, PrintRate(this));
	}
}

/*
*********************************************************************************************************
* Function Name : ADC_ReadOrigin
* Description	: ��ȡADCԭʼ����
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ADC_ReadOrigin(struct CS553X_Drv *this, int32_t *code)
{
	bool ret = false;
	bsp_DelayUS(1);
	this->setCS(true);
	bsp_DelayUS(1);
	
	if (READ_ADC_MISO()) {
		this->setCS(false);
		return false;
	}

	if (this->chgConvFreq) {
		this->chgConvFreq = false;
		CS553x_WriteByte(0xFF);
		CS553x_ReadU32();
		ChangeRateHandler(this);
		CS553x_WriteByte(CMD_CONV_CTN + this->setup);
		//�л�Ƶ�ʺ󣬱���Ҫ�ȴ�3������
		this->filterNums = 3;
	} else {
		/* ���SDO��ʶ */
		CS553x_WriteByte(0x00); 
		/* ��ȡת����� */
		*code = CS553x_ReadU32();
		/* ������ */
		this->overflow = *code & 0x04;
		if (this->filterNums) {		
			this->filterNums--;
		} else {
			ret = true;
		}
	}
	
	this->setCS(false);
   	return ret;
}

/*
*********************************************************************************************************
* Function Name : ADC_ReadConv
* Description	: ��ȡADCת��������
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
static bool ADC_ReadConv(struct CS553X_Drv *this, int32_t *code)
{
    if (!this->chip) {
        return false;
    }
    
    int32_t ret = 0;    
    // 读取原始32位数据 (包含状态位等)
    if (ADC_ReadOrigin(this, &ret) == false) {        
        return false;
    }
    
    // ============================================================
    // 修改注：此处原有的移位逻辑已移除，由 App 层决定如何处理数据格式
    // ============================================================
    // 原逻辑：
    // if ((this->type == TYPE_CS5532) && (this->gain == GAIN_32X)) { ret >>= 12; } 
    // else if ... { ret >>= 13; }
    
    // 新逻辑：直接赋值，原样输出
    *code = ret;
    
    return true;
}


/*
*********************************************************************************************************
*                              				API
*********************************************************************************************************
*/
/*
*********************************************************************************************************
* Function Name : bsp_InitCS553x
* Description	: ��ʼ��CS553X
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void bsp_InitCS553x(void)
{
	/* ����ADC����ʱ��/�������ߣ���ˣ������ȳ�ʼ������ͨ����GPIO */
	for (uint8_t i=0; i<ARRAY_SIZE(g_ADCDev); ++i) {
		if (g_ADCDev[i].init) {
			g_ADCDev[i].init(&g_ADCDev[i]);
		}
	}
	
	for (uint8_t i=0; i<ARRAY_SIZE(g_ADCDev); ++i) {
		if (g_ADCDev[i].chip) {
			bool ret = __ADC_Init(&g_ADCDev[i]);
			// ECHO(ECHO_DEBUG, "[BSP] ADC %d ��ʼ�� .......... %s", g_ADCDev[i].dev+1, (ret) ? "OK" : "ERROR");
		}
	}
}

/*
*********************************************************************************************************
* Function Name : ADC_Sample
* Description	: ADC��������
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool ADC_Sample(ADC_DevEnum dev, int32_t *code)
{
	struct CS553X_Drv *this = GetDevHandle(dev);
	if (this == NULL) {
		return false;
	}
	return this->read(this, code);
}

/*
*********************************************************************************************************
* Function Name : ADC_IsOverflow
* Description	: ADC�Ƿ����
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool ADC_IsOverflow(ADC_DevEnum dev)
{
	struct CS553X_Drv *this = GetDevHandle(dev);
	if (this == NULL) {
		return false;
	}
	return this->overflow;
}

/*
*********************************************************************************************************
* Function Name : SetRate
* Description	: ����ת��Ƶ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void SetRate(ADC_DevEnum dev, float rate)
{
	struct CS553X_Drv *this = GetDevHandle(dev);
	if (this == NULL) {
		return;
	}
	__SetRate(this, rate);
}

/*
*********************************************************************************************************
* Function Name : ChangeSetup
* Description	: �޸�ͨ��
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
void ChangeSetup(ADC_DevEnum dev, ADC_SETUP_TypeDef setup)
{
	struct CS553X_Drv *this = GetDevHandle(dev);
	if (this == NULL) {
		return;
	}
	if (setup != this->setup) {
		this->chgConvFreq = true;
		this->setup = setup;
		// ECHO(ECHO_DEBUG, "[BSP] ADC[%d] ��ʼ��, ת��ͨ�� = %s", this->dev+1, (setup == SETUP_1) ? "SETUP1" : "SETUP2");
	}
}

/*
*********************************************************************************************************
* Function Name : ADC_IsBusy
* Description	: ADC�Ƿ�æµ
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
bool ADC_IsBusy(ADC_DevEnum dev)
{
	struct CS553X_Drv *this = GetDevHandle(dev);
	if (this == NULL) {
		return false;
	}
	return ((this->chgConvFreq) || (this->filterNums));
}

/*
*********************************************************************************************************
* Function Name : RateToNums
* Description	: Ƶ��ת��������
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
float RateToNums(ADC_DevEnum dev)
{
	struct CS553X_Drv *this = GetDevHandle(dev);
	if (this == NULL) {
		return DEFAULT_SEND_FREQ;
	}
	return __RateToNums(this->rate);
}

/*
*********************************************************************************************************
* Function Name : RateDecNums
* Description	: ����Ƶ�ʵݼ�
* Input			: None
* Output		: None
* Return		: None
*********************************************************************************************************
*/
float RateDecNums(ADC_DevEnum dev, uint8_t level)
{
	struct CS553X_Drv *this = GetDevHandle(dev);
	if (this == NULL) {
		return DEFAULT_SEND_FREQ;
	}
	ADC_RATE_ENUM rate = this->rate;
	for (uint8_t i=0; i<level; ++i) {
		 rate = __RateDec(rate);
	}
	return __RateToNums(rate);
}


/************************ (C) COPYRIGHT STMicroelectronics **********END OF FILE*************************/
