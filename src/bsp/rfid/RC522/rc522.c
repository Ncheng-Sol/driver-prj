#ifndef __RC522_C
#define __RC522_C

#ifdef __cplusplus
 extern "C" {
#endif 

#include "rc522_reg.h"
#include "rc522_cmd.h"
#include "middle.h"

/*ʹ��/����MFRC522�ж�*/
#define RC522_INT_EN        0

#define RC522_RETRY_MAX   (110)//���Դ���

/*Timer Para*/
/*
ע��:RC522_TIMER_PRES ���뱣֤������ʱҪ��
*/
#define RC522_TIMER_PRES        (100)//Timer Prescaler 0 and 4095 
#if (0 == ((13560) / (RC522_TIMER_PRES * 2 + 1)))
#err "Timer Prescaler Err!"
#else
/*Reload Val Per Ms*/
#define RC522_RELOAD_MS         ((13.56 * 1000) / (RC522_TIMER_PRES * 2 + 1) - 1)
#endif

#define RC522_READ_FLG      (1 << 7)
#define RC522_WRITE_FLG     (0)

#define RC522_BUFFER_SZ     64//FIFO buffer size

#define RC522_SPI_CH        (Spi1)


/*RC522 GPIO*/
/*interrupt pin(�������interrupt pin RC522_INT_IRQn����ͬ������)*/
#define RC522_INT_PORT      (GPIOA)
#define RC522_INT_PIN       (GpioPin0)
#define RC522_INT_LINE      (EXTI_Line0)
#define RC522_INT_ENR       (RCC_APB2EN(GPIOA))

/*rest pin*/
#define RC522_RST_PORT      (GPIOA)
#define RC522_RST_PIN       (GpioPin4)
#define RC522_RST_ENR       (RCC_APB2EN(GPIOA))

/*SPI Baud rate*/
#define RC522_BR            (SPI_BaudRatePrescaler_8)//spi 72M / 8

/*WithOut Collision*/
#define NO_COLL             (0x00)
#define RANGE_COLL          (0xff)// position of the collision is out of the range

#if !RC522_INT_EN
/*�����ѯģʽ�µ�flg*/
typedef enum{
FlgTx,
FlgRx,
FlgIdle,
FlgTimer,
FlgCRC,
FlgMFCrypto1,
}enRc522FlgDef;
#endif

static uint8 Rc522WaitFlg(enRc522FlgDef enFlg);
static void Rc522PwrSw(enOnOffDef OnOff);
static uint8 Rc522ReadReg(uint8 const addr);
static void Rc522WriteReg(uint8 const addr, uint8 ucData);
static void Rc522ClrIntrBuffer(void);
static void Rc522Rst(void);

#if RC522_DBG
static void Rc522SelfTest(void);
#endif
static uint8 Rc522GetRxBitsCnt(void);
static bool Rc522SendCmd(uint8 const ucCmd);

/*��ȡaddr byte*/
#define RC522_GET_ADDR_BYTE(wr, addr)        (wr | ((addr) << 1))

#define Rc522IrqClr(cmd)                     do{\
                                                    Rc522WriteReg(RegComReq, 0x00);\
                                                    Rc522WriteReg(RegDivReq, 0x00);\
                                                }while(0)



/* Timer Start/Stop */
#define Rc522StartT()                         do{Rc522WriteReg(RegControl, TStartNow);}while(0)
#define Rc522StopT()                          do{Rc522WriteReg(RegControl, TStopNow);}while(0)
#define Rc522GetTReload(ms)                  ((ms) * RC522_RELOAD_MS)

/******************************RC522 Cfg Para************************************/


/************************************Rc522 sta/err check*********************/
/*
����:
    1.���Rc522 Error Register����ӦBits�Ƿ���λ
����:   
    1.ucMark: Ҫ����Bits����:CollErr etc.
����ֵ: 
    1.TRUE:  Ҫ����Bits����λ
    2.FALSE: Ҫ����Bitsδ��λ
����:   void
���:   void
��ע:   ��
ע��:   
    1.ִ��������������TempErr������д���λ
 */
static uint8 Rc522ChckErr(uint8 ucMark)
{
    uint8 ucTmp = 0;
	
    ucTmp = Rc522ReadReg(RegErr);

    return (ucTmp & ucMark) ? TRUE : FALSE;
}

/*
����:
    1.�ж�Rc522��������PICC������ʱ�Ƿ���������ײ����
����:   void
����ֵ: 
    TRUE:  ����ײ
    FALSE: ����ײ
����:   void
���:   void
��ע:   void
ע��:   void
*/
bool Rc522IsColl(void)
{
    return  Rc522ChckErr(CollErr) ? TRUE : FALSE;
}

/*
����:
    1.��ȡ��ײbit����λ��(1-32)
����:   
    1.ucMark: Ҫ����Bits����:CollErr etc.
����ֵ: 
    ��ײbit����λ��(����NO_COLL��ʾû����ײ, RANGE_COLL��ʾ��ײ����Χ)  
����:   void
���:   void
��ע:   void
ע��:   void
 */
static uint8 Rc522GetCollBitPos(void)
{
    uint8 ucPos = NO_COLL;
    uint8 ucTmp = 0;
    
    if (Rc522IsColl())
    {
        ucTmp = Rc522ReadReg(RegColl);
        if(ucTmp & CollPosNotValid)
        {// position of the collision is out of the range
            ucPos = RANGE_COLL;
        }
        else
        {
            ucPos = ucTmp & CollPosMask;
            if (0 == ucPos)
            {
                ucPos = 32;
            }
        }
    }
    return ucPos;
}

/*
����:
    1.��ȡ�ѳɹ����յ�Bits��
����:
    1.pucBitCntָ�����ڴ洢��ЧBits���ı�����ָ��
����ֵ:
    TRUE:   ����ײ
    FALSE:  ����ײ
��ע:   void
ע��:
    1.����ײbit����32bit����*pucBitCntΪ32
    2.��û����ײ������*pucBitCnt = FIFO�е����ݳ��� * 8
*/
bool Rc522GetValidCnt(uint8 *pucBitCnt)
{
    uint8 ucPos    = Rc522GetCollBitPos();
    bool  bIsColl  = FALSE;

    if (NO_COLL == ucPos)
    {//����ײλ
        uint8 ucRecvCnt = 0;

        ucRecvCnt = Rc522GetFifoLevel();
        *pucBitCnt = ucRecvCnt * 8;
        bIsColl = FALSE;
    }
    else if (RANGE_COLL == ucPos)
    {//��ײλ����Χ
        *pucBitCnt = 32;
        bIsColl = TRUE;
    }
    else
    {
        ucPos--;//1-32 -> 0-31 ��ײλ��ת��Ϊ��Чbits����
        *pucBitCnt = ucPos;
        bIsColl = TRUE;
    }
    return bIsColl;
}

/************************************Rc522 Init*********************/
#if RC522_INT_EN
/*
����:
    1.MFRC522 Interrupt Init
����:   void
����ֵ: void
��ע:   void
ע��:   void
*/
static void Rc522IntInit(void)
{
    /* interrupt enable*/
    Rc522WriteReg(RegComlEn, (TimerIEn | IRqInv));
    Rc522WriteReg(RegDivIEn, IRQPushPull);
    return;
}

/*
����:
    1.MFRC522 Interrupt ����
����:   void
����ֵ: void
��ע:   void
ע��:   void
*/
static void Rc522IntDeal(void)
{
    uint8 ucCom = 0;
    uint8 ucDiv = 0;

    ucCom = Rc522ReadReg(RegComReq);
    ucDiv = Rc522ReadReg(RegDivReq);

    if (ucCom & TimerIRq)
    {
        
    }
    if (ucCom & ErrIRq)
    {

    }
    if (ucCom & IdleIRq)
    {

    }
    if (ucCom & RxIRq)
    {

    }
    if (ucCom & TxIRq)
    {

    }
    if (ucDiv & CRCIRq)
    {

    }
    return;
}

/*
����:
    1.MFRC522 �����жϷ�����
����:   void
����ֵ: void
��ע:   void
ע��:   void
*/
static void Rc522Isr(void)
{
    Rc522IntDeal();
	Rc522IrqClr();
    EXTI_ClearITPendingBit(RC522_INT_LINE);//���־
    return;
}
#else
/*
����:
    1.MFRC522 ��ѯģʽ�£�������Interrupt��ʼ������
����:   void
����ֵ: void
��ע:   void
ע��:   void
*/
#define Rc522IntInit()       do{}while(0)

/*
����:
    1.�ȴ�ComIrqReg��DivIrqReg��Status2�Ĵ����е���ӦBits��λ
    2.����ComIrqReg��DivIrqReg�Ĵ����еĶ�ӦBits
    3.���ȴ���ʱ���ڼ�Error Register����Bit(s)��λ,�򷵻ش������
����:   void
����ֵ: 
    0:      success
    Others: error code
��ע:   void
ע��:   void
*/
static uint8 Rc522WaitFlg(enRc522FlgDef enFlg)
{
    uint8 err = 0xff;
    uint8 i   = RC522_RETRY_MAX;
    uint8 ucTmp = 0;
    uint8 ucAddr = 0;
    uint8 step = 0;
    uint8 ucFlg = 0;

    /*����enFlgȷ��Ҫ�����ļĴ�����ַ*/
    if (enFlg < FlgCRC)
    {
        ucAddr = RegComReq;
    }
    else if(FlgMFCrypto1 == enFlg)
    {
        ucAddr = RegStatus2;
    }
    else
    {
        ucAddr = RegStatus1;
    }
    /*��enFlgת��Ϊ��Ӧ�ļĴ����е�λ��־*/
    switch(enFlg)
    {
        case FlgTx:
        {
            ucFlg = TxIRq;
            break;
        }
        case FlgRx:
        {
            ucFlg = RxIRq;
            break;
        }
        case FlgIdle:
        {
            ucFlg = IdleIRq;
            break;
        }
        case FlgTimer:
        {
            ucFlg = TimerIRq;
            break;
        }
        case FlgCRC:
        {
            ucFlg = CRCReady;
            break;
        }
        case FlgMFCrypto1:
        {
            ucFlg = MFCrypto1On;
            break;
        }
        default:
        {
            step = 0;
            err = 0xff;
            goto ErrReTurn;
        }
    }
    do 
    {
        ucTmp = Rc522ReadReg(RegComReq);
    #if RC522_DBG
        printf("RegComReq:%x\r\n", (ucTmp));
    #endif
        err = Rc522ReadReg(RegErr);
        if (ucTmp & ErrIRq)
        {
            step = 1;
            goto ErrReTurn;
        }
        ucTmp = Rc522ReadReg(ucAddr);
        if (ucTmp & ucFlg)
        {
            goto SuccessReturn;
        }
        DelayUs(100);
    }
    while(i--);    
    err = 0xfe;
    step = 2;
ErrReTurn:
    PrintErr(Rc522WaitFlg, step, err);
    return err;
SuccessReturn:
#if S50DBG
    if (i != 0xff)
    {
        printf("Rc522 TimeOut:%d\r\n", i);
    }
#endif
    Rc522WriteReg(ucAddr, ucTmp & (~ucFlg));
    return 0;
}
#endif

/*
����:
    1.�ȴ����ݽ������,��ֹͣTransceive����
    2.FIFO�е����ݳ����Ƿ�Ϊ0
����:   void
����ֵ: 
    TRUE:  Success
    FALSE: Fail
��ע:   void
ע��:   
    1.������������Transceive�������ɺ�
*/
bool Rc522WaitRxEnd(void)
{
    bool res = FALSE;

#if 0//S50DBG
    if ((0 == Rc522WaitFlg(FlgRx)) && Rc522GetFifoLevel())
#else
    if (0 == Rc522WaitFlg(FlgRx))
#endif
    {
        res = TRUE;
    }   
    DelayMs(1);//FlgRx��λ����Ҫ��һ�������������ȡFIFO�е����ݳ���
    return res;
}

/*
����:
    1.��RC522����CRC_A
����:
    1.pucData ָ��Ҫ����CRC�����ݰ�ͷ
    2.ucLen   pucData�����ݳ���
����ֵ: 
    TRUE:  Success
    FALSE: Fail
����:   
    1.pucData
���:   void
��ע:   void
ע��:   
    1.��Ҫ���͵����ݽ�β����RCR_A
*/
bool Rc522CalcCrc(uint8 *pucData, uint8 ucLen)
{
    bool bRes = FALSE;
	
    Rc522WriteFIFO(pucData, ucLen);
    Rc522SendCmd(CmdCRC);
    if(Rc522WaitFlg(FlgCRC))
    {
        bRes = FALSE;
        printf("S50 Calc Crc Err!\r\n");
        goto ErrReturn;
    }
    bRes = TRUE;
ErrReturn:
    Rc522SendCmd(CmdIdle);
    return bRes;
}

/*
����:
    1.��ʼ��RC522 Timer
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void Rc522TimerInit(void)
{
    Rc522WriteReg(RegTMode,      TPresHiMask   & (RC522_TIMER_PRES >> 8));
    Rc522WriteReg(RegTPrescaler, TPresLiMask   & RC522_TIMER_PRES);
    //Rc522WriteReg(RegTReloadH,   TReloadHiMask & (RC522_TIMER_RELOAD >> 8));
    //Rc522WriteReg(RegTReloadL,   TReloadLiMask & (RC522_TIMER_RELOAD));
    return;
}

/*
����:
    1.RC522 ͨ�üĴ���Init
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void Rc522ComInit(void)
{
    Rc522IrqClr();
    Rc522WriteReg(RegStatus2, 0x00);
    Rc522WriteReg(RegColl, CollNoClr);//all received bits will be cleared after a collision disable
    Rc522WriteReg(RegMode, TxWaitRF | Crc6363);/*transmitter can only be started if an RF field is generated*/
    return;
}

/*
����:
    1.RC522 ������ؼĴ���Init
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void Rc522RxInit(void)
{
    //Rc522WriteReg(RegRxMode, RxSpeed106k | RxNoErr);
    //Rc522WriteReg(RegRxSel, 0);
    //Rc522WriteReg(RegRxThrd, 0);//defines the minimum signal strength at the decoder input that will be accepted
    Rc522WriteReg(RegMfRx, ParityDisable);
    return ;
}

/*
����:
    1.RC522 ������ؼĴ���Init
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void Rc522TxInit(void)
{
    //Rc522WriteReg(RegTxMode, TxSpeed106k);/*106 kBd   Tx CRC Disable */
    //Rc522WriteReg(RegTxCtrl, Tx2RFEn | Tx1RFEn);
    Rc522WriteReg(RegTxASK,  Force100ASK);
    Rc522WriteReg(RegTxSel,  DriverSel);
    return ;
}

/*
����:
    1.RC522 Hard Rest
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void Rc522Rst(void)
{
    GpioClrBit(RC522_RST_PORT, RC522_RST_PIN);
    DelayMs(1);
    GpioSetBit(RC522_RST_PORT, RC522_RST_PIN);
    DelayMs(1);
    return;
}

/*
����:
    1.RC522 Power On
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void Rc522PowerOn(void)
{
    Rc522Rst();
    Rc522PwrSw(enOff);
    Rc522PwrSw(enOn);
    Rc522SendCmd(CmdSoftRst);//soft rest
    Rc522RfSw(enOn);
    return;
}

/*
����:
    1.RC522 RF��ؼĴ���Init
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void Rc522RfInit(void)
{
    Rc522WriteReg(RegRFCfg,  RxGain);
    return ;
}

/*
����:
    1.RC522 Register Init
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void Rc522RegInit(void)
{
    Rc522PowerOn();
    Rc522IntInit();//�ж�����
    Rc522ComInit();//ͨ���ԼĴ�����ʼ��
    Rc522RxInit();
    Rc522TxInit();
    Rc522RfInit();
    Rc522TimerInit();
    return;
}

/*
����:
    1.RC522 SPI Init
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void Rc522SpiInit(void)
{
        stSpiInitDef stSpiInitInfo;
        SPI_InitTypeDef SPI_InitStruct;//used in StdPeriph_Lib

        /*Spi Init*/
        SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
        SPI_InitStruct.SPI_Mode      = SPI_Mode_Master;
        SPI_InitStruct.SPI_DataSize  = SPI_DataSize_8b;
        SPI_InitStruct.SPI_CPOL      = SPI_CPOL_Low;
        SPI_InitStruct.SPI_CPHA      = SPI_CPHA_1Edge;        
        SPI_InitStruct.SPI_NSS       = SPI_NSS_Soft;
        SPI_InitStruct.SPI_BaudRatePrescaler  = RC522_BR;
        SPI_InitStruct.SPI_FirstBit  = SPI_FirstBit_MSB;
        SPI_InitStruct.SPI_CRCPolynomial  = 1;

        stSpiInitInfo.Ch             = RC522_SPI_CH;
        stSpiInitInfo.SPI_InitStruct = &SPI_InitStruct;
        stSpiInitInfo.pSpiReadFunc   = RC522SpiRead;
        SpiInit(&stSpiInitInfo);
        return;
}

/*
����:
    1.RC522 GPIO Init
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void Rc522GpioInit(void)
{
    GpioInitDef GPIO_InitStruct;
#if RC522_INT_EN//�ж�ģʽ
    {
        EXTI_InitTypeDef EXTI_InitStruct;
        /*interrupt pin cfg*/
        RCC->APB2ENR |= RC522_INT_ENR;
        GPIO_InitStruct.GpioPin     = RC522_INT_PIN;
        GPIO_InitStruct.GPIO_Mode   = GpioModeInFloat;
        GPIO_InitStruct.GPIO_Speed  = GpioInput;
        GpioInit(RC522_INT_PORT, &GPIO_InitStruct);

        /*interrupt cfg*/
        EXTI_InitStruct.EXTI_Line    = RC522_INT_LINE;
        EXTI_InitStruct.EXTI_LineCmd = ENABLE;
        EXTI_InitStruct.EXTI_Mode    = EXTI_Mode_Interrupt;
        EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
        EXTI_Init(&EXTI_InitStruct);
        IrqRegester(RC522_INT_IRQn, Rc522Isr);
    }
#endif
    {
        RCC->APB2ENR |= RC522_RST_ENR;
        GPIO_InitStruct.GpioPin     = RC522_RST_PIN;
        GPIO_InitStruct.GPIO_Mode   = GpioModeOutPP;
        GPIO_InitStruct.GPIO_Speed  = GpioSpeed10MHz;
        GpioInit(RC522_RST_PORT, &GPIO_InitStruct);
    }
    return;
}

/*
����:
    1.RC522 Init
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
void Rc522Init(void)
{
    Rc522SpiInit();
    Rc522GpioInit();
    Rc522RegInit();
    return;
}

/*********************Rc522 Reg Read/Write***********************************************/
/*
����:
    1.Write RC522 Register
����:   
    1.addr:     Ҫд��Register�ĵ�ַ
    2.ucData:   Ҫд��Register������
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void Rc522WriteReg(uint8 const addr, uint8 ucData)
{
    uint8 aucTmp[2] = {0};
    uint8 i = 0;

    aucTmp[i++] = RC522_GET_ADDR_BYTE(RC522_WRITE_FLG, addr);
    aucTmp[i++] = ucData;
    SpiDataWrite(RC522_SPI_CH, aucTmp, 2);

    return;
}

/*
����:
    1.Read RC522 Register
����:   
    1.addr:     Ҫ��ȡ��Register�ĵ�ַ
����ֵ: 
    ��ȡ��Register������
����:   void
���:   void
��ע:   void
ע��:   void
*/
static uint8 Rc522ReadReg(uint8 const addr)
{
    uint8 ucTmp = 0;
    uint8 ucReg = 0;

    ucTmp = RC522_GET_ADDR_BYTE(RC522_READ_FLG, addr);
    SpiDataRead(RC522_SPI_CH, &ucReg, 1, &ucTmp, sizeof(ucTmp));

    return ucReg;
}

/*
����:
    �ж�CMD�Ƿ����Զ�����
����:
    ucCmd: ����
����ֵ:
    TRUE:  ���Զ�����
    FALSE: �����Զ�����
����:
    1.astCmdAttr:   �洢��Ӧ�����Ƿ��Զ������Ľṹ��
���:void
��ע:
*/
static bool Rc522ChckCmdAutoEnd(uint8 const ucCmd)
{
    uint8 i = 0;
    bool res = FALSE;
    
    for(i = 0; i < SizeOfArray(astCmdAttr); i++)
    {
        if (ucCmd == astCmdAttr[i].enCmd)
        {
            res = astCmdAttr[i].bIsAutoEnd;
            break;
        }
    }
    return res;
}

/*
����:
    1. ���RegComReq��RegDivReq�����ж������־
    2. ��RC522����CMD
    3. ������������Զ���ɣ���ȴ�RegComReq�е�IdleIRq��־
       �����������MFAuthent��ȴ�Status2Reg register�е�MFCrypto1On
       �����Ӧ��־�ȴ���ʱ���򷵻�FALSE
    4. ���
����:
    ucCmd: ����
����ֵ:
    TRUE:  Success
    FALSE: �ȴ�Ӧ��ʱ/����
����:void
���:void
��ע:
*/
static bool Rc522SendCmd(uint8 const ucCmd)
{
    bool res = FALSE;

    Rc522IrqClr();
    Rc522WriteReg(RegCmd, CmdIdle);
    if(CmdMFA == ucCmd)
    {//���RegStatus2�Ĵ���
        Rc522WriteReg(RegStatus2, 0x00);
    }
    Rc522WriteReg(RegCmd, ucCmd);
    if (Rc522ChckCmdAutoEnd(ucCmd & CmdMsk))
    {
        res = !Rc522WaitFlg(FlgIdle);
    }
    else if(CmdMFA == ucCmd)
    {
        res = !Rc522WaitFlg(FlgMFCrypto1);
    }
    else
    {
        res = TRUE;
    }
    if (FALSE == res)
    {
        printf("Rc522 Send Cmd Error:%x\r\n", ucCmd);
    }
    return res;
}

/*
����:
    1.��ȡ���յ����������һ���ֽڵ���Чλ��
����:   void
����ֵ:
    ���յ����������һ���ֽڵ���Чλ��(1-8)
����:   void
���:   void
��ע:   void
*/
static uint8 Rc522GetRxBitsCnt(void)
{
    uint8 ucTmp = RxBitsMask & Rc522ReadReg(RegControl);
    
    if(0 == ucTmp)
    {
        ucTmp = 8;
    }
    return ucTmp;
}

/************************Rc522 Reg Cfg**************************************************/
/*
����:
    1.Rc522 RF module On/Off
����:   
    1.OnOff: on/off����
����ֵ: void
����:   void
���:   void
��ע:   void
*/
void Rc522RfSw(enOnOffDef OnOff)
{
    bool bIsOff = (enOff == OnOff);
    uint8  ucReg = 0;
    uint8 ucCmd = 0;
	
    {//Rceive On/Off
        ucCmd = CmdNoCmd | (bIsOff ? RcvOff : 0);
        Rc522SendCmd(ucCmd);
    }
    {//Tran On/Off
        ucReg = (bIsOff ? 0 : (Tx2RFEn | Tx1RFEn | InvTx2RFOn));
        Rc522WriteReg(RegTxCtrl, ucReg);
    }
    DelayUs(1000);
    return;
}

/*
����:
    1.Soft power-down mode entered/exit
����:   
    1.OnOff: on/off����
����ֵ: void
����:   void
���:   void
��ע:   void
*/
static void Rc522PwrSw(enOnOffDef PwrOnOff)
{
    uint8 ucCmd   = CmdNoCmd | ( (enOff == PwrOnOff) ? PwrDown : 0);
    uint8 ucState = 0;

    Rc522SendCmd(ucCmd);
    if (enOn == PwrOnOff)
    {
        do
        {
            ucState = Rc522ReadReg(RegCmd);
            ucState &= PwrDown;
        }
        while(ucState);
    }
    return;
}

/*
����:
    1.���ݷ�������
����:   
    1.bIsCrc TRUE:���ݰ��а���CRC,FALSE:���ݰ��в�����CRC
����ֵ: void
����:   void
���:   void
��ע:   void
*/
void Rc522TxCfg(bool bIsCrc)
{
    uint8 ucTmp = 0;
    
    if(bIsCrc)
    {/*106 kBd   Tx CRC Enable */
        ucTmp = TxSpeed106k | TxCRCEn;
    }
    else
    {/*106 kBd   Tx CRC Disable */
        ucTmp = TxSpeed106k;
    }
    Rc522WriteReg(RegTxMode, ucTmp);
    return;
}

/*
����:
    1.���ݽ�������
����:   
    1.bIsCrc  �������ݰ����Ƿ����CRC
    2.bIsPrty �������ݰ����Ƿ����У��λ
����ֵ: void
����:   void
���:   void
��ע:   void
*/
void Rc522RxCfg(bool bIsCrc, bool bIsPrty)
{
    uint8 ucTmp = 0;
    
    if(bIsCrc)
    {/*106 kBd   Rx CRC Enable */

        ucTmp = RxSpeed106k | RxCRCEn | RxNoErr;
    }
    else
    {/*106 kBd   Rx CRC Disable */
        ucTmp = RxSpeed106k | RxNoErr;
    }
    Rc522WriteReg(RegRxMode, ucTmp);
    
    if(bIsPrty)
    {/*Parity Enable */
        ucTmp = 0;
    }
    else
    {/*Parity Disable */
        ucTmp = ParityDisable;
    }
    Rc522WriteReg(RegMfRx, ucTmp);
    return;
}
/*********************Rc522 CMD**********************************************************/

/*
����:
    1.������Ƶ���ݷ��Ͳ��ȴ��������
����:   
    1.TranLen     Ҫ���͵����һ���ֽ����ݵ�bits����(0-7,TX_WHOLE_BYTE��ʾ�����ֽ�ȫ������)
����ֵ: void
����:   void
���:   void
��ע:   void
*/
void Rc522StartTransCv(uint8 RxPos, uint8 TranLen)
{
    uint8 ucTmp = 0;
#if S50DBG
    static uint8 ucLst = 0;
#endif

    Rc522SendCmd(CmdTransCv);
    ucTmp = (RxPos << 4) | (TranLen & TxLastBits);
#if S50DBG
    ucLst = Rc522ReadReg(RegBitFraming);
    if(ucLst != ucTmp)
    {
        Rc522WriteReg(RegBitFraming, ucTmp);//FULL BYTE transmission/reception
    }
#endif
    Rc522WriteReg(RegBitFraming, ucTmp | StartSend);//FULL BYTE transmission/reception
    return;
}

/*
����:
    1.����RC522 Tranceive ָ��
����:   
    1.TranLen     Ҫ���͵����һ���ֽ����ݵ�bits����(0-7,TX_WHOLE_BYTE��ʾ�����ֽ�ȫ������)
����ֵ: void
����:   void
���:   void
��ע:   void
*/
void Rc522StopTransCv(void)
{
    Rc522SendCmd(CmdIdle);
    Rc522WriteReg(RegBitFraming, 0);//FULL BYTE transmission/reception
    return;
}

/*****************************************FIFO*******************************/

/*
����:
    1.��ȡFIFO�����ݳ���
����:   void
����ֵ: 
    FIFO�����ݳ���
��ע:   void
ע��:   void
*/
uint8 Rc522GetFifoLevel(void)
{
    uint8 ucTmp = 0;
    
    ucTmp = Rc522ReadReg(RegFIFOLevel) & FIFO_SzMsk;
    return ucTmp;
}

/*
����:
    1.��ȡRC522�е����ݲ�д��pData����
    2.���FIFO
����:
    1.pData: ָ���ȡ�����ָ��
    2.ucLen: Ҫ��ȡ�����ݳ���
����ֵ: 
    1.TRUE:  Success
    2.FALSE: Fail
����:   void
���:   void
��ע:
    1.���FIFO�е�����<ucLen�����ش���
    2.���FIFO�е�����>=ucLen�����ȡucLen��Bytes������
ע��:   void
*/
bool Rc522ReadFIFO(uint8 *pData, uint8 ucLen)
{
    uint8 aucSend = RC522_GET_ADDR_BYTE(RC522_READ_FLG, RegFIFOData);
    uint8 ucDataLen = 0;//FIFO�е���Ч���ݳ���
    bool  bRes = FALSE;

    ucDataLen = Rc522GetFifoLevel();
    if (ucDataLen < ucLen)
    {//���ȴ���
        goto ErrReturn;
    }
    if (ucDataLen > 0)
    {
        bRes = TRUE;
        SpiDataRead(RC522_SPI_CH, pData, ucLen, &aucSend, 0);
    }
    Rc522ClrFIFO();
ErrReturn:
    if(FALSE == bRes)
    {
        printf("Rc522ReadFIFO Err!\r\n");
    }
    return bRes;
}

/*
����:
    1.���FIFO
    2.д��ָ�����ȵ����ݵ�FIFO
����:
    pData:  ָ��д�뻺���ָ��
    uclen:  ��д������ݳ���
����ֵ: void
����:   void
���:   void
��ע:
    1.����/�������ݾ�����CRC��parity 
ע��:   void
*/
void Rc522WriteFIFO(uint8 *pData, uint8 uclen)
{
    uint8 *pucTmp = NULL;
    uint16 usIndex = 0;
    uint8  i =0;

    pucTmp = MemMalloc(enMemSmall, &usIndex);
    if(pucTmp)
    {
        Rc522ClrFIFO();
        pucTmp[i++] = RC522_GET_ADDR_BYTE(RC522_WRITE_FLG, RegFIFOData);
        memcpy(&pucTmp[i], pData, uclen);
        i += uclen;
        SpiDataWrite(RC522_SPI_CH, pucTmp, i);
        MemFree(enMemSmall, usIndex);
    }
    else
    {
        PrintErr(Rc522WriteFIFO, 0, 0);
    }
    
    return;
}

/*
����:
    1.���internal buffer
����:   void
����ֵ: void
��ע:   void
ע��:   void
*/
static void Rc522ClrIntrBuffer(void)
{
    uint8 aucTmp[25] = {0};

    Rc522WriteFIFO(aucTmp, sizeof(aucTmp));
    Rc522SendCmd(CmdMem);    
	Rc522ClrFIFO();
    return;
}

/*
����:
    1.��Rc522����10�������(Byte)��д�뻺��
����:   pucRead
����ֵ: 
    1.TRUE:  Success
    2.FALSE: Fail
����:   void
���:   
    1.pucRead   ָ�����ڻ������ݵ�ָ��
��ע:   ��
ע��:   
*/
bool Rc522GetRanData(uint8 *pucRead)
{
    bool bRes = FALSE;

    Rc522SendCmd(CmdGRID);
    Rc522ClrFIFO();
    Rc522SendCmd(CmdMem);
    bRes = Rc522ReadFIFO(pucRead, RANDOM_SZ);
    return bRes;
}
#if RC522_DBG

/*
����:
    1.���ڴ�ӡFIFO�е�����
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void Rc522PrintFIFO(void)
{
    uint8 *pucTmp = NULL;
    uint16 usIndex = 0;
    uint8  ucCnt   = 0;//���ڳɹ���ȡ�Ĵ洢fifo�е����ݳ���

    pucTmp = (uint8 *)MemMalloc(enMemSmall64, &usIndex);
    if(pucTmp)
    {
        Rc522ReadFIFO(pucTmp, RC522_BUFFER_SZ);
        UartPrintBuffer(pucTmp, ucCnt);
        MemFree(enMemSmall, usIndex);
    }


    return;
}

/*
����:
    1.��FIFO��д��һ��0(������)
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void Rc522WriteFIFOZero(void)
{
    uint8 ucTmp = 0;

    Rc522WriteFIFO(&ucTmp, sizeof(ucTmp));
    return;
}

/*
����:
    1.����RC522��MCUͨ���Ƿ�����
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void Rc522SelfTest(void)
{
    Rc522Rst();//Ӳ����λ
    Rc522IrqClr();
    Rc522ClrFIFO();
    Rc522SendCmd(CmdSoftRst);//soft rest
    Rc522ClrIntrBuffer();/*���internal buffer */
    Rc522WriteReg(AutoTestReg, 0x09);//Enable the self test 
    Rc522WriteFIFOZero();//Write 00h to the FIFO buffer.
    Rc522SendCmd(CmdCRC);//Start the self test with the CalcCRC command.
    while(0 == (Rc522ReadReg(RegStatus1) & CRCReady));
    Rc522PrintFIFO();

    return;
}

#endif
/**************************Timer***************************/

/*
����:
    1.����RC522 Timer�Ĵ����Եȴ�һ���ض�ʱ��
    2.��TimerIRq Bit
    3.���bIsAutoΪTRUE,��Timer����Ϊ������ģʽ(������ɺ�����),����
      ����������Timer
����:   
    1.usTime:   ��msΪ��λ��ʱ��
    2.bIsAuto:  �Ƿ��ڷ�����ɺ��Զ�����Timer
����ֵ: void
����:   void
���:   void
��ע:   
    
ע��:   void
*/
void Rc522TimerCfg(uint16 usTime, bool bIsAuto)
{
    uint16 usTReload = Rc522GetTReload(usTime);

    Rc522WriteReg(RegTReloadH,   (usTReload >> 8));
    Rc522WriteReg(RegTReloadL,   (usTReload));
    if (bIsAuto)
    {
        uint8 ucTmp = 0;

        ucTmp = Rc522ReadReg(RegTMode);
        Rc522WriteReg(RegTMode,   (usTReload));
    }
    else
    {
        Rc522StartT();
    }
    return;
}

/*
����:
    1.��CMD code(0x60 KeyA, 1Byte) + BlockAddr(1Byte) + KeyA(6Byte) + UID(4Byte)д��FIFO,
      ��ִ��MFRC522 MFAuthent ���� ������ִ�н��
����:   
    1.ucCmd:  Authentication command code (60h, 61h)
    2.ucBlockAddr: Block address
    3.pucKeyA: ָ��KeyA��ָ��

    4.pucUid : ָ��Uid��
����ֵ: 
    1.TRUE:  ��֤�ɹ�
    2.FALSE: ��֤ʧ��
����:   void
���:   void
��ע:   ��
ע��:   
*/
bool Rc522MFAuthent(uint8 ucCmd, uint8 ucBlockAddr, uint8 *pucKeyA, uint8 *pucUid)
{
    uint8 i = 0;
    uint8 aucTmp[12] = {0};
    bool  bRes = FALSE;

    aucTmp[i++] = ucCmd;
    aucTmp[i++] = ucBlockAddr;
    memcpy(&aucTmp[i], pucKeyA, 6);
    i += 6;
    memcpy(&aucTmp[i], pucUid, 4);
    i += 4;
    Rc522WriteFIFO(aucTmp, sizeof(aucTmp));

    bRes = Rc522SendCmd(CmdMFA);
    return bRes;
}

#ifdef __cplusplus
}
#endif
#endif

