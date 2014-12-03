// ____  ____  ____  _   _ _____     _____ __  __ _____ ______   __
//|  _ \|  _ \/ ___|| | | | ____|   | ____|  \/  |_   _|  _ \ \ / /
//| | | | |_) \___ \| | | |  _|     |  _| | |\/| | | | | |_) \ V /
//| |_| |  _ < ___) | |_| | |___    | |___| |  | | | | |  __/ | |
//|____/|_| \_\____/ \___/|_____|___|_____|_|  |_| |_| |_|    |_|
//                             |_____|
//
//
//   ACLK = n/a, MCLK = DCO ~16MHz, SMCLK = DCO/8 , BRCLK = SMCLK/4
//
//
//                    MSP430G2xx3
//                 -----------------
//                |             P1.2|-> Data Out (UCA0SIMO)
//          LED <-|P1.0         P1.1|<- Data In (UCA0SOMI)
//  Slave reset <-|P1.5         P1.4|-> Serial Clock Out (UCA0CLK)
//  /CS         <-|P1.3         P1.7|<- INT
//                 -----------------

#include <msp430.h>
#include <stdint.h>
#include "spi.h"
#include "can.h"
#include "runtime.h"
#include "mcp2515.h"

/* tick period in timer clock cycles a 2Mhz: 1ms == 2000 */

#define CL_PERIOD				((int)2000)

#define SQUARE_WAVE_PERIOD 		((int)200)
#define SQUARE_WAVE_TICKS 		(1 * (SQUARE_WAVE_PERIOD/2))
#define ADC_SAMPLE_TICKS		((int) 2)
#define RECV_MSG_SAMPLE_TICKS	((int) 1)
#define MAX_UPDATE_DELAY_TICKS 	((int) 3)
#define TARGET_VALUE_1500_MV	((uint16_t) 433)
#define TARGET_VALUE_1000_MV	((uint16_t) 288)
#define TARGET_VALUE  			TARGET_VALUE_1500_MV

int main (void)
{
	/* setup board */
	initRT ( CL_PERIOD );

    /* start RT handling */
    executeRTloop ();
}

void toggleLed1() {
	static uint8_t old_state = 0;

	if (old_state == 0) {
		setLED1();
		old_state = 1;
	} else {
		clearLED1();
		old_state = 0;
	}
}

void taskSecondLED(void) {
	static int nCount = 0;
	static int nToggle = 0;

	nCount++;
	if ( nCount > 1000 )
	{
		nCount= 0;
        if ( nToggle > 0 )
        {
            setLED2 ();
            setLED1 ();
            nToggle= 0;
        }
        else
        {
            clearLED2 ();
            clearLED1 ();
            nToggle= 1;
        }
	}
}

void taskSquarewave(void) {
	static int nSquareWaveFrequencyCount = 0;
	static int SquareWaveHigh = 0;

	nSquareWaveFrequencyCount++;
	if (nSquareWaveFrequencyCount >= SQUARE_WAVE_TICKS) {
		if (SquareWaveHigh) {
			// Toggle pin low
			clearDOUT();
			SquareWaveHigh = 0;
		} else {
			// Toggle pin high
			setDOUT();
			SquareWaveHigh = 1;
		}
		nSquareWaveFrequencyCount = 0;
	}
}

void taskRequestADC(void) {
	static int nSampleTime = 0;

    // request ADC sample
    nSampleTime++;
    if (nSampleTime >= ADC_SAMPLE_TICKS) {
        requestADC();
        nSampleTime = 0;
    }
}


void taskReceiveMsg(void) {
	static int nSampleTime = 0;
	static int nTicksSinceLastUpdate = 0;
	uint16_t val;
	int len, id;

    nSampleTime++;
    nTicksSinceLastUpdate++;
    if (nSampleTime >= RECV_MSG_SAMPLE_TICKS) {

    	if (receiveCANMessage((uint8_t*)&val, &len, &id, 0) == 1) {
        // received a CAN message -> clear CAN Error
    		setLED2();
    		nTicksSinceLastUpdate = 0;

    		// control loop
        if (val < TARGET_VALUE) {
          setDOUT();
        } else {
          clearDOUT();
        }

        // error detection
        if (data->nValue < TARGET_VALUE_1000_MV) {
          setLED1();
        } else {
          clearLED1();
        }
    	}
        nSampleTime = 0;
    }

    if (nTicksSinceLastUpdate >= MAX_UPDATE_DELAY_TICKS) {
    	// CAN communication error, received no messages. GREEN led off
    	clearLED2();
    }
}

void taskTestComDelaySlave(void) {
	int id;
	int len;
	uint8_t data[8];

	if (receiveCANMessage((uint8_t*)data, &len, &id, 0) == 1) {
		// set output
		if (len >= 1 && data[0] == 0) {
			clearDOUT();
		} else if (len >= 1 && data[0] == 1) {
			setDOUT();
		} else {
			setLED1();
		}
	}
}

int handleApplicationEvent ( int nEvent, void * pParam )
{
	int		nRequestMode= RT_MODE_IDLE;

	switch ( nEvent )
	{
	case RT_TICK:

		//taskSecondLED();
		//taskSquarewave();
		//taskRequestADC();
		taskReceiveMsg();
		//taskTestComDelaySlave();

		break;

	case RT_ADC:
	{
		ADCData *data = (ADCData *)pParam;
		if (data->nValue < TARGET_VALUE) {
			setDOUT();
			setLED1();
			clearLED2();
		} else {
			clearDOUT();
			clearLED1();
			setLED2();
		}

		break;
	}

	case RT_STARTUP:
        initADC();
        setupCANController();

        clearLED1();
        clearLED2();
		break;

	case RT_TERMINATE:
		break;
    }

	return nRequestMode;
}
