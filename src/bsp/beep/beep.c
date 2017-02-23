#ifndef __BEEP_C
#define __BEEP_C

#ifdef __cplusplus
 extern "C" {
#endif 

/*
����:
    1.��������ʼ��
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
*/
void BeepInit(void)
{
    GpioInitDef GPIO_InitStruct;
    
    RCC->APB2ENR |= BEEP_ENR;

    GPIO_InitStruct.GpioPin    = BEEP_PIN;
    GPIO_InitStruct.GPIO_Mode  = GpioModeOutPP;
    GPIO_InitStruct.GPIO_Speed = GpioSpeed50MHz;
    GpioInit(BEEP_PORT, &GPIO_InitStruct);
    return;
}

/*
����:
    1.���������η���(����/����)
����:   
    1.enBpType :BeepShort(����)2.BeepLong(����)
����ֵ: void
����:   void
���:   void
��ע:   void
*/
static void BeepS(enBpTypedef enBpType)
{
    uint16 usDelay = 0;

    if (BeepShort == enBpType)
    {
        usDelay = BEEP_SHORT_T;
    }
    else
    {
        usDelay = BEEP_LONG_T;
    }
    GpioSetBit(BEEP_PORT, BEEP_PIN);
    DelayMs(usDelay);
    GpioClrBit(BEEP_PORT, BEEP_PIN);
    return;
}

/*
����:
    1.��������˫��
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
*/
static void BeepD(void)
{
    uint8 i = 0;

    for(i = 0; i < 2; i++)
    {
        BeepS(BeepShort);
        DelayMs(BEEP_DOUBLE_T);
    }
    return;
}

/*
����:
    1.����������
����:   
    enBpType :1.BeepShort(����)2.BeepLong(����)3.BeepDouble(˫��)
����ֵ: void
����:   void
���:   void
��ע:   void
*/
void Beep(enBpTypedef enBpType)
{
    switch(enBpType)
    {
        case BeepShort:
        case BeepLong:
        {
            BeepS(enBpType);
            break;
        }
        case BeepDouble:
        {
            BeepD();
            break;
        }
        default:
        {
            break;
        }
    }
    return;
}

#if BEEP_DBG
void BeepTest(void)
{
    BeepInit();
    while(1)
    {
        Beep(BeepShort);
        DelayMs(2000);
        Beep(BeepLong);
        DelayMs(2000);
        Beep(BeepDouble);
        DelayMs(2000);/**/
    }
    return;
}
#endif

#ifdef __cplusplus
}
#endif
#endif /* __BEEP_C */

