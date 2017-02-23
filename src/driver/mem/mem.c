#ifndef __MEM_C
#define __MEM_C

#ifdef __cplusplus
 extern "C" {
#endif 

#define MEM_SIZE                (60 * 1024)//pool ������60k

#define MEM_BIG_NUM              10//1024 byte block�������������
#define MEM_MIDDLE_NUM           40//512 byte block�������������
#define MEM_SMALL_NUM            80//256 byte block�������������
#define MEM_SMALL64_NUM          160//64 byte block�������������  

/*BitMap ��ռ�ڴ�word(uint32)����*/
#define MEM_BIG_BNUM              (MEM_BIG_NUM / sizeof(uint32) + ((0 == (MEM_BIG_NUM % sizeof(uint32))) ? 0 : 1))//1024 byte block���������������Ӧ��bitmap ����
#define MEM_MIDDLE_BNUM           (MEM_MIDDLE_NUM / sizeof(uint32) + ((0 == (MEM_MIDDLE_NUM % sizeof(uint32))) ? 0 : 1))//512 byte block���������������Ӧ��bitmap ����
#define MEM_SMALL_BNUM            (MEM_SMALL_NUM / sizeof(uint32) + ((0 == (MEM_SMALL_NUM % sizeof(uint32))) ? 0 : 1))//256 byte block���������������Ӧ��bitmap ����
#define MEM_SMALL64_BNUM          (MEM_SMALL64_NUM / sizeof(uint32) + ((0 == (MEM_SMALL64_NUM % sizeof(uint32))) ? 0 : 1))//64 byte block���������������Ӧ��bitmap ����

#if (((MEM_BIG_NUM * 1024) + (MEM_MIDDLE_NUM * 512) + (MEM_SMALL_NUM * 256) + (MEM_SMALL64_NUM * 64)) != MEM_SIZE)
#error  Memory Cfg err!//�ڴ��������������ڴ��֮��һ��
#endif

/*memory manage bitmap*/
typedef struct{
uint32 aulMemBig[MEM_BIG_BNUM];//1024
uint32 aulMemMiddle[MEM_MIDDLE_BNUM];//512
uint32 aulMemSmall[MEM_SMALL_BNUM];//256
uint32 aulMemSmall64[MEM_SMALL64_BNUM];//64
}stMemBitMapDef;

typedef struct{
uint8 aulMemBig[MEM_BIG_NUM][MEM_BIG_SZ];//1024 * 10
uint8 aulMemMiddle[MEM_MIDDLE_NUM][MEM_MIDDLE_SZ];//512 * 40
uint8 aulMemSmall[MEM_SMALL_NUM][MEM_SMALL_SZ];//256 * 80
uint8 aulMemSmall64[MEM_SMALL64_NUM][MEM_SMALL64_SZ];//64 * 160
}stMemBuffDef;

typedef const struct{
uint32 *pulBitMap;//stMemBitMapDef �ж�Ӧ�������ʼ��ַ
uint8  ucBitMapLen;//stMemBitMapDef �ж�Ӧ���鳤��
}stMemManageDef;

static stMemBitMapDef aulMemBitMap = {0};
static uint8  aucMemPool[MEM_SIZE] = {0};

stMemManageDef stMemManage[] = {
    {
        &aulMemBitMap.aulMemBig[0],
        MEM_BIG_BNUM,
    },
    {
        &aulMemBitMap.aulMemMiddle[0],
        MEM_MIDDLE_BNUM,
    },
    {
        &aulMemBitMap.aulMemSmall[0],
        MEM_SMALL_BNUM,
    },
    {
        &aulMemBitMap.aulMemSmall64[0],
        MEM_SMALL64_BNUM,
    },
};

/*��ȡ1��word�е�һ��0����λ��*/
static uint8 MemGetFirstZeroWord(uint32 ulData)
{
    uint8 i      = 0;
    uint32 ulTmp = 1;

    for(i = 0; i < 32; i++)
    {
        if ((ulData & ulTmp) == 0)
        {
            break;
        }
        ulTmp <<= 1;
    }
    return i;
}

/*
    ��ȡbitmap�е�һ��������λ��
    para: 
        pulData   Ϊָ��bitmap ��Ԫ�ص�ָ��
        ucDataLen ��bitmap ����
        pucIndex  ���ڷ��ص�һ������bitmap�����е�λ��
        pucBitIndex  ���ڷ��ص�һ������bitmap����Ԫ���е�bitλ��
    ����:
        FALSE ʧ��(�ڴ�����)
        TRUE  �ɹ�
*/
static bool MemGetFirstZero(const uint32 * const pulData, const uint8 ucDataLen, uint8 *const pucIndex, uint8 *const pucBitIndex)
{
    bool  res = FALSE;
    uint8 i = 0;

    for (i = 0; i < ucDataLen; i++)
    {
        if(pulData[i] == 0xffffffff)
        {//��word bitmap
            continue;
        }
        res = TRUE;
        *pucBitIndex = MemGetFirstZeroWord(pulData[i]);
        break;
    }
    *pucIndex = i;

    return res;
}

void MemInit(void)
{
    memset(&aulMemBitMap, 0x00, sizeof(aulMemBitMap));
    return;
}

/*
��ȡָ�����͵��ڴ棬������
para:
      enType,   Ҫ��ȡ���ڴ泤�ȶ�Ӧ������
      usIndex,  ���ڴ洢��ȡ��blockλ�õ�ָ��(�ͷ��ڴ�ʱʹ��)
*/
void *MemMalloc(enMemTypeDef enType, uint16 *usIndex)
{
    void *pMem = NULL;
    uint8 ucStep       = 0;
    stMemManageDef *stManage = &stMemManage[enType];
    /*bitmap ���*/
    uint8 ucIndex = 0;//stMemBitMapDef����Ӧ�����е�λ��
    uint8 ucBitIndex  = 0;//stMemBitMapDef��Ӧ����Ԫ���е�λ��

    if (FALSE == MemGetFirstZero(stManage->pulBitMap, stManage->ucBitMapLen, &ucIndex, &ucBitIndex))
    {//���ڴ����
        ucStep = 1;
        goto ErrReTurn;
    }
    stManage->pulBitMap[ucIndex] |= (1 << ucBitIndex);
    *usIndex = ucIndex * 32 + ucBitIndex;
    switch(enType)
    {
        case enMemBig:
        {
            pMem = (void *)&(((stMemBuffDef *) aucMemPool)->aulMemBig[*usIndex][0]);
            memset(pMem, 0x00, MEM_BIG_SZ);
            break;
        }
        case enMemMiddle:
        {
            pMem = (void *)&(((stMemBuffDef *) aucMemPool)->aulMemMiddle[*usIndex][0]);
            memset(pMem, 0x00, MEM_MIDDLE_SZ);
            break;
        }
        case enMemSmall:
        {
            pMem = (void *)&(((stMemBuffDef *) aucMemPool)->aulMemSmall[*usIndex][0]);
            memset(pMem, 0x00, MEM_SMALL_SZ);
            break;
        }
        case enMemSmall64:
        {
            pMem = (void *)&(((stMemBuffDef *) aucMemPool)->aulMemSmall64[*usIndex][0]);
            memset(pMem, 0x00, MEM_SMALL64_SZ);
            break;
        }
        default:
        {
            break;
        }
    }
    return pMem;
ErrReTurn:
    PrintErr(MemMalloc, ucStep, 0);
    return NULL;
}

void MemFree(enMemTypeDef enType, uint16 usIndex)
{
    stMemManageDef *stManage = &stMemManage[enType];

    stManage->pulBitMap[usIndex / 32] &= ~(1 << (usIndex % 32));
    return;
}

#ifdef __cplusplus
}
#endif
#endif

