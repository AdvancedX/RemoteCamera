#include "./USMART/usmart.h"
#include "./USMART/usmart_str.h"

/******************************************************************************************/
/* �û�������
 * ������Ҫ�������õ��ĺ�����������ͷ�ļ�(�û��Լ�����)
 */
 
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./FATFS/exfuns/fattester.h"
#include "./PICTURE/piclib.h"


/* �������б���ʼ��(�û��Լ�����)
 * �û�ֱ������������Ҫִ�еĺ�����������Ҵ�
 */
struct _m_usmart_nametab usmart_nametab[] =
{
#if USMART_USE_WRFUNS == 1      /* ���ʹ���˶�д���� */
    (void *)read_addr, "uint32_t read_addr(uint32_t addr)",
    (void *)write_addr, "void write_addr(uint32_t addr,uint32_t val)",
#endif
    (void *)delay_ms, "void delay_ms(uint16_t nms)",
    (void *)delay_us, "void delay_us(uint32_t nus)",

    (void *)mf_scan_files, "uint8_t mf_scan_files(uint8_t * path)",
    (void*)piclib_ai_load_picfile,"uint8_t piclib_ai_load_picfile(const uint8_t *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t fast)",
    (void*)minibmp_decode,"uint8_t minibmp_decode(uint8_t *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t acolor, uint8_t mode)",
    (void*)bmp_encode,"uint8_t bmp_encode(uint8_t *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t mode)",

};

/******************************************************************************************/

/* �������ƹ�������ʼ��
 * �õ������ܿغ���������
 * �õ�����������
 */
struct _m_usmart_dev usmart_dev =
{
    usmart_nametab,
    usmart_init,
    usmart_cmd_rec,
    usmart_exe,
    usmart_scan,
    sizeof(usmart_nametab) / sizeof(struct _m_usmart_nametab), /* �������� */
    0,      /* �������� */
    0,      /* ����ID */
    1,      /* ������ʾ����,0,10����;1,16���� */
    0,      /* ��������.bitx:,0,����;1,�ַ��� */
    0,      /* ÿ�������ĳ����ݴ��,��ҪMAX_PARM��0��ʼ�� */
    0,      /* �����Ĳ���,��ҪPARM_LEN��0��ʼ�� */
};


















