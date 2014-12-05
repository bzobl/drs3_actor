// ____  ____  ____  _   _ _____     _____ __  __ _____ ______   __
//|  _ \|  _ \/ ___|| | | | ____|   | ____|  \/  |_   _|  _ \ \ / /
//| | | | |_) \___ \| | | |  _|     |  _| | |\/| | | | | |_) \ V /
//| |_| |  _ < ___) | |_| | |___    | |___| |  | | | | |  __/ | |
//|____/|_| \_\____/ \___/|_____|___|_____|_|  |_| |_| |_|    |_|
//                             |_____|
//
//
//   ACLK = n/a, MCLK = DCO ~16MHz, SMCLK = DCO/4 , BRCLK = SMCLK/4
//
//
//                    MSP430G2xx3
//                 -----------------
//                |             P1.2|-> Data Out(UCA0SIMO)
//          LED <-|P1.0         P1.1|<- Data In(UCA0SOMI)
//  Slave reset <-|P1.5         P1.4|-> Serial Clock Out(UCA0CLK)
//  /CS         <-|P1.3         P1.7|<- INT
//                 -----------------

#include <msp430.h>
#include <stdint.h>

#include "actor.h"
#include "spi.h"
#include "can.h"
#include "runtime.h"
#include "mcp2515.h"

/* tick period in timer clock cycles a 4Mhz: 1ms == 4000 */
#define CL_PERIOD               ((int)4000)

int main(void)
{
	/* setup board */
	initRT(CL_PERIOD);

  /* start RT handling */
  executeRTloop();
}

int handleApplicationEvent(int nEvent, void *pParam)
{
	int nRequestMode= RT_MODE_IDLE;

	switch(nEvent) {
    case RT_TICK:
      break;

    case RT_ADC:
      break;

    case RT_STARTUP:
          //setupCANController();
          actor_init();

          clearLED1();
          clearLED2();
      break;

    case RT_TERMINATE:
      break;
  }

	return nRequestMode;
}
