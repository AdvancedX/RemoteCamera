/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-28
 * @brief       照相机 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32F103开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
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


extern uint8_t g_ov7725_vsta;       /* 在exit.c里 面定义 */
extern uint8_t g_ov7725_frame;      /* 在timer.c里面定义 */

/* 
 * 受限于OV7725模组的FIFO的容量，无法存下一帧640*480的画像，我们这里QVGA和VGA模式都采用
 * 320*240的分辨率,VGA模式下支持缩放，所以可以使用更小的分辨率来显示，读者可以根据需要自行修改
 */
uint16_t g_ov7725_wwidth  = 320;    /* 默认窗口宽度为320 */
uint16_t g_ov7725_wheight = 240;    /* 默认窗口高度为240 */

const char *LMODE_TBL[6]   = {"Auto", "Sunny", "Cloudy", "Office", "Home", "Night"};                    /* 6种光照模式 */
const char *EFFECTS_TBL[7] = {"Normal", "Negative", "B&W", "Redish", "Greenish", "Bluish", "Antique"};  /* 7种特效 */

/**
 * @brief       更新LCD显示
 *   @note      该函数将OV7725模块FIFO里面的数据拷贝到LCD屏幕上
 * @param       无
 * @retval      无
 */
void ov7725_camera_refresh(void)
{
    uint32_t i, j;
    uint16_t color;
 
    if (g_ov7725_vsta)                  /* 有帧中断更新 */
    {
        lcd_scan_dir(U2D_L2R);          /* 从上到下, 从左到右 */
        lcd_set_window((lcddev.width - g_ov7725_wwidth) / 2, (lcddev.height - g_ov7725_wheight) / 2,
                        g_ov7725_wwidth, g_ov7725_wheight);     /* 将显示区域设置到屏幕中央 */

        lcd_write_ram_prepare();        /* 开始写入GRAM */

        OV7725_RRST(0);                 /* 开始复位读指针 */
        OV7725_RCLK(0);
        OV7725_RCLK(1);
        OV7725_RCLK(0);
        OV7725_RRST(1);                 /* 复位读指针结束 */
        OV7725_RCLK(1);

        for (i = 0; i < g_ov7725_wheight; i++)
        {
            for (j = 0; j < g_ov7725_wwidth; j++)
            {
                OV7725_RCLK(0);
                color = OV7725_DATA;    /* 读数据 */
                OV7725_RCLK(1);
                color <<= 8;
                OV7725_RCLK(0);
                color |= OV7725_DATA;   /* 读数据 */
                OV7725_RCLK(1);
                LCD->LCD_RAM = color;
            }
        }

        g_ov7725_vsta = 0;              /* 清零帧中断标记 */
        g_ov7725_frame++;
        lcd_scan_dir(DFT_SCAN_DIR);     /* 恢复默认扫描方向 */
    }
}

/**
 * @brief       文件名自增（避免覆盖）
 *   @note      组合成形如 "0:PHOTO/PIC13141.bmp" 的文件名
 * @param       pname : 有效的文件名
 * @retval      无
 */
void camera_new_pathname(char *pname)
{
    uint8_t res;
    uint16_t index = 0;
    FIL *ftemp;
    
    ftemp = (FIL *)mymalloc(SRAMIN, sizeof(FIL));   /* 开辟FIL字节的内存区域 */

    if (ftemp == NULL) return;                      /* 内存申请失败 */

    while (index < 0XFFFF)
    {
        sprintf((char *)pname, "0:PHOTO/PIC%05d.bmp", index);
        res = f_open(ftemp, (const TCHAR *)pname, FA_READ); /* 尝试打开这个文件 */

        if (res == FR_NO_FILE)break;                /* 该文件名不存在, 正是我们需要的 */

        index++;
    }
    myfree(SRAMIN, ftemp);
}
uint16_t pic_get_tnum(char *path)
{
    uint8_t res;
    uint16_t rval = 0;
    DIR tdir;                                                 /* 临时目录 */
    FILINFO *tfileinfo;                                       /* 临时文件信息 */
    tfileinfo = (FILINFO *)mymalloc(SRAMIN, sizeof(FILINFO)); /* 申请内存 */
    res = f_opendir(&tdir, (const TCHAR *)path);              /* 打开目录 */

    if (res == FR_OK && tfileinfo)
    {
        while (1) /* 查询总的有效文件数 */
        {
            res = f_readdir(&tdir, tfileinfo); /* 读取目录下的一个文件 */

            if (res != FR_OK || tfileinfo->fname[0] == 0)
                break; /* 错误了/到末尾了,退出 */

            res = exfuns_file_type(tfileinfo->fname);

            if ((res & 0XF0) == 0X50) /* 取高四位,看看是不是图片文件 */
            {
                rval++; /* 有效文件数增加1 */
            }
        }
    }

    myfree(SRAMIN, tfileinfo); /* 释放内存 */
    return rval;
}

int main(void)
{
    uint8_t res;
	  DIR picdir;           /* 图片目录 */

           /* 图片目录 */
    FILINFO *picfileinfo; /* 文件信息 */
    char *pname;          /* 带路径的文件名 */
    uint16_t totpicnum;   /* 图片文件总数 */
    uint16_t curindex;    /* 图片当前索引 */
    uint8_t key;          /* 键值 */
    uint8_t pause = 0;    /* 暂停标记 */
    uint8_t t;
    uint16_t temp;
    uint32_t *picoffsettbl; /* 图片文件offset索引表 */
	  char *str = 0;
	  char msgbuf[15]; /* 消息缓存区 */
	  uint8_t lightmode = 0, effect = 0;
    uint8_t saturation = 4, brightness = 4, contrast = 4;

    uint8_t i;
    uint8_t sd_ok = 1;          /* 0, sd卡不正常; 1, SD卡正常 */
    uint8_t vga_mode = 0;       /* 0, QVGA模式(320 * 240); 1, VGA模式(640 * 480) */

    sys_stm32_clock_init(9);    /* 设置时钟, 72Mhz */
    delay_init(72);             /* 延时初始化 */
    usart_init(72, 115200);     /* 串口初始化为115200 */
    usmart_dev.init(72);        /* 初始化USMART */
    led_init();                 /* 初始化LED */
    lcd_init();                 /* 初始化LCD */
    key_init();                 /* 初始化按键 */
    beep_init();                /* 蜂鸣器初始化 */
    norflash_init();            /* 初始化NORFLASH */
    my_mem_init(SRAMIN);        /* 初始化内部SRAM内存池 */
    exfuns_init();              /* 为fatfs相关变量申请内存 */
    f_mount(fs[0], "0:", 1);    /* 挂载SD卡 */
    f_mount(fs[1], "1:", 1);    /* 挂载FLASH */
		remote_init();
    uint8_t tm = 0;

    piclib_init();              /* 初始化画图 */
    while (fonts_init())        /* 检查字库 */
    {
        lcd_show_string(30, 50, 200, 16, 16, "Font Error!", RED);
        delay_ms(200);
        lcd_fill(30, 50, 240, 66, WHITE);   /* 清除显示 */
        delay_ms(200);
    }

    text_show_string(30, 50, 200, 16, "正点原子STM32开发板", 16, 0, RED);
    text_show_string(30, 70, 200, 16, "照相机 实验", 16, 0, RED);
    text_show_string(30, 90, 200, 16, "KEY0:拍照(bmp格式)", 16, 0, RED);
    res = f_mkdir("0:/PHOTO");  /* 创建PHOTO文件夹 */

    if (res != FR_EXIST && res != FR_OK)    /* 发生了错误 */
    {
        res = f_mkdir("0:/PHOTO");          /* 创建PHOTO文件夹 */
        text_show_string(30, 110, 240, 16, "SD卡错误!", 16, 0, RED);
        delay_ms(200);
        text_show_string(30, 110, 240, 16, "拍照功能将不可用!", 16, 0, RED);
        delay_ms(200);
        sd_ok = 0;
    }

    while (ov7725_init() != 0)  /* 初始化OV7725 失败? */
    {
        lcd_show_string(30, 130, 200, 16, 16, "OV7725 Error!!", RED);
        delay_ms(200);
        lcd_fill(30, 130, 239, 246, WHITE);
        delay_ms(200);
    }
    
    lcd_show_string(30, 130, 200, 16, 16, "OV7725 Init OK       ", RED);

    delay_ms(1500);
    
    
    /* 输出窗口大小设置 QVGA模式 */
    g_ov7725_wwidth  = 320;                 /* 默认窗口宽度为320 */
    g_ov7725_wheight = 240;                 /* 默认窗口高度为240 */
    ov7725_window_set(g_ov7725_wwidth, g_ov7725_wheight, vga_mode);

    ov7725_light_mode(0);                   /* 自动 灯光模式 */
    ov7725_color_saturation(4);             /* 默认 色彩饱和度 */
    ov7725_brightness(4);                   /* 默认 亮度 */
    ov7725_contrast(4);                     /* 默认 对比度 */
    ov7725_special_effects(0);              /* 默认 特效 */

    OV7725_OE(0);                           /* 使能OV7725 FIFO数据输出 */

    pname = mymalloc(SRAMIN, 30);           /* 为带路径的文件名分配30个字节的内存 */
    btim_timx_int_init(10000, 7200 - 1);    /* 10Khz计数频率,1秒钟中断 */
    exti_ov7725_vsync_init();               /* 使能OV7725 VSYNC外部中断, 捕获帧中断 */
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
                LED1(0);                    /* 点亮LED1,提示正在拍照 */
                camera_new_pathname(pname); /* 得到文件名 */

                /* 编码成bmp图片 */
                if (bmp_encode((uint8_t *)pname, (lcddev.width - g_ov7725_wheight) / 2, (lcddev.height - g_ov7725_wwidth) / 2, g_ov7725_wheight, g_ov7725_wwidth, 0))
                {
                    text_show_string(40, 110, 240, 12, "写入文件错误!", 12, 0, RED);
                }
                else
                {
                    text_show_string(40, 110, 240, 12, "拍照成功!", 12, 0, BLUE);
                    text_show_string(40, 130, 240, 12, "保存为:", 12, 0, BLUE);
                    text_show_string(40 + 42, 130, 240, 12, pname, 12, 0, BLUE);
                    BEEP(1);        /* 蜂鸣器短叫，提示拍照完成 */
                    delay_ms(100);
                }
            }
            else     /* 提示SD卡错误 */
            {
                text_show_string(40, 110, 240, 12, "SD卡错误!", 12, 0, RED);
                text_show_string(40, 130, 240, 12, "拍照功能不可用!", 12, 0, RED);
            }

            BEEP(0);        /* 关闭蜂鸣器 */
            LED1(1);        /* 关闭LED1 */
            lcd_clear(BLACK);
						
            break;

                case 71://third key
           while (f_opendir(&picdir, "0:/PHOTO")) /* 打开图片文件夹 */
    {
        text_show_string(30, 150, 240, 16, "PHOTO文件夹错误!", 16, 0, RED);
        delay_ms(200);
        lcd_fill(30, 150, 240, 186, WHITE); /* 清除显示 */
        delay_ms(200);
    }

    totpicnum = pic_get_tnum("0:/PHOTO"); /* 得到总有效文件数 */

    while (totpicnum == NULL) /* 图片文件为0 */
    {
        text_show_string(30, 150, 240, 16, "没有图片文件!", 16, 0, RED);
        delay_ms(200);
        lcd_fill(30, 150, 240, 186, WHITE); /* 清除显示 */
        delay_ms(200);
    }

    picfileinfo = (FILINFO *)mymalloc(SRAMIN, sizeof(FILINFO)); /* 申请内存 */
    pname = mymalloc(SRAMIN, FF_MAX_LFN * 2 + 1);               /* 为带路径的文件名分配内存 */
    picoffsettbl = mymalloc(SRAMIN, 4 * totpicnum);             /* 申请4*totpicnum个字节的内存,用于存放图片索引 */

    while (!picfileinfo || !pname || !picoffsettbl) /* 内存分配出错 */
    {
        text_show_string(30, 150, 240, 16, "内存分配失败!", 16, 0, RED);
        delay_ms(200);
        lcd_fill(30, 150, 240, 186, WHITE); /* 清除显示 */
        delay_ms(200);
    }

    /* 记录索引 */
    res = f_opendir(&picdir, "0:/PHOTO"); /* 打开目录 */

    if (res == FR_OK)
    {
        curindex = 0; /* 当前索引为0 */

        while (1) /* 全部查询一遍 */
        {
            temp = picdir.dptr;                    /* 记录当前dptr偏移 */
            res = f_readdir(&picdir, picfileinfo); /* 读取目录下的一个文件 */

            if (res != FR_OK || picfileinfo->fname[0] == 0)
                break; /* 错误了/到末尾了,退出 */

            res = exfuns_file_type(picfileinfo->fname);

            if ((res & 0XF0) == 0X50) /* 取高四位,看看是不是图片文件 */
            {
                picoffsettbl[curindex] = temp; /* 记录索引 */
                curindex++;
            }
        }
    }

    text_show_string(30, 150, 240, 16, "开始显示...", 16, 0, RED);
    delay_ms(1500);
    piclib_init();                                         /* 初始化画图 */
    curindex = 0;                                          /* 从0开始显示 */
    res = f_opendir(&picdir, (const TCHAR *)"0:/PHOTO"); /* 打开目录 */

    while (res == FR_OK) /* 打开成功 */
    {		
				remote_scan();
        dir_sdi(&picdir, picoffsettbl[curindex]); /* 改变当前目录索引 */
        res = f_readdir(&picdir, picfileinfo);    /* 读取目录下的一个文件 */

        if (res != FR_OK || picfileinfo->fname[0] == 0)
            break; /* 错误了/到末尾了,退出 */

        strcpy((char *)pname, "0:/PHOTO/");                    /* 复制路径(目录) */
        strcat((char *)pname, (const char *)picfileinfo->fname); /* 将文件名接在后面 */
        lcd_clear(BLACK);
        piclib_ai_load_picfile(pname, 0, 0, lcddev.width, lcddev.height, 1); /* 显示图片 */
        text_show_string(2, 2, lcddev.width, 16, (char *)pname, 16, 1, RED); /* 显示图片名字 */
        t = 0;
				uint16_t ok =1;
				key = remote_scan(); 
				/* 扫描按键 */
				if (key == 71) 
            {
							  ok = 0 ;
							   break;
            }
        while (ok)
        {
					  remote_scan();
            key = remote_scan(); /* 扫描按键 */
            if ((t % 20) == 0)
            {
                LED0_TOGGLE(); /* LED0闪烁,提示程序正在运行. */
            }

            if (key == 68) /* 上一张 */
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
            else if (key == 67) /* 下一张 */
            {
                curindex++;

                if (curindex >= totpicnum)
                    curindex = 0; /* 到末尾的时候,自动从头开始 */
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

    myfree(SRAMIN, picfileinfo);  /* 释放内存 */
    myfree(SRAMIN, pname);        /* 释放内存 */
    myfree(SRAMIN, picoffsettbl); /* 释放内存 */      
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

                    ov7725_special_effects(effect); /* 设置特效 */
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
                    BEEP(1);        /* 蜂鸣器短叫，提示拍照完成 */
                    delay_ms(100);
								    BEEP(0);
                    break;

                case 74:
                    str = "DELETE";
                    break;
            }
				}
				    
        ov7725_camera_refresh();/* 更新显示 */
        i++;
				remote_scan();
        if (i >= 15)
        {
            i = 0;
            LED0_TOGGLE();  /* LED0闪烁 */
        }
    }
}









