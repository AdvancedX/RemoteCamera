/**
 ****************************************************************************************************
 * @file        beep.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-18
 * @brief       ������ ��������
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
 * V1.0 20200418
 * ��һ�η���
 *
 ****************************************************************************************************
 */

#ifndef __BEEP_H
#define __BEEP_H

#include "./SYSTEM/sys/sys.h"

/******************************************************************************************/
/* ���� ���� */

#define BEEP_GPIO_PORT                  GPIOB
#define BEEP_GPIO_PIN                   SYS_GPIO_PIN8
#define BEEP_GPIO_CLK_ENABLE()          do{ RCC->APB2ENR |= 1 << 3; }while(0)   /* PB��ʱ��ʹ�� */

/******************************************************************************************/

/* ���������� */
#define BEEP(x)         sys_gpio_pin_set(BEEP_GPIO_PORT, BEEP_GPIO_PIN, x)

/* BEEPȡ������ */
#define BEEP_TOGGLE()   do{ BEEP_GPIO_PORT->ODR ^= BEEP_GPIO_PIN; }while(0)     /* BEEP = !BEEP */

void beep_init(void);   /* ��ʼ�������� */

#endif
















