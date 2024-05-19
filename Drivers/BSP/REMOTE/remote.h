/**
 ****************************************************************************************************
 * @file        remote.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-25
 * @brief       ����ң�ؽ��� ��������
 * @license     Copyright (c) 2020-2032, �������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� STM32F103������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 * �޸�˵��
 * V1.0 20200425
 * ��һ�η���
 *
 ****************************************************************************************************
 */

#ifndef __REMOTE_H
#define __REMOTE_H

#include "./SYSTEM/sys/sys.h"


/******************************************************************************************/
/* �����������ż���ʱ�� ���� */

#define REMOTE_IN_GPIO_PORT                     GPIOB
#define REMOTE_IN_GPIO_PIN                      SYS_GPIO_PIN9
#define REMOTE_IN_GPIO_CLK_ENABLE()             do{ RCC->APB2ENR |= 1 << 3; }while(0)   /* PB��ʱ��ʹ�� */


#define REMOTE_IN_TIMX                          TIM4
#define REMOTE_IN_TIMX_IRQn                     TIM4_IRQn
#define REMOTE_IN_TIMX_IRQHandler               TIM4_IRQHandler
#define REMOTE_IN_TIMX_CHY                      4                        /* ͨ��Y,  1<= Y <=4*/ 
#define REMOTE_IN_TIMX_CCRY                     REMOTE_IN_TIMX->CCR4
#define REMOTE_IN_TIMX_CHY_CLK_ENABLE()         do{ RCC->APB1ENR |= 1 << 2; }while(0)   /* TIMX ʱ��ʹ�� */

/******************************************************************************************/


#define RDATA           sys_gpio_pin_get(REMOTE_IN_GPIO_PORT, REMOTE_IN_GPIO_PIN)   /* ������������� */


/* ����ң��ʶ����(ID),ÿ��ң�����ĸ�ֵ��������һ��,��Ҳ��һ����.
 * ����ѡ�õ�ң����ʶ����Ϊ0
*/
#define REMOTE_ID       0

extern uint8_t g_remote_cnt;    /* �������µĴ��� */
    
void remote_init(void);         /* ���⴫��������ͷ���ų�ʼ�� */
uint8_t remote_scan(void);
#endif















