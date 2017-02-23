
/* Includes ------------------------------------------------------------------*/
#include "include.h"
char *pcTmp =   "������������ҵ��ʱ��ҹ�Լ��յĲ�иŬ����\r\n"
                "С�����ڽ���stm32��reference manual\r\n"
                "��Sd Card��Specification����ȫӢ���ĵ���\r\n"
                "�����ȫ������sdio����sd���������,\r\n"
                "���ɹ���ɻ��ڸ�������FATFS�ļ�ϵͳ��ֲ�����\r\n"
                "�����ڿ�����������֣��������ڿ��������ø�FATFSд�뵽SD����\r\n"
                "��������windows����Ļ������ʾ���ѡ�\r\n"
                "���Ŀ������������۰Ѷ���λŮ�񿴳���̫�Ŷ��������ġ�\r\n"
                "�������������Ҫ����ϸ��������������Գ����Ҫ\r\n"
                "���û�������Դ�����ܸ�Ч���õ�ǰ���£�����\r\n"
                "�Ż������ܡ��淶�����д����Ա������Ķ���Debug.\r\n"
                "������ʱ���Ѿ����ˣ�ɧ�꣬˯ʲô��? "
                "���Һ���ò��ֻ�ܿ�Ǯ������������ֽ���������ĳ����ӵ����롣---������2017-1-12";
//static  __align(4) uint8 aucSdWriteBuffer[] = "asdea8csa2asxa";
//static  __align(4) uint8 aucSdReadBuffer[512 * 3] = {0};

int main(void)
{
    DriverInit();
    BspInit();
    MiddleInit();

#if DELAY_DBG
    DelayTest();
#endif

#if SD_DBG//SD CARD
    SdTest();
#endif

#if FLASH_DBG//W25Q64
    W25Q64Test();
#endif

#if OLED_DBG//OLED
    OLED_Test();
#endif

#if FATFS_DBG//FATFS
    FatfsTest(pcTmp);
#endif

#if E2PROM_DBG
    E2PROM_Test();
#endif

#if BEEP_DBG
    BeepTest();
#endif

#if RC522_DBG
    Rc522SelfTest();
#endif

#if S50DBG//RC522
    S50Test();
#endif
    while(1);
    return 0;
}
