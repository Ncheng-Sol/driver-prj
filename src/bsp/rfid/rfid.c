#ifndef __RFID_C
#define __RFID_C

#ifdef __cplusplus
 extern "C" {
#endif 

#include "rc522/rc522.c"
#include "s50/s50.c"

/*
����:
    RFID ģ���ʼ��
����:   void
����ֵ: void
����:   void
���:   void
��ע:
*/
void RFIDInit(void)
{
    Rc522Init();
    S50DataInit();
    return;
}



#ifdef __cplusplus
}
#endif


#endif /* __RFID_C */
