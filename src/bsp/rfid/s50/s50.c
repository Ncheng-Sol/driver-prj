#ifndef __S50_C
#define __S50_C

#ifdef __cplusplus
 extern "C" {
#endif 

#include "s50cmd.h"
#include "s50reg.h"

#define S50_REQA_INVL       200//REQA���ͼ��(ms)

typedef enum{
S50StaNull,//δѰ����
S50StaReady,//����̬,�ѳɹ��յ�PICC��REQA or WUPA cmd��Ӧ��ATQA
S50StaActive,//��ɿ�select
S50StaHalt,
}enS50StaDef;

/*
ע��:UID�Ĵ洢��ʽ����:
    single: UID0-UID4
    double: UID0-UID6
    triple: UID0-UID9
*/
typedef struct{
uint8 ucUidSz;//UID size
uint8 aucUID[S50_UID_MAX_SZ];//UID
uint8 aucClnBuff[CLN_MAX][S50_CLN_SZ];//�洢����UID�����ݻ���

uint8 aucKeyA[S50_KEYA_SZ];
}stS50InfoDef;

static bool S50SendShortFrame(uint8 ucCmd, uint8 *pucRecv, uint8 ucRecvLen);
static bool S50ChckBcc(uint8 *pucData);

static stS50InfoDef stS50Info = {0};

/*����Anticollision loop������͵����ݣ�����CLn���ݣ���Select����ʹ��*/
#define S50SaveClnData(level, pucData)         do{memcpy(stS50Info.aucClnBuff[level - 1], pucData, S50_CLN_SZ);}while(0)
#define S50GetClnData(level, pucData)          do{memcpy(pucData, stS50Info.aucClnBuff[level - 1], S50_CLN_SZ);}while(0)

/*��ATQA�з����UID Size*/
static  uint8 S50UidSzCNV[] = {4, 7, 10};//ATQA �е�UID size bit frameλ��0-2��Ӧ��UID size�ֱ�Ϊ4,7,10
#define S50GetUidSz(pucRecv)          (S50UidSzCNV[((stS50AtqaDef *)&pucRecv)->UidSz])

/*�����յ���־UID���������͵�SAK��Ӧ��Select�ȼ�������UID Size��stS50Info.ucUidSz*/
#define S50SaveUidSz(level)             do{stS50Info.ucUidSz = S50UidSzCNV[level - 1];}while(0)

/*
����:
    ��S50������Short frame���ȴ�Ӧ��
����:
    ucCmd: ����
    pucRecv: ָ����ջ����ָ��
����ֵ:
    TRUE:�ɹ��ȵ�Ӧ�� 
    FALSE:�ȴ�Ӧ��ʱ/����
����:void
���:void
��ע:
    1.ATQA,2Bytes,��У����CRC
    2.����14443-3��ATQAֻ��
*/
static bool S50SendShortFrame(uint8 ucCmd, uint8 *pucRecv, uint8 ucRecvLen)
{
    bool bRes = FALSE;

    Rc522RxCfg(FALSE, TRUE);
    Rc522TxCfg(FALSE);
    Rc522WriteFIFO(&ucCmd, sizeof(ucCmd));
    //Rc522RfSw(enOn);
    Rc522StartTransCv(0, SHORT_FRAME_LEN);//ShortFrameֻ��7bits����
    bRes = Rc522WaitRxEnd();
    Rc522StopTransCv();
    if(bRes && (FALSE == Rc522IsColl()))
    {//�յ�Ӧ��������ײ
        bRes = Rc522ReadFIFO(pucRecv, ucRecvLen);
    }
    else
    {
        bRes = FALSE;
    }
    //Rc522RfSw(enOff);
    return bRes;
}

/*
����:
    ��S50������Standard frame���ȴ�Ӧ��
����:
    pucRecv:  ָ����ջ����ָ��
    pucSend:  ָ���ͻ����ָ��
    ucSendLen: Ҫ���͵����ݳ���
����ֵ:
    TRUE:�ɹ��ȵ�Ӧ�� 
    FALSE:�ȴ�Ӧ��ʱ/����
����:void
���:void
��ע:
    1.����/�������ݾ�����CRC��parity 
*/
static bool S50SendStandFrame(uint8 *pucRecv, uint8 ucRecvLen, uint8 *pucSend, uint8 ucSendLen)
{
    bool bRes = FALSE;
    //uint8 aucCrc[2] = {0};

    Rc522RxCfg(TRUE, TRUE);
    Rc522TxCfg(TRUE);
    Rc522CalcCrc(pucSend, ucSendLen);    
    Rc522WriteFIFO(pucSend, ucSendLen);
    Rc522StartTransCv(0, TX_WHOLE_BYTE);//���ֽڷ���
    bRes = Rc522WaitRxEnd();
    Rc522StopTransCv();
    if((FALSE == bRes) || Rc522IsColl())
    {//����ײ����Rc522WaitRxEnd()���ش���
        bRes = FALSE;
        goto ErrReturn;
    }
    bRes = Rc522ReadFIFO(pucRecv, ucRecvLen);
ErrReturn:
    //Rc522RfSw(enOff);
    return bRes;
}

/*
����:
    ��S50������REQA���ȴ�Ӧ��
����:
    void
����ֵ:
    TRUE:�ɹ��ȵ�Ӧ�� FALSE:�ȴ�Ӧ��ʱ
����:void
���:UID Size
*/
static bool S50SendREQA(void)
{
    uint8  aucTmp[ATQA_SZ] = {0};
    bool   bRes  = FALSE;

    bRes = S50SendShortFrame(CMD_REQA, &aucTmp[0], sizeof(aucTmp));
    if(bRes)
    {
        stS50Info.ucUidSz = S50GetUidSz(aucTmp[0]);
        Beep(BeepDouble);
        printf("S50 Card Detected!UID Size:%d\r\n", stS50Info.ucUidSz);
    }
    return bRes;
}

/*
����:
    ��S50������WUPA���ȴ�Ӧ��
����:
    void
����ֵ:
    TRUE:�ɹ��ȵ�Ӧ�� FALSE:�ȴ�Ӧ��ʱ
����:void
���:UID Size
*/
bool S50SendWUPA(void)
{
    uint8  aucTmp[ATQA_SZ] = {0};
    bool   bRes  = FALSE;

    bRes = S50SendShortFrame(CMD_WUPA, &aucTmp[0], sizeof(aucTmp));
    if(bRes)
    {
        stS50Info.ucUidSz = S50GetUidSz(aucTmp[0]);
    }
    return bRes;
}

/*
����:
    Anticollision loop�����и��ݽ��յ������ݣ������Ҫ���͵�����
����:
    1.pucSend     ָ���ͻ����ָ��
    2.pusSendCnt  ָ��Ҫ���͵�����Bit����(����NVB��SEL)�Ľṹָ��
����ֵ: 
    1.TRUE:  success
    2.FALSE: fail
����:   void
���:   void
��ע:
    1.NVB�д洢���ǵ�ǰ����ͻ֡�е���ЧBytes+Bits
    2.�������ݰ�����:SEL + NVB + (n + 1)Bits(����nΪ���յ���bit��)
      ���Ͱ��ڽ��հ��������ڵ�һ����ͻλѡ����0��1(S50_COLL_SEL)
ע��:
    1.pstSendCnt->ucByteCnt�洢д��FIFO������(����������Byte)
    2.pstSendCnt->ucBitCnt �洢Ҫ���͵����һ��Byte�е���ЧBit��
*/
static bool S50PackAcData(uint8 *pucSend, stDataCntDef *pstSendCnt)
{
    uint8  ucSendCnt     = 0;//�˴ν��յ�bits��+�ϴη��͵�Bits��(bitΪ��λ,��SEL��NVB)
    const uint8  ucSendByteCnt = pstSendCnt->ucByteCnt;//����˴ν��յ�������ǰ,�����е���Ч���ݳ���(��������Byte)
    const uint8  ucSendBitCnt  = pstSendCnt->ucBitCnt;//����˴ν��յ�������ǰ,�����е����һ��Byte�е�Bit��
    uint8  aucRecv[S50_CLN_SZ] = {0};//�洢�˴��յ�������
    uint8  ucNewBitCnt = 0;//�½��յ���Ч���ݳ���(bitΪ��λ)
    bool   bIsColl = FALSE;

    if(0 == ucSendBitCnt)
    {//�ϴη��Ͳ����ڲ�������Byte,
        ucSendCnt = ucSendByteCnt * 8;
    }
    else
    {
        ucSendCnt = (ucSendByteCnt - 1) * 8 + ucSendBitCnt;
    }
    bIsColl = Rc522GetValidCnt(&ucNewBitCnt);
    ucSendCnt += ucNewBitCnt;//�ϴη���Bits��+�˴ν��յ�Bits��
    if(((ANTICOLL_FRAME_LEN * 8) < ucSendCnt) || ((FALSE == bIsColl) && ((ANTICOLL_FRAME_LEN * 8) != ucSendCnt)))
    {//���ݳ��ȴ���
        goto ErrReturn;
    }
    {//��ȡFIFO�е����ݣ����ε���Чλ
        uint8 ucDataLen  = 0;
        uint8 ucBitCnt   = ucSendBitCnt + ucNewBitCnt;//���յ��ĵ�һ��Bit�ڻ����е�λ��+���յ�����Ч���ݳ���(bitΪ��λ)
        
        if(0 == (ucBitCnt % 8))
        {
            ucDataLen = ucBitCnt / 8;
            Rc522ReadFIFO(&aucRecv[0], ucDataLen);
        }
        else
        {//���յ�������β���ڲ�����Byte ucDataLen + 1
            ucDataLen = ucBitCnt / 8 + 1;
            Rc522ReadFIFO(&aucRecv[0], ucDataLen);
            aucRecv[ucDataLen - 1] &= BitMask(ucBitCnt % 8);//���ε�����β����Чλ
        }
    }
    {//�����յ�������ճ����pucSend�� //��Ҫ���͵����һ��Bit���ó�S50_COLL_SEL
        uint8 ucTmp = ucNewBitCnt + ucSendBitCnt;
        uint8 ucByteCnt = ucTmp / 8;//�¼����Byte����
        uint8 ucBitCnt  = ucTmp % 8;

        if(ucBitCnt)
        {
            ucByteCnt++;
        }
        /*�����յ�������ճ����pucSend��*/
        if (0 == ucSendBitCnt)
        {//��ӵ�����ǰ�˲����ڲ�����Byte
            memcpy(&pucSend[ucSendByteCnt], &aucRecv[0], ucByteCnt);
        }
        else
        {
            pucSend[ucSendByteCnt - 1] |= aucRecv[0] & BitMask(ucSendBitCnt);//���ε�����ͷ����Чλ
            memcpy(&pucSend[ucSendByteCnt], &aucRecv[1], ucByteCnt - 1);
        }

        /*��Ҫ���͵����һ��Bit���ó�S50_COLL_SEL*/
        if((ANTICOLL_FRAME_LEN * 8) != ucSendCnt)
        {//UCLδ��������
            ucTmp++;
            ucByteCnt = ucTmp / 8;
            ucBitCnt  = ucTmp % 8;
            if(0 == ucBitCnt)
            {
                pucSend[ucSendByteCnt + ucByteCnt - 1] |= (S50_COLL_SEL << 7);
            }
            else
            {
                pucSend[ucSendByteCnt + ucByteCnt] |= (S50_COLL_SEL << (ucBitCnt - 1));
            }
        }
        pstSendCnt->ucBitCnt  = ucBitCnt;
        pstSendCnt->ucByteCnt = ucByteCnt + ucSendByteCnt;
        if(ucBitCnt)
        {
            pstSendCnt->ucByteCnt++;
        }
    }
    return TRUE;
ErrReturn:
    PrintErr(S50PackAcData, 00, 0xff);
    return FALSE;
}

/*
����:
    ���S50��Anticollision loopִ��״̬
����:   void
����ֵ:
    AntiColling:  ��ǰlevel�� Anticollision loopδ���
    AntiCollOk:   ��ǰlevel�� Anticollision loop�����(BCCУ��ͨ��)
    AntiCollErr:  Fail
����:void
���:void
��ע:
    1.NVB�д洢���ǵ�ǰ����ͻ֡�е���ЧBytes+Bits(����NVB��SEL)
    2.����ͻ֡��CRC����parity
*/
static enS50AcStaDef S50AntiCollStaChck(uint8 *pucUid, stDataCntDef stSendCnt)
{
    enS50AcStaDef enSta = AntiCollErr;
    uint8 ucSendBitCnt = stSendCnt.ucBitCnt;//Ҫ���͵����������һ���ֽڵ�Bit��
    uint8 ucSendByteCnt = stSendCnt.ucByteCnt;//Ҫ���͵����������һ���ֽڵ�Bit��

    if (ucSendByteCnt < ANTICOLL_FRAME_LEN)
    {
        enSta = AntiColling;
        goto Return;
    }   
    else if(ucSendByteCnt > ANTICOLL_FRAME_LEN)
    {
        enSta = AntiCollErr;
        goto Return;
    }
    else if((0 == ucSendBitCnt) && S50ChckBcc(pucUid))
    {
        enSta = AntiCollOk;
        goto Return;
    }
    
Return :
    return enSta;
}

/*
����:
    1.��S50������Anticollision loop���ݲ��ȴ��������
����:
    1.pucSend:      ָ���ͻ����ָ��
    2.pstSendCnt:   ָ��洢�������ݳ��ȵĽṹ���ָ��
����ֵ:
    TRUE:  Success 
    FALSE: Fail
����:void
���:void
��ע:
    1.��Ž��յ��ĵ�һ��Bit��λ�ã�ǡ��Ҳ��stSendCnt.ucBitCnt
*/
static bool S50SendAcData(uint8 *pucSend, stDataCntDef stSendCnt)
{
    bool bRes = FALSE;
    
    Rc522WriteFIFO(&pucSend[0], stSendCnt.ucByteCnt);
    Rc522StartTransCv(stSendCnt.ucBitCnt, stSendCnt.ucBitCnt);
    bRes = Rc522WaitRxEnd();
    Rc522StopTransCv();
    return bRes;
}

/*
����:
    ��S50���������ײѭ��leveln
����:
    ucLevel: Anticollision loop, cascade level(1-3)
����ֵ:
    TRUE:  Success 
    FALSE: Fail
����:void
���:void
��ע:
    1.NVB�д洢���ǵ�ǰ����ͻ֡�е���ЧBytes+Bits(����NVB��SEL)
    2.����ͻ֡��CRC����parity
*/
static bool S50AntiCollLeveln(uint8 ucLevel)
{
    bool   res = FALSE;//���غ���ִ�н��
    //bool   bIfSend = FALSE;
    uint8  ucStep = 0;//���ش�����
    uint8  aucSend[ANTICOLL_FRAME_LEN] = {0};
    uint8  ucLoopCnt  = 1;//ѭ����������
    stDataCntDef stSendCnt = {0};//�����������ݳ���ͳ��
    enS50AcStaDef enSta = AntiCollErr;

    Rc522RxCfg(FALSE, TRUE);
    Rc522TxCfg(FALSE);
    aucSend[SEL_POS] = GetSEL(ucLevel);
    aucSend[NVB_POS] = GetNVB(2, 0);//loop��ʼʱByteCntΪ2��BitCntΪ0
    stSendCnt.ucByteCnt = 2;//��Ҫ����2Byte����
    stSendCnt.ucBitCnt  = 0;//����ֽ���������
    //Rc522RfSw(enOn);
LoopStart:
    res=  S50SendAcData(&aucSend[0], stSendCnt);
    if((FALSE == res) || (FALSE == S50PackAcData(&aucSend[0], &stSendCnt)))
    {//���մ����������Ч
        res = FALSE;
        ucStep = 0;
        goto ErrReTurn;
    }
	enSta   = S50AntiCollStaChck(&aucSend[UID_POS], stSendCnt);
	if(AntiCollOk == enSta)
	{//���ַ���ײ�������
		S50SaveClnData(ucLevel, &aucSend[UID_POS]);//����CLn���ݣ���Select����ʹ��
		goto LoopEnd;
	}
	if ((AntiCollErr == enSta) || (ucLoopCnt > S50_LOOP_MAX))
	{
		ucStep = 1;
		res = FALSE;
		goto ErrReTurn;
	}
	else
	{
		ucLoopCnt++;
		goto LoopStart;
	}
ErrReTurn:
    PrintErr(S50AntiCollLeveln, ucStep, 0);
    return FALSE;
LoopEnd:
    //Rc522RfSw(enOff);
    return TRUE;
}

/*
����:
    calculated as exclusive-or over the 4 previous bytes, 
    �ж��Ƿ�BCCУ��ͨ��
����:
    pucData: ָ��Ҫ�������ʼ�ֽڵ�ָ��
����ֵ: 
    TRUE:  BCC���ͨ��
    FALSE: BCC���δͨ��
����:   void
���:   void
��ע:
*/
static bool S50ChckBcc(uint8 *pucData)
{
    uint8 ucRes = 0;
    uint8 i = 0;
    uint8 ucBcc = pucData[4];

    for(i = 0; i < 4; i++)
    {
        ucRes ^= pucData[i];
    }
    return (ucRes == ucBcc);
}

/*
����:
    ���ݴ洢��stS50Info.aucClnBuff�е����ݣ������UID sizeΪ1��PICC��UID
����:   void
����ֵ: void
����:   void
���:   void
��ע:
    1. UID size double: [CT][UID0][UID1][UID2][BCC],[CT][UID3][UID4][UID5][BCC],[UID6][UID7][UID8][UID9][BCC]
*/
static void S50GetTripleUid(void)
{
    uint8 (*paucData)[S50_CLN_SZ] = &stS50Info.aucClnBuff[0];

    /*Copy UID*/
    memcpy(&(stS50Info.aucUID[0]), &paucData[0][1], 3);//(UID0-UID2)
    memcpy(&(stS50Info.aucUID[3]), &paucData[1][1], 3);//(UID3-UID5)
    memcpy(&(stS50Info.aucUID[6]), &paucData[2][0], 4);//(UID6-UID9)
    return;
}

/*
����:
    ���ݴ洢��stS50Info.aucClnBuff�е����ݣ������UID sizeΪ1��PICC��UID
����:   void
����ֵ: void
����:   void
���:   void
��ע:
    1. UID size double: [CT][UID0][UID1][UID2][BCC],[UID3][UID4][UID5][UID6][BCC],
*/
static void S50GetDoubleUid(void)
{
    uint8 (*paucData)[S50_CLN_SZ] = &stS50Info.aucClnBuff[0];

    /*Copy UID*/
    memcpy(&(stS50Info.aucUID[0]), &paucData[0][1], 3);//(UID0-UID2)
    memcpy(&(stS50Info.aucUID[3]), &paucData[1][0], 4);//(UID3-UID6)
    return;
}

/*
����:
    ���ݴ洢��stS50Info.aucClnBuff�е����ݣ������UID sizeΪ1��PICC��UID
����:   void
����ֵ: void
����:   void
���:   void
��ע:
    1.UID size single: [UID0][UID1][UID2][UID3][BCC]
*/
static void S50GetSingleUid(void)
{
    uint8 *pucData = &(stS50Info.aucClnBuff[0][0]);
    
    memcpy(&(stS50Info.aucUID[0]), pucData, 4);
    return;
}

/*
����:
    ���ݴ洢��stS50Info.aucClnBuff�е����ݣ������UID
����:
    ucLevel: Anticollision loop, cascade level 
             ������SAK��ȷ��UID��������ȡ,ucLevel��UID size
����ֵ: void
����:   void
���:   void
��ע:
    1.UID size single: [UID0][UID1][UID2][UID3][BCC]
      UID size double: [CT][UID0][UID1][UID2][BCC],[UID3][UID4][UID5][UID6][BCC],
      UID size double: [CT][UID0][UID1][UID2][BCC],[CT][UID3][UID4][UID5][BCC],[UID6][UID7][UID8][UID9][BCC]
*/
static void S50GetUid(uint8 ucLevel)
{
    switch(ucLevel)
    {
        case 1:
        {
            S50GetSingleUid();
            break;
        }
        case 2:
        {
            S50GetDoubleUid();
            break;
        }
        case 3:
        {
            S50GetTripleUid();
            break;
        }
        default:
        {
            break;
        }
    }
    return;
}

/*
����:
    �������PICC��SAK,�ж�UID�Ƿ���������
����:
    void
����ֵ:
    UidNoCMPE: PCDδ�������յ�PICC��UID
    ProtOK: PCD���������յ�PICC��UID,��PICC����14443-4Э��
    ProtErr:PCD���������յ�PICC��UID,��PICC������14443-4Э��
����:void
���:void
*/
static enS50AcStaDef S50ChckSak(uint8 ucSAK)
{
    enS50AcStaDef stSta = AntiCollErr;

    if((ucSAK & SAK_UID_NO_COMP) == SAK_UID_NO_COMP)
    {
        stSta = AntiCollOk;
    }
    else if((ucSAK & SAK_UID_COMP_OK) == SAK_UID_COMP_OK)
    {
        stSta = AntiCollCmp;
    }
    else
    {
        stSta = AntiCollErr;
        printf("PICC is Not S50!\r\n");
    }
    return stSta;
}

/*
����:
    ���SELECT Command����
����:   
    ucLevel: ��ǰ��Anticollision loop, cascade level(1-3)
    pucData: ָ�򻺴��ָ��
����ֵ: 
    ���������ݳ���
����:   void
���:   void
��ע:
*/
static uint8 S50PackSelectData(uint8 ucLevel, uint8 *pucData)
{
    uint8 i = 0;
    
    pucData[i++] = GetSEL(ucLevel);
    pucData[i++] = GetNVB(ANTICOLL_FRAME_LEN, 0);//
    S50GetClnData(ucLevel, &pucData[i]);
    return ANTICOLL_FRAME_LEN;
}

/*
����:
    ��S50������SELECT Command�������յ���Ӧ�����UID��ȫ������ɣ��򷵻�TRUE
    ���򣬷���FALSE
����:   
    ucLevel: ��ǰ��Anticollision loop, cascade level(1-3)
����ֵ: 
    1.AntiCollErr:  ���ݽ��մ���
    2.AntiCollOk:   ���ݽ��մ��󣬵�UID ����δ���
    3.AntiCollCmp:  UID �������
����:   void
���:   void
��ע:
*/
static enS50AcStaDef S50SelectCard(uint8 ucLevel)
{
    uint8 aucSend[ANTICOLL_FRAME_LEN] = {0};
    uint8 aucRecv[SAK_SZ] = {0};
    bool  bRes  = FALSE;
    enS50AcStaDef enSta = AntiCollErr;
    
    S50PackSelectData(ucLevel, &aucSend[0]);
    bRes = S50SendStandFrame(&aucRecv[0], sizeof(aucRecv), &aucSend[0], sizeof(aucSend));
    if(FALSE == bRes)
    {
        enSta = AntiCollErr;
        goto ErrReturn;
    }
    enSta = S50ChckSak(aucRecv[0]);//���ճɹ�
    if(AntiCollCmp == enSta)
    {
        S50GetUid(ucLevel);
    }    
ErrReturn:
    return enSta;
}


/*
����:
    ��S50���������ײѭ��
����:
    void
����ֵ:
    TRUE:Success FALSE:Fail
����:void
���:void
*/
static bool S50AntiCollLoop(void)
{
    bool res = FALSE;
    //uint8 ucStep = 0;
    uint8 i = 0;
    enS50AcStaDef enSta = AntiCollErr;

    for(i = 0; i < CLN_MAX;)
    {
        res = S50AntiCollLeveln(i + 1);
        if(FALSE == res)
        {
            goto ErrReturn;
        }
        enSta = S50SelectCard(i + 1);
        switch(enSta)
        {
            case AntiCollErr:
            {//Recv Error
                res = FALSE;
                goto ErrReturn;
            }
            case AntiCollOk:
            {//Recv ok but UID TransFer not complete 
                i++;
                res = TRUE;
                break;
            }
            case AntiCollCmp:
            {//Anticollision loop complete
                S50SaveUidSz(i + 1);
                res = TRUE;
                goto SucReturn;
            }
            default :
            {
                break;
            }
        }
    }
ErrReturn:
    return res;
SucReturn:
    return TRUE;
}

/*
����:
    ��S50������HLTA���ʹ�����HLTA state
����:
    void
����ֵ:
    TRUE:�ɹ��ȵ�Ӧ�� FALSE:�ȴ�Ӧ��ʱ
*/
void S50SendHLTA(void)
{
    //uint8 aucTmp[] = {};
    return;
}

/*
����:
     S50������ݽṹ��ʼ��
����:   void
����ֵ: void
����:   void
���:   void
��ע:   ��
ע��:   ��
 */
void S50DataInit(void)
{
    uint8 ucTmp = 0;
    
    memset(&stS50Info, 0, sizeof(stS50Info));

    ucTmp = GetE2promAddr(S50KeyA);
    E2PROM_Read(ucTmp, &(stS50Info.aucKeyA[0]), S50_KEYA_SZ);//E2PROM������δ��ɣ�
    return;
}

/*
����:
    1.��S50������REQA
    2.����⵽S50������ִ�з���ײ����
    3.����ײѭ����ɺ�Select Card
����:   void
����ֵ: void
����:   void
���:   void
��ע:   ��
*/
bool S50CardDetect(void)
{
    bool bRes = FALSE;
    
    while(FALSE == S50SendREQA())
    {
        DelayMs(S50_REQA_INVL);
    }
    bRes = S50AntiCollLoop();
    return bRes;
}

/*
����:
    1.����UID Size MF��ȡ��֤�����еĲ���
����:   
    1.pucUid ָ���ucSeq����֤ʱ���������ָ��
    2.ucSeqΪ��ǰ��֤����(0-2)
����ֵ: void
����:   void
���:   void
��ע:   ��
ע��:   ��
*/
void S50MFGetUid(uint8 *pucUid, uint8 ucSeq)
{
    uint8 ucUidSz = stS50Info.ucUidSz;

    if (4 == ucUidSz)
    {//���Uid SizeΪ4����������֤���ظ�����UID
        memcpy(pucUid, stS50Info.aucUID, 4);
    }
    else if(7 == ucUidSz)
    {
        switch(ucSeq)
        {
            case 0:
            {//��һ����֤����CL2(UID3-UID6)
                memcpy(pucUid, &(stS50Info.aucClnBuff[1][0]), 4);
                break;
            }
            case 1:
            {//�ڶ�����֤����CL1(CT+UID0---UID2)
                memcpy(pucUid, &(stS50Info.aucClnBuff[0][0]), 4);
                break;
            }
            case 2:
            {//��������֤����UID0-UID3
                memcpy(pucUid, &(stS50Info.aucUID[0]), 4);
                break;
            }
            default:
            {
                break;
            }
        }
    }
    return;
}
/*
����:
    1.��S50��������֤����
    2.������֤���
����:   
    1.ucBlockAddr:  ��Ҫ��֤��Block Address(0-63)
����ֵ: 
    1.TRUE:  ��֤ͨ��
    2.FALSE: ��֤ʧ��
����:   void
���:   void
��ע:   ��
ע��:   ��
*/
bool S50MFAuthent(uint8 ucBlockAddr)
{
    uint8 *pucKeyA = stS50Info.aucKeyA;
    uint8 aucUid[4]   = {0};
    uint8 i = 0;//��¼��֤����
    bool  bRes = FALSE;

    //for(i = 0; i < MFAuthentTimes; i++)
    {
        S50MFGetUid(&aucUid[0], i);
        bRes = Rc522MFAuthent(CMD_AUT_KEYA, ucBlockAddr, pucKeyA, aucUid);
        if(FALSE == bRes)
        {
            goto ErrReturn;
        }
    }
ErrReturn:
    return bRes;
}


/*
����:
    1.�ȴ�MIFARE Classic ACK��NAK�����صȴ����
����: void
����ֵ: 
    1.��S50MF_ACK_Def
����: void
���: void
��ע: 
ע��:   
*/
S50MF_ACK_Def S50MFWaitAck(void)
{
    S50MF_ACK_Def enMfAck = NakTimeout;

#if S50DBG
    if (AckOk != enMfAck)
    {
        printf("S50MFWaitAck: %d\r\n", enMfAck);
    }
#endif
    return enMfAck;
}

/*
����:
    1.��S50������MIFARE Read����,�ȴ�Ӧ�𣬲������յ�������д�뻺��
����:   
    1.ucBlockAddr,Block��ַ
    2.pucRecv ָ����յ������ݵ�ָ��
����ֵ: 
    1.TRUE:  Success
    2.FALSE: Fail
����:   void
���:   void
��ע:   ��
ע��:   
    1.S50��ÿ��Block�洢16Bytes����,ʵ����Ч������Ϊ4Byte(����) + 1Byte(�����ڴ洢����Block��ַ)
      �����Ϊ�����õ��ظ�ֵ
    2.S50������С�˴洢
    3.�������ݲ�ȷ��
*/
static bool S50MFRecvBlockData(uint8 ucBlockAddr, uint8 *pucRecv)
{
    bool bRes = FALSE;
    uint8 aucSend[2] = {0};
    uint8 i = 0;

    aucSend[i++] = CMD_MIFARE_READ;
    aucSend[i++] = ucBlockAddr;
    bRes = S50SendStandFrame(pucRecv, S50BLOCK_DATA_SZ, aucSend, sizeof(aucSend));
    return bRes;
}

/*
����:
    1.���ݴ�S50���ж�ȡ��1Block�����ݣ������4Byte��Ч����
    2.����У����
����:   pucBlock,pulVal
����ֵ: 
    1.TRUE:  Success
    2.FALSE: Fail
����:   
    1.pstBlockData ָ����յ���S50��1Block���ݵ�ָ��
���:   
    1.pulVal   ָ��洢���������Ч���ݵ�ָ��
��ע:   ��
ע��:   
    1.S50��ÿ��Block�洢16Bytes����,ʵ����Ч������Ϊ4Byte(����) + 1Byte(�����ڴ洢����Block��ַ)
      �����Ϊ�����õ��ظ�ֵ
    2.S50������С�˴洢
    3.�������ݲ�ȷ��
*/
static bool S50MFUnPackReadData(stS50DataBlockDef *pstBlockData, uint32 *pulVal)
{
    {
        uint32 ulVal0 = pstBlockData->unVal0.ulWord;
        uint32 ulVal1 = pstBlockData->unVal1.ulWord;
        uint32 _ulVal = pstBlockData->_unVal.ulWord;
        //У��
        if((ulVal0 != ulVal1) || (ulVal1 != GetInvtWord(_ulVal)))
        {
            goto ErrReturn;
        }
    }
    *pulVal = pstBlockData->unVal0.ulWord;
    return TRUE;
ErrReturn:
    printf("S50 Block Data Chck Error!\r\n");
    return FALSE;
}

/*
����:
    1.��ȡָ��block�е�����,�����ض�ȡ���
����:   void
����ֵ: 
    1.TRUE: Success
    2.FALSE: Fail
����:   
    1.ucBlockAddr Block Address
���:   
    1.pulValָ����ջ����ָ��
��ע:   ��
ע��:   
    1.S50��ÿ��Block�洢16Bytes����,ʵ����Ч������Ϊ4Byte(����) + 1Byte(�����ڴ洢����Block��ַ)
      �����Ϊ�����õ��ظ�ֵ
    2.S50������С�˴洢
    3.�������ݲ�ȷ��
*/
bool S50MFBlockRead(uint8 ucBlockAddr, uint32 *pulVal)
{
    bool bRes = FALSE;
    uint8 aucRecv[S50BLOCK_DATA_SZ] = {0};

    bRes = S50MFRecvBlockData(ucBlockAddr, &aucRecv[0]);//���Ͷ���ָ��,������Block����
    if(FALSE == bRes)
    {
        printf("S50 Block Data Recv Error!\r\n");
        goto ErrorReturn;
    }
    bRes = S50MFUnPackReadData((stS50DataBlockDef *)aucRecv, pulVal);
    return bRes;
ErrorReturn:
    return FALSE;
}

/*
����:
    1.��S50������MIFARE Write����,�ȴ�Ӧ�𣬲������յ�������д�뻺��
����:   
    1.ucBlockAddr,Block��ַ
    2.pucSend ָ���͵����ݵ�ָ��
����ֵ: 
    1.TRUE:  Success
    2.FALSE: Fail
����:   void
���:   void
��ע:   ��
ע��:   
    1.S50��ÿ��Block�洢16Bytes����,ʵ����Ч������Ϊ4Byte(����) + 1Byte(�����ڴ洢����Block��ַ)
      �����Ϊ�����õ��ظ�ֵ
    2.S50������С�˴洢
    3.�������ݲ�ȷ��
*/
static bool S50MFSendBlockData(uint8 ucBlockAddr, uint8 *pucSend)
{
    bool bRes = FALSE;
    uint8 aucSend[2] = {0};
    uint8 i = 0;

    aucSend[i++] = CMD_MIFARE_WRITE;
    aucSend[i++] = ucBlockAddr;
    bRes = S50SendStandFrame(aucSend, S50BLOCK_DATA_SZ, aucSend, sizeof(aucSend));
    return bRes;
}

/*
����:
    1.����ulVal��ȡд��S50��1��Block�����ݰ�
����:   
    1.ulVal
    2.pucWrite
����ֵ: void
����:   
    1.ulVal��д��block����
���:   
    1.pucWrite�����ɺ�����ָ��
��ע:   ��MIFARE 
ע��:
    1.S50��ÿ��Block�洢4Bytes��Ч����
    2.S50������С�˴洢
    3.�������ݲ�ȷ��
*/
static void S50MFPackWriteData(uint32 ulVal, uint8 *pucSend)
{
    stS50DataBlockDef *pstBlockData = (stS50DataBlockDef *)pucSend;

    pstBlockData->unVal0.ulWord = ulVal;
    pstBlockData->_unVal.ulWord = GetInvtWord(ulVal);
    pstBlockData->unVal1.ulWord = ulVal;

    /*����Block ��δ����*/
    pstBlockData->ucAddr0 = pstBlockData->ucAddr1 = 0;
    pstBlockData->_ucAddr0 = pstBlockData->_ucAddr1 = GetInvtByte(0);
    return;
}
/*
����:
    1.ָ��block�е�����,�����ض�ȡ���
����:   void
����ֵ: 
    1.TRUE: Success
    2.FALSE: Fail
����:   
    1.ucBlockAddr Block Address
    2.ָ����ջ����ָ��
���:   void
��ע:   ��MIFARE 
ע��:   
    1.S50��ÿ��Block�洢16Bytes����,ʵ����Ч������Ϊ4Byte(����) + 1Byte(�����ڴ洢����Block��ַ)
      �����Ϊ�����õ��ظ�ֵ
    2.S50������С�˴洢
    3.�������ݲ�ȷ��
*/
bool S50MFBlockWrite(uint8 ucBlockAddr, uint32 ulVal)
{
    bool bRes = FALSE;
    uint8 aucSend[S50BLOCK_DATA_SZ] = {0};

    S50MFPackWriteData(ulVal, &aucSend[0]);//�����д��S50��1��Block������
    bRes = S50MFSendBlockData(ucBlockAddr, &aucSend[0]);
    return bRes;
}

#if S50DBG
#define S50TEST_BLOCK           1
/*
����:
    1.��stS50Info.aucKeyA���ΪĬ��ֵ
����:   void
����ֵ: void
����:   void
���:   void
��ע:   ��
*/
void S50TestGetKeyA(void)
{
    memset(stS50Info.aucKeyA, 0xff, S50_KEYA_SZ);
    return;
}

/*
����:
    1.S50����д����
����:   void
����ֵ: void
����:   void
���:   void
��ע:   ��
*/
void S50RwTest(void)
{
    uint32 ulRead = 0;
    uint32 ulWrite = 56;

    S50MFBlockRead(S50TEST_BLOCK, &ulRead);
    //S50MFBlockWrite(S50TEST_BLOCK, ulWrite);
    
    S50MFBlockRead(S50TEST_BLOCK, &ulRead);
    if(ulRead == ulWrite)
    {
        printf("S50 Read/Write Test Success!\r\n");
    }
    return;
}


/*
����:
    1.S50�������������
����:   void
����ֵ: void
����:   void
���:   void
��ע:   ��
*/
void S50Test(void)
{
    bool bRes = FALSE;

    S50TestGetKeyA();
Start:
    DelayMs(10);
    bRes = S50CardDetect();
    if(FALSE == bRes)
    {
        goto Start;
    }
    bRes = S50MFAuthent(S50TEST_BLOCK);
    if(FALSE == bRes)
    {
        goto Start;
    }
    S50RwTest();
    goto Start;
    return;
}
#endif

#ifdef __cplusplus
}
#endif


#endif /* __S50_C */


