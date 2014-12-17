// ____  ____  ____  _   _ _____     _____ __  __ _____ ______   __
//|  _ \|  _ \/ ___|| | | | ____|   | ____|  \/  |_   _|  _ \ \ / /
//| | | | |_) \___ \| | | |  _|     |  _| | |\/| | | | | |_) \ V /
//| |_| |  _ < ___) | |_| | |___    | |___| |  | | | | |  __/ | |
//|____/|_| \_\____/ \___/|_____|___|_____|_|  |_| |_| |_|    |_|
//                             |_____|
//
//
//   ACLK = n/a, MCLK = DCO ~16MHz, SMCLK = DCO/4 , BRCLK = SMCLK/4
// //
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

#include "com.h"

/* tick period in timer clock cycles a 4Mhz: 1ms == 4000 */
#define CL_PERIOD               ((int)4000)

#define MACRO_TICKS				((int) 1)
#define VOTE_SLOT				((int) 0)

static enum Node my_node = Actuator1;
static int slot_now = 1;

int main(void)
{
	/* setup board */
	initRT(CL_PERIOD);

  /* start RT handling */
  executeRTloop();
}

void packet_received_cb(enum Node sender, uint16_t timestamp, enum PacketType type, void *packet)
{
	switch (type) {
	case PSync:
		break;
	case PDutyCycle:
	{
		uint8_t * duty_cycle = (uint8_t *)packet;
		if (actor_add_voter_value((int) sender, 0, *duty_cycle)) {
			// red led on, when a value is overwritten
			setLED1();
		}
		break;
	}
	default:
		break;
	}

	//TODO set slot_now;
	slot_now = 1;
}

void taskStartVoting() {
	// TODO set slot_now!
	if (slot_now == VOTE_SLOT) {
		if (actor_vote()) {
			// green led on
			clearLED2();
		} else {
			// green led off
			setLED2();
		}

		// clear red led of previous voting
		clearLED1();
	}
}

int handleApplicationEvent(int nEvent, void *pParam)
{
	int nRequestMode= RT_MODE_IDLE;
	enum TickStatus status;

	switch(nEvent) {
    case RT_TICK:
      status = com_tick();
      switch (status) {
      	  case Error:
      		  // set red led in case of error
      		  setLED1();
      		  // set error of sender in current tick?
      		  //actor_add_voter_value((int) sender, *error, 0);
      		  break;

      	  case BadPktType:
      		  // TODO ?
      		  break;

      	  case BadPacket:
      		  // TODO ?
      		  break;

      	  case NotConnected:
      		  // TODO ?
      		  break;

      	  case Connected:
      		  // TODO ?
      		  break;

      	  default:
      		  break;
      }

      taskStartVoting();
      break;

    case RT_ADC:
      break;

    case RT_STARTUP:
    	  com_init(MACRO_TICKS, my_node);
    	  com_register_receiving_handler(packet_received_cb);
          actor_init();

          clearLED1();
          clearLED2();
      break;

    case RT_TERMINATE:
      break;
  }

	return nRequestMode;
}
