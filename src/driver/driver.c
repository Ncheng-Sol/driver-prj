#ifndef __DRIVER_C
#define __DRIVER_C

#ifdef __cplusplus
 extern "C" {
#endif 

/*************************ͷ�ļ�********************************/
#include "driver.h"
#include "bsp.h"

#include "tool/tool.c"
/************************����**********************************/
/*Ƭ������*/
#include "delay/delay.c"
#include "uart/uart.c"
#include "dma/dma.c"
#include "spi/spi.c"
#include "gpio/gpio.c"
#include "i2c/i2c.c"
#include "mem/mem.c"
#include "interrupt/interrupt.c"
#include "timer/timer.c"

void DriverInit(void)
{
    DelayInit();
    UartInit();
    MemInit();
    InterruptInit();
}

/*************************����*********************************/
#ifdef __cplusplus
             }
#endif 
#endif
