#ifndef __RC522_CMD_H
#define __RC522_CMD_H

#ifdef __cplusplus
 extern "C" {
#endif 

#define RANDOM_SZ           10//RC522 Generate RandomID���������������������

typedef enum{
    CmdIdle = 0,
    CmdMem,//stores 25 bytes into the internal buffer
    CmdGRID,//generates  a 10-byte random ID number
    CmdCRC,//activates the CRC coprocessor or performs a self test
    CmdTrans,//transmits data from the FIFO buffer
    CmdNoCmd = 7,/*no command change, can be used to modify the 
            CommandReg register bits without affecting the command, 
            for example, the PowerDown bit*/
    CmdRecv,//activates the receiver circuits
    CmdTransCv = 0x0c,/*transmits data from FIFO buffer to antenna and automatically 
            activates the receiver after transmission*/
    CmdMFA  = 0x0e,//performs the MIFARE standard authentication as a reader
    CmdSoftRst,//resets the MFRC522
}enCmdTypeDef;

typedef const struct{
    enCmdTypeDef enCmd;
    bool bIsAutoEnd;//�����Զ�����
}stCmdAttrDef;

stCmdAttrDef astCmdAttr[] = {//�洢��Ӧ�����Ƿ��Զ�����(�������ж��Ƿ���Ҫ�ȴ�IdleIRq)
    {CmdIdle, FALSE},//Idle
    {CmdMem,  TRUE},//Mem
    {CmdGRID, TRUE},//Generate RandomID
    {CmdCRC,  FALSE},//CalcCRC
    {CmdTrans, TRUE},//Transmit
    {CmdNoCmd, FALSE},//NoCmdChange
    {CmdRecv,  TRUE},//Receive
    {CmdTransCv, FALSE},//Transceive
    {CmdMFA, FALSE},//MFAuthent
    {CmdSoftRst, TRUE},//SoftReset
};

#ifdef __cplusplus
}
#endif
#endif
