#ifndef __S50_REG_H
#define __S50_REG_H

#ifdef __cplusplus
 extern "C" {
#endif 
/*
�ļ�˵��:
    1.������S50����Block��Access conditions����Ϣ
��ע:
    1. The 1024 X 8 bit EEPROM memory is organized in 16 sectors of 4 blocks
    2. One block contains 16 bytes
    3. Block0 Sector0 is Manufacturer Data
    4. 
*/

#define S50VAL_DATA_SZ      4//1��Block�д洢����Ч�����ݳ���
#define S50BLOCK_DATA_SZ    16//1��Block�д洢�������ݳ���

/*
stS50DataBlockDef 
˵��: 
    1.������DataBlock(Block0 - Block2)���������
    2.С�˴洢
��ע:
    1.S50��ÿ��Block�洢16Bytes����,ʵ����Ч������Ϊ4Byte(����) + 1Byte(�����ڴ洢����Block��ַ)
      �����Ϊ�����õ��ظ�ֵ
      �����ʽ����:
        value(4Byte) + /value(4Byte) + value(4Byte) + addr + /addr + addr + /addr
        ����,addr�������ڼ�¼����Block��ַ,/value��ʾvalue��λȡ�����ֵ
*/
typedef struct{
unWordDef unVal0;//non-inverted
unWordDef _unVal;//inverted Data
unWordDef unVal1;//non-inverted

uint8 ucAddr0;//non-inverted
uint8 _ucAddr0;//inverted Data
uint8 ucAddr1;//non-inverted
uint8 _ucAddr1;//inverted Data
}stS50DataBlockDef;

#ifdef __cplusplus
}
#endif


#endif /* __S50_REG_H */

