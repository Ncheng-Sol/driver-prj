#ifndef __BEEP_H
#define __BEEP_H

#ifdef __cplusplus
 extern "C" {
#endif 

#define BEEP_DBG                0

#define BEEP_SHORT_T            100//��Beep������ʱ��(ms)
#define BEEP_LONG_T             500//��Beep������ʱ��(ms)
#define BEEP_DOUBLE_T           200//��Beep������ʱ��(ms)

/************GPIO************/
#define BEEP_PORT               GPIOB
#define BEEP_PIN                GpioPin8
#define BEEP_ENR                RCC_APB2EN(GPIOB)

/*beep �������Ͷ���*/
typedef enum{
BeepShort,
BeepLong,
BeepDouble,
}enBpTypedef;

extern void BeepInit(void);
extern void Beep(enBpTypedef enBpType);
#if BEEP_DBG
extern void BeepTest(void);
#endif

#ifdef __cplusplus
}
#endif
#endif /* __BEEP_H */


