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

/* tick period in timer clock cycles a 4Mhz: 1ms == 4000 */
#define CL_PERIOD               ((int)4000)

#define NODE_NAME 				(Actor1)
#define VOTE_PERIOD				((int) 5)

int main(void)
{
	/* setup board */
	initRT(CL_PERIOD);

  /* start RT handling */
  executeRTloop();
}

typedef enum {
	LOREM
} Node;

typedef enum {
	PSync,
	PDutyCycle,
	PError
} PacketType;

void packet_received(Node sender, uint16_t timestamp, PacketType type, void *packet)
{
	switch (type) {
	case PSync:
		break;
	case PDutyCycle:
	{
		// TODO: is Error in DutyCycle packet?
		uint8_t * duty_cycle = (uint8_t *)packet;
		actor_add_voter_value((int) sender, 0, *duty_cycle);
		break;
	}
	case PError:
	{
		uint8_t * error = (uint8_t *)packet;
		actor_add_voter_value((int) sender, *error, 0);
		break;
	}
	default:
		break;
	}
}

void taskStartVoting() {
	static uint8_t vote_counter = 0;

	vote_counter++;
	if (vote_counter >= VOTE_PERIOD) {
		actor_vote();
		vote_counter = 0;
	}
}

int handleApplicationEvent(int nEvent, void *pParam)
{
	int nRequestMode= RT_MODE_IDLE;

	switch(nEvent) {
    case RT_TICK:
      //com_tick_sync();
      taskStartVoting();
      break;

    case RT_ADC:
      break;

    case RT_STARTUP:
          //comInit(this_node);
          //com_handle_receive(actor_receive_cb);
          //setupCANController();
    	  //com_init(NODE_NAME);
    	  //com_handle_receive(packet_received);
          actor_init();

          clearLED1();
          clearLED2();
      break;

    case RT_TERMINATE:
      break;
  }

	return nRequestMode;
}
