/*
 *
 *  ____  _   _ _   _ _____ ___ __  __ _____
 * |  _ \| | | | \ | |_   _|_ _|  \/  | ____|
 * | |_) | | | |  \| | | |  | || |\/| |  _|
 * |  _ <| |_| | |\  | | |  | || |  | | |___
 * |_| \_\\___/|_| \_| |_| |___|_|  |_|_____|
 *
 * runtime.c
 *
 * Author: dietmar millinger
 * Revision: 0.5
 *
 *   ACLK = n/a, MCLK = DCO ~16MHz, SMCLK = DCO/8(2MHz),  BRCLK = SMCLK/2
 *
 *                    MSP430G2xx3
 *                 -----------------
 *                |             P1.2|-> Data Out(UCA0SIMO)
 *          LED <-|P1.0         P1.1|<- Data In(UCA0SOMI)
 *  Slave reset <-|P1.5         P1.4|-> Serial Clock Out(UCA0CLK)
 *  /CS         <-|P1.3         P1.7|<- INT
 *                 -----------------
 */
#ifndef RUNTIME_H_
#define RUNTIME_H_

#define RT_NO_MODE			0x00
#define RT_MODE_IDLE		0x01
#define RT_MODE_SLEEP		0x02
#define RT_MODE_DEEPSLEEP	0x03

#define RT_MODE_POWERUP     0x04
#define RT_MODE_RUNNING     0x05
#define RT_MODE_TERMINATED  0x06


#define RT_TICK				  0x01		/* timer tick */
#define RT_STARTUP			0x02		/* mcu started up */
#define RT_TERMINATE		0x03		/* mcu terminates operation */
#define RT_SUSPEND			0x04		/* mcu suspends operation after this loop call */
#define RT_RESUME			  0x05		/* mcu resumed operation after sleep */
#define RT_CANRX			  0x06		/* can receive frame available */
#define RT_CANTXREADY		0x07		/* can transmission completed */
#define RT_ADC				  0x08		/* adc measurement ready */
#define RT_PININT			  0x09		/* pin triggered interrupt */

typedef struct _adcdata {
	uint16_t nValue;
	uint16_t nTimeStamp;
} ADCData;

extern void initRT(int nPeriodInClockTicks);
extern void setRTtick(int nPeriodInMicroSeconds);
extern int handleApplicationEvent(int nEvent, void *pParam);
extern void executeRTloop();
extern void setLED1();
extern void clearLED1();
extern void setLED2();
extern void clearLED2();
extern void setDOUT();
extern void clearDOUT();
extern void requestADC();
void initADC();

#endif /* RUNTIME_H_ */
