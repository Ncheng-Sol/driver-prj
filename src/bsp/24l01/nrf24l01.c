#ifndef __24L01_C
#define __24L01_C

#ifdef __cplusplus
 extern "C" {
#endif 

#include "nrf_cmd.h"
#include "nrf_reg.h"
#include "nrf_cfg.h"


/*********ȫ�ֱ���*************/
static uint8 ucAddrWidth = 0;//RX/TX Address field width 

/*
����:
    1.NRF24L01 SPI Init
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void NrfSpiInit(void)
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
        SPI_InitStruct.SPI_BaudRatePrescaler  = NRF_SPI_BR;
        SPI_InitStruct.SPI_FirstBit  = SPI_FirstBit_MSB;
        SPI_InitStruct.SPI_CRCPolynomial  = 1;

        stSpiInitInfo.Ch             = NRF_SPI_CH;
        stSpiInitInfo.SPI_InitStruct = &SPI_InitStruct;
        stSpiInitInfo.pSpiReadFunc   = RC522SpiRead;
        SpiInit(&stSpiInitInfo);
        return;
}

static void NrfIsr(void)
{
    
    return;
}

/*
����:
    1.NRF24L01 GPIO Init
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void NrfGpioInit(void)
{
    GpioInitDef GPIO_InitStruct;

    /*interrupt pin and CE pin Rcc Init*/
    RCC->APB2ENR |= NRF_CE_ENR | NRF_INT_ENR;

    /*CE pin cfg*/
    GPIO_InitStruct.GpioPin     = NRF_CE_PIN;
    GPIO_InitStruct.GPIO_Mode   = GpioModeOutPP;
    GPIO_InitStruct.GPIO_Speed  = GpioSpeed10MHz;
    GpioInit(NRF_CE_PORT, &GPIO_InitStruct);

    /*interrupt pin cfg*/
    GPIO_InitStruct.GpioPin     = NRF_INT_PIN;
    GPIO_InitStruct.GPIO_Mode   = GpioModeIPU;
    GPIO_InitStruct.GPIO_Speed  = GpioInput;
    GpioInit(NRF_INT_PORT, &GPIO_InitStruct);
    
    return;
}

/*
����:
    1.NRF24L01 Interrupt Init
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void NrfIntInit(void)
{
    EXTI_InitTypeDef EXTI_InitStruct;

    /*interrupt cfg*/
    EXTI_InitStruct.EXTI_Line    = NRF_INT_LINE;
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    EXTI_InitStruct.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_Init(&EXTI_InitStruct);
    IrqRegester(NRF_INT_IRQn, NrfIsr);
    return;
}

/*
����:
    1.NRF24L01 Register Init
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
static void NrfRegInit(void)
{
    
    return;
}

/*
����:
    1.NRF24L01 Init
����:   void
����ֵ: void
����:   void
���:   void
��ע:   void
ע��:   void
*/
void NrfInit(void)
{
    NrfSpiInit();
    NrfGpioInit();
    NrfIntInit();
    NrfRegInit();
    return;
}

/*
����:
    1.NRF24L01 Write One Byte Data into Register
����:   
    1.ucAddr:  Register ��ַ
    2.ucWrite: Ҫд�������(1 Byte)
����ֵ: 
    0:      success
    Ohers:  Error Code
����:   void
���:   void
��ע:   void
ע��:   void
*/
uint16 NrfWrite1Byte(uint8 ucAddr, uint8 ucWrite)
{
    uint16 usErr = 0;
    //usErr = SpiDataWrite();
    return usErr;
}

/*
����:
    1.NRF24L01 Write n Bytes Data into Register
����:   
    1.ucAddr:     Register ��ַ
    2.pucWrite:   ָ��д�뻺���ָ��
    3.ucWriteLen: Ҫд������ݳ���
����ֵ: 
    0:      success
    Ohers:  Error Code
����:   void
���:   void
��ע:   void
ע��:   void
*/
uint16 NrfWriteNByte(uint8 ucAddr, uint8 *pucWrite, uint8 ucWriteLen)
{
    uint16 usErr = 0;
    //usErr = SpiDataWrite();
    return usErr;
}

/*
����:
    1.NRF24L01 Read One Byte Data From Register
����:   
    1.ucAddr:     Register ��ַ
    2.pucRead:    ָ���ȡ�����ָ��
����ֵ: 
    0:      success
    Ohers:  Error Code
����:   void
���:   void
��ע:   void
ע��:   void
*/
uint16 NrfRead1Byte(uint8 ucAddr, uint8 *pucRead)
{
    uint16 usErr = 0;
    //usErr = SpiDataWrite();
    return usErr;
}

/*
����:
    1.NRF24L01 Read n Bytes Data From Register
����:   
    1.ucAddr:    Register ��ַ
    2.pucRead:   ָ���ȡ�����ָ��
    3.ucReadLen: Ҫ��ȡ�����ݳ���
����ֵ: 
    0:      success
    Ohers:  Error Code
����:   void
���:   pucRead
��ע:   void
ע��:   void
*/
uint16 NrfReadNByte(uint8 ucAddr, uint8 *pucRead, uint8 ucReadLen)
{
    uint16 usErr = 0;
    //usErr = SpiDataWrite();
    return usErr;
}



#ifdef __cplusplus
}
#endif
#endif//__24L01_C
