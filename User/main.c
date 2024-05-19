/**
 ****************************************************************************************************
 * @file        main.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-28
 * @brief       ����� ʵ��
 * @license     Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� STM32F103������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./USMART/usmart.h"
#include "./MALLOC/malloc.h"
#include "./FATFS/exfuns/exfuns.h"
#include "./TEXT/text.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/SDIO/sdio_sdcard.h"
#include "./BSP/NORFLASH/norflash.h"
#include "./PICTURE/piclib.h"
#include "./BSP/OV7725/ov7725.h"
#include "./BSP/EXTI/exti.h"
#include "./BSP/TIMER/btim.h"
#include "./BSP/BEEP/beep.h"
#include "./BSP/REMOTE/remote.h"
#include "./PICTURE/piclib.h"

#include "string.h"
#include "math.h"


extern uint8_t g_ov7725_vsta;       /* ��exit.c�� �涨�� */
extern uint8_t g_ov7725_frame;      /* ��timer.c���涨�� */

/* 
 * ������OV7725ģ���FIFO���������޷�����һ֡640*480�Ļ�����������QVGA��VGAģʽ������
 * 320*240�ķֱ���,VGAģʽ��֧�����ţ����Կ���ʹ�ø�С�ķֱ�������ʾ�����߿��Ը�����Ҫ�����޸�
 */
uint16_t g_ov7725_wwidth  = 320;    /* Ĭ�ϴ��ڿ��Ϊ320 */
uint16_t g_ov7725_wheight = 240;    /* Ĭ�ϴ��ڸ߶�Ϊ240 */

const char *LMODE_TBL[6]   = {"Auto", "Sunny", "Cloudy", "Office", "Home", "Night"};                    /* 6�ֹ���ģʽ */
const char *EFFECTS_TBL[7] = {"Normal", "Negative", "B&W", "Redish", "Greenish", "Bluish", "Antique"};  /* 7����Ч */

/**
 * @brief       ����LCD��ʾ
 *   @note      �ú�����OV7725ģ��FIFO��������ݿ�����LCD��Ļ��
 * @param       ��
 * @retval      ��
 */
void ov7725_camera_refresh(void)
{
    uint32_t i, j;
    uint16_t color;
 
    if (g_ov7725_vsta)                  /* ��֡�жϸ��� */
    {
        lcd_scan_dir(U2D_L2R);          /* ���ϵ���, ������ */
        lcd_set_window((lcddev.width - g_ov7725_wwidth) / 2, (lcddev.height - g_ov7725_wheight) / 2,
                        g_ov7725_wwidth, g_ov7725_wheight);     /* ����ʾ�������õ���Ļ���� */

        lcd_write_ram_prepare();        /* ��ʼд��GRAM */

        OV7725_RRST(0);                 /* ��ʼ��λ��ָ�� */
        OV7725_RCLK(0);
        OV7725_RCLK(1);
        OV7725_RCLK(0);
        OV7725_RRST(1);                 /* ��λ��ָ����� */
        OV7725_RCLK(1);

        for (i = 0; i < g_ov7725_wheight; i++)
        {
            for (j = 0; j < g_ov7725_wwidth; j++)
            {
                OV7725_RCLK(0);
                color = OV7725_DATA;    /* ������ */
                OV7725_RCLK(1);
                color <<= 8;
                OV7725_RCLK(0);
                color |= OV7725_DATA;   /* ������ */
                OV7725_RCLK(1);
                LCD->LCD_RAM = color;
            }
        }

        g_ov7725_vsta = 0;              /* ����֡�жϱ�� */
        g_ov7725_frame++;
        lcd_scan_dir(DFT_SCAN_DIR);     /* �ָ�Ĭ��ɨ�跽�� */
    }
}

/**
 * @brief       �ļ������������⸲�ǣ�
 *   @note      ��ϳ����� "0:PHOTO/PIC13141.bmp" ���ļ���
 * @param       pname : ��Ч���ļ���
 * @retval      ��
 */
void camera_new_pathname(char *pname)
{
    uint8_t res;
    uint16_t index = 0;
    FIL *ftemp;
    
    ftemp = (FIL *)mymalloc(SRAMIN, sizeof(FIL));   /* ����FIL�ֽڵ��ڴ����� */

    if (ftemp == NULL) return;                      /* �ڴ�����ʧ�� */

    while (index < 0XFFFF)
    {
        sprintf((char *)pname, "0:PHOTO/PIC%05d.bmp", index);
        res = f_open(ftemp, (const TCHAR *)pname, FA_READ); /* ���Դ�����ļ� */

        if (res == FR_NO_FILE)break;                /* ���ļ���������, ����������Ҫ�� */

        index++;
    }
    myfree(SRAMIN, ftemp);
}
uint16_t pic_get_tnum(char *path)
{
    uint8_t res;
    uint16_t rval = 0;
    DIR tdir;                                                 /* ��ʱĿ¼ */
    FILINFO *tfileinfo;                                       /* ��ʱ�ļ���Ϣ */
    tfileinfo = (FILINFO *)mymalloc(SRAMIN, sizeof(FILINFO)); /* �����ڴ� */
    res = f_opendir(&tdir, (const TCHAR *)path);              /* ��Ŀ¼ */

    if (res == FR_OK && tfileinfo)
    {
        while (1) /* ��ѯ�ܵ���Ч�ļ��� */
        {
            res = f_readdir(&tdir, tfileinfo); /* ��ȡĿ¼�µ�һ���ļ� */

            if (res != FR_OK || tfileinfo->fname[0] == 0)
                break; /* ������/��ĩβ��,�˳� */

            res = exfuns_file_type(tfileinfo->fname);

            if ((res & 0XF0) == 0X50) /* ȡ����λ,�����ǲ���ͼƬ�ļ� */
            {
                rval++; /* ��Ч�ļ�������1 */
            }
        }
    }

    myfree(SRAMIN, tfileinfo); /* �ͷ��ڴ� */
    return rval;
}

int main(void)
{
    uint8_t res;
	  DIR picdir;           /* ͼƬĿ¼ */

           /* ͼƬĿ¼ */
    FILINFO *picfileinfo; /* �ļ���Ϣ */
    char *pname;          /* ��·�����ļ��� */
    uint16_t totpicnum;   /* ͼƬ�ļ����� */
    uint16_t curindex;    /* ͼƬ��ǰ���� */
    uint8_t key;          /* ��ֵ */
    uint8_t pause = 0;    /* ��ͣ��� */
    uint8_t t;
    uint16_t temp;
    uint32_t *picoffsettbl; /* ͼƬ�ļ�offset������ */
	  char *str = 0;
	  char msgbuf[15]; /* ��Ϣ������ */
	  uint8_t lightmode = 0, effect = 0;
    uint8_t saturation = 4, brightness = 4, contrast = 4;

    uint8_t i;
    uint8_t sd_ok = 1;          /* 0, sd��������; 1, SD������ */
    uint8_t vga_mode = 0;       /* 0, QVGAģʽ(320 * 240); 1, VGAģʽ(640 * 480) */

    sys_stm32_clock_init(9);    /* ����ʱ��, 72Mhz */
    delay_init(72);             /* ��ʱ��ʼ�� */
    usart_init(72, 115200);     /* ���ڳ�ʼ��Ϊ115200 */
    usmart_dev.init(72);        /* ��ʼ��USMART */
    led_init();                 /* ��ʼ��LED */
    lcd_init();                 /* ��ʼ��LCD */
    key_init();                 /* ��ʼ������ */
    beep_init();                /* ��������ʼ�� */
    norflash_init();            /* ��ʼ��NORFLASH */
    my_mem_init(SRAMIN);        /* ��ʼ���ڲ�SRAM�ڴ�� */
    exfuns_init();              /* Ϊfatfs��ر��������ڴ� */
    f_mount(fs[0], "0:", 1);    /* ����SD�� */
    f_mount(fs[1], "1:", 1);    /* ����FLASH */
		remote_init();
    uint8_t tm = 0;

    piclib_init();              /* ��ʼ����ͼ */
    while (fonts_init())        /* ����ֿ� */
    {
        lcd_show_string(30, 50, 200, 16, 16, "Font Error!", RED);
        delay_ms(200);
        lcd_fill(30, 50, 240, 66, WHITE);   /* �����ʾ */
        delay_ms(200);
    }

    text_show_string(30, 50, 200, 16, "����ԭ��STM32������", 16, 0, RED);
    text_show_string(30, 70, 200, 16, "����� ʵ��", 16, 0, RED);
    text_show_string(30, 90, 200, 16, "KEY0:����(bmp��ʽ)", 16, 0, RED);
    res = f_mkdir("0:/PHOTO");  /* ����PHOTO�ļ��� */

    if (res != FR_EXIST && res != FR_OK)    /* �����˴��� */
    {
        res = f_mkdir("0:/PHOTO");          /* ����PHOTO�ļ��� */
        text_show_string(30, 110, 240, 16, "SD������!", 16, 0, RED);
        delay_ms(200);
        text_show_string(30, 110, 240, 16, "���չ��ܽ�������!", 16, 0, RED);
        delay_ms(200);
        sd_ok = 0;
    }

    while (ov7725_init() != 0)  /* ��ʼ��OV7725 ʧ��? */
    {
        lcd_show_string(30, 130, 200, 16, 16, "OV7725 Error!!", RED);
        delay_ms(200);
        lcd_fill(30, 130, 239, 246, WHITE);
        delay_ms(200);
    }
    
    lcd_show_string(30, 130, 200, 16, 16, "OV7725 Init OK       ", RED);

    delay_ms(1500);
    
    
    /* ������ڴ�С���� QVGAģʽ */
    g_ov7725_wwidth  = 320;                 /* Ĭ�ϴ��ڿ��Ϊ320 */
    g_ov7725_wheight = 240;                 /* Ĭ�ϴ��ڸ߶�Ϊ240 */
    ov7725_window_set(g_ov7725_wwidth, g_ov7725_wheight, vga_mode);

    ov7725_light_mode(0);                   /* �Զ� �ƹ�ģʽ */
    ov7725_color_saturation(4);             /* Ĭ�� ɫ�ʱ��Ͷ� */
    ov7725_brightness(4);                   /* Ĭ�� ���� */
    ov7725_contrast(4);                     /* Ĭ�� �Աȶ� */
    ov7725_special_effects(0);              /* Ĭ�� ��Ч */

    OV7725_OE(0);                           /* ʹ��OV7725 FIFO������� */

    pname = mymalloc(SRAMIN, 30);           /* Ϊ��·�����ļ�������30���ֽڵ��ڴ� */
    btim_timx_int_init(10000, 7200 - 1);    /* 10Khz����Ƶ��,1�����ж� */
    exti_ov7725_vsync_init();               /* ʹ��OV7725 VSYNC�ⲿ�ж�, ����֡�ж� */
    lcd_clear(BLACK);
    lcd_show_string(30,  50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30,  70, 200, 16, 16, "OV7725 TEST", RED);
    lcd_show_string(30,  90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, "KEY0:Light Mode", RED);
    lcd_show_string(30, 130, 200, 16, 16, "KEY1:Saturation", RED);
    lcd_show_string(30, 150, 200, 16, 16, "KEY_UP:Contrast", RED);
    lcd_show_string(30, 170, 200, 16, 16, "TPAD:Effects", RED);
    lcd_show_string(30, 190, 200, 16, 16, "OV7725 Init...", RED);  
		ov7725_light_mode(lightmode);
    ov7725_color_saturation(saturation);
    ov7725_brightness(brightness);
    ov7725_contrast(contrast);
    ov7725_special_effects(effect);
		while (1)
    { 
        key = remote_scan();
				if(key){
				switch (key)
            {
                case 69:
                    str = "POWER";
                    break;

                case 70:
                    str = "UP";
                    break;

                case 64://PLAY
									if (sd_ok)
            {
                LED1(0);                    /* ����LED1,��ʾ�������� */
                camera_new_pathname(pname); /* �õ��ļ��� */

                /* �����bmpͼƬ */
                if (bmp_encode((uint8_t *)pname, (lcddev.width - g_ov7725_wheight) / 2, (lcddev.height - g_ov7725_wwidth) / 2, g_ov7725_wheight, g_ov7725_wwidth, 0))
                {
                    text_show_string(40, 110, 240, 12, "д���ļ�����!", 12, 0, RED);
                }
                else
                {
                    text_show_string(40, 110, 240, 12, "���ճɹ�!", 12, 0, BLUE);
                    text_show_string(40, 130, 240, 12, "����Ϊ:", 12, 0, BLUE);
                    text_show_string(40 + 42, 130, 240, 12, pname, 12, 0, BLUE);
                    BEEP(1);        /* �������̽У���ʾ������� */
                    delay_ms(100);
                }
            }
            else     /* ��ʾSD������ */
            {
                text_show_string(40, 110, 240, 12, "SD������!", 12, 0, RED);
                text_show_string(40, 130, 240, 12, "���չ��ܲ�����!", 12, 0, RED);
            }

            BEEP(0);        /* �رշ����� */
            LED1(1);        /* �ر�LED1 */
            lcd_clear(BLACK);
						
            break;

                case 71://third key
           while (f_opendir(&picdir, "0:/PHOTO")) /* ��ͼƬ�ļ��� */
    {
        text_show_string(30, 150, 240, 16, "PHOTO�ļ��д���!", 16, 0, RED);
        delay_ms(200);
        lcd_fill(30, 150, 240, 186, WHITE); /* �����ʾ */
        delay_ms(200);
    }

    totpicnum = pic_get_tnum("0:/PHOTO"); /* �õ�����Ч�ļ��� */

    while (totpicnum == NULL) /* ͼƬ�ļ�Ϊ0 */
    {
        text_show_string(30, 150, 240, 16, "û��ͼƬ�ļ�!", 16, 0, RED);
        delay_ms(200);
        lcd_fill(30, 150, 240, 186, WHITE); /* �����ʾ */
        delay_ms(200);
    }

    picfileinfo = (FILINFO *)mymalloc(SRAMIN, sizeof(FILINFO)); /* �����ڴ� */
    pname = mymalloc(SRAMIN, FF_MAX_LFN * 2 + 1);               /* Ϊ��·�����ļ��������ڴ� */
    picoffsettbl = mymalloc(SRAMIN, 4 * totpicnum);             /* ����4*totpicnum���ֽڵ��ڴ�,���ڴ��ͼƬ���� */

    while (!picfileinfo || !pname || !picoffsettbl) /* �ڴ������� */
    {
        text_show_string(30, 150, 240, 16, "�ڴ����ʧ��!", 16, 0, RED);
        delay_ms(200);
        lcd_fill(30, 150, 240, 186, WHITE); /* �����ʾ */
        delay_ms(200);
    }

    /* ��¼���� */
    res = f_opendir(&picdir, "0:/PHOTO"); /* ��Ŀ¼ */

    if (res == FR_OK)
    {
        curindex = 0; /* ��ǰ����Ϊ0 */

        while (1) /* ȫ����ѯһ�� */
        {
            temp = picdir.dptr;                    /* ��¼��ǰdptrƫ�� */
            res = f_readdir(&picdir, picfileinfo); /* ��ȡĿ¼�µ�һ���ļ� */

            if (res != FR_OK || picfileinfo->fname[0] == 0)
                break; /* ������/��ĩβ��,�˳� */

            res = exfuns_file_type(picfileinfo->fname);

            if ((res & 0XF0) == 0X50) /* ȡ����λ,�����ǲ���ͼƬ�ļ� */
            {
                picoffsettbl[curindex] = temp; /* ��¼���� */
                curindex++;
            }
        }
    }

    text_show_string(30, 150, 240, 16, "��ʼ��ʾ...", 16, 0, RED);
    delay_ms(1500);
    piclib_init();                                         /* ��ʼ����ͼ */
    curindex = 0;                                          /* ��0��ʼ��ʾ */
    res = f_opendir(&picdir, (const TCHAR *)"0:/PHOTO"); /* ��Ŀ¼ */

    while (res == FR_OK) /* �򿪳ɹ� */
    {		
				remote_scan();
        dir_sdi(&picdir, picoffsettbl[curindex]); /* �ı䵱ǰĿ¼���� */
        res = f_readdir(&picdir, picfileinfo);    /* ��ȡĿ¼�µ�һ���ļ� */

        if (res != FR_OK || picfileinfo->fname[0] == 0)
            break; /* ������/��ĩβ��,�˳� */

        strcpy((char *)pname, "0:/PHOTO/");                    /* ����·��(Ŀ¼) */
        strcat((char *)pname, (const char *)picfileinfo->fname); /* ���ļ������ں��� */
        lcd_clear(BLACK);
        piclib_ai_load_picfile(pname, 0, 0, lcddev.width, lcddev.height, 1); /* ��ʾͼƬ */
        text_show_string(2, 2, lcddev.width, 16, (char *)pname, 16, 1, RED); /* ��ʾͼƬ���� */
        t = 0;
				uint16_t ok =1;
				key = remote_scan(); 
				/* ɨ�谴�� */
				if (key == 71) 
            {
							  ok = 0 ;
							   break;
            }
        while (ok)
        {
					  remote_scan();
            key = remote_scan(); /* ɨ�谴�� */
            if ((t % 20) == 0)
            {
                LED0_TOGGLE(); /* LED0��˸,��ʾ������������. */
            }

            if (key == 68) /* ��һ�� */
            {
                if (curindex)
                {
                    curindex--;
                }
                else
                {
                    curindex = totpicnum - 1;
                }
								key = 0;
                break;
            }
            else if (key == 67) /* ��һ�� */
            {
                curindex++;

                if (curindex >= totpicnum)
                    curindex = 0; /* ��ĩβ��ʱ��,�Զ���ͷ��ʼ */
								key = remote_scan();
                break;
            }
						else if (key == 71) 
            {
							  ok = 0 ;
							  break;
            }
					
        }

        res = 0;
    }

    myfree(SRAMIN, picfileinfo);  /* �ͷ��ڴ� */
    myfree(SRAMIN, pname);        /* �ͷ��ڴ� */
    myfree(SRAMIN, picoffsettbl); /* �ͷ��ڴ� */      
		            break;

                case 67:
                    str = "RIGHT";
                    break;

                case 68:
                    str = "LEFT";
                    break;

                case 7:
                    str = "VOL-";
                    break;

                case 21:
                    str = "DOWN";
                    break;

                case 9:
                    str = "VOL+";
                    break;

                case 22://1
										saturation++;
                    if (saturation > 8)
                        saturation = 0;
                    ov7725_color_saturation(saturation);
										break;

                case 25://2
                    lightmode++;

                    if (lightmode > 5)
                        lightmode = 0;

                    ov7725_light_mode(lightmode);
                    sprintf((char *)msgbuf, "%s", LMODE_TBL[lightmode]);
                    break;

                case 13://3
                    contrast++;
                    if (contrast > 8)
                        contrast = 0;
                    ov7725_contrast(contrast);
                    sprintf((char *)msgbuf, "Contrast:%d", contrast);
                    break;

                    case 12://4
                    effect++;

                    if (effect > 6)
                    effect = 0;

                    ov7725_special_effects(effect); /* ������Ч */
                    sprintf((char *)msgbuf, "%s", EFFECTS_TBL[effect]);
                    tm = 20;                    
						        break;

                case 24:
                    str = "5";
                    break;

                case 94:
                    str = "6";
                    break;

                case 8:
                    str = "7";
                    break;

                case 28:
                    str = "8";
                    break;

                case 90:
                    str = "9";
                    break;

                case 66://0
                    saturation = 4;
                    lightmode = 0;
                    contrast = 4;
                    effect = 0;

								    ov7725_color_saturation(saturation);
								    ov7725_color_saturation(lightmode);
								    ov7725_color_saturation(contrast);
								    ov7725_color_saturation(effect);
                    BEEP(1);        /* �������̽У���ʾ������� */
                    delay_ms(100);
								    BEEP(0);
                    break;

                case 74:
                    str = "DELETE";
                    break;
            }
				}
				    
        ov7725_camera_refresh();/* ������ʾ */
        i++;
				remote_scan();
        if (i >= 15)
        {
            i = 0;
            LED0_TOGGLE();  /* LED0��˸ */
        }
    }
}









