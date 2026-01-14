/****************************************Copyright (c)**************************************************
**                                �����θ߿Ƽ����޹�˾
**                                     
**                                
** ��   ��   ��: delay.h
** ����޸�����: 2014/4/25 18:07:05
** ��        ��: 
** ��	     ��: V1.0
** ��  ��  о Ƭ:  ����Ƶ��: 
** IDE:  ��ʱ����
**********************************************************************************************************/
#ifndef __DELAY_H
#define	__DELAY_H
  

void delay_us(uint16_t ud);		//����uS����ʱ
void delay_ms(uint16_t md); 	//����mS����ʱ	
// void delay_init_t2(void);		//TIM2��ʱ��ʱ����ʼ��
// void delay_t2(uint32_t t);		//TIM2�ľ�׼��ʱ����λ0.1mS
 
typedef enum
{
	TIMEIN=0,
	TIMEOUT,
}
TIMEOUT_Typedef; 
 
// void timeout_init(void);
// void timeout_set(uint32_t t);
// TIMEOUT_Typedef timeout_query(void);  
  

#endif




	

	

	












