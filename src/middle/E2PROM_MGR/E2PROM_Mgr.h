#ifndef __E2PROM_MGR_H
#define  __E2PROM_MGR_H

#ifdef __cplusplus
 extern "C" {
#endif 
/*
˵��:
    1.E2PROM ������س���
*/
/****************************************************************************************/
#define E2PROM_DEFAULT_VAL       0xff//ִ��E2PROM ��ʼ��ʱд��E2PROM��ֵ

/****E2PROM�洢��������***/
#pragma pack(1)
typedef struct{
uint8 S50KeyA[S50_KEYA_SZ];
}stE2promIndexDef;
#pragma pack()

/********E2PROM�洢�����ܿռ�********/
#define E2PROM_INDEX_SZ		(S50_KEYA_SZ)

#if (E2PROM_CAP < E2PROM_INDEX_SZ)
#error "E2PROM Capacity Err!"
#endif

/*��ȡObj��E2PROM�еĴ洢��ַ(ObjΪstE2promIndexDef�еĳ�Ա)*/
#define GetE2promAddr(Obj)         offsetof(stE2promIndexDef, Obj)

extern void E2PROM_Default(void);

#ifdef __cplusplus
}
#endif


#endif /* __E2PROM_MGR_H */


