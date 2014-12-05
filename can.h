/*
 * can.h
 *
 *  Created on: 09.07.2013
 *      Author: dietmar
 */
#ifndef CAN_H_
#define CAN_H_

#include <msp430.h>
#include <stdint.h>

#define CAN_OK                  0
#define CAN_ERR_TXBUFFERFULL    1

#define CAN_IDMAX               0x7FF 	/* max id */
#define CAN_IDMAXEXT            1024 /* FIXME */

extern void setupCANController();
extern int sendCANMessage(uint8_t *pnData, int nLen, int nID);
extern uint8_t readCANErrorFlags();
extern uint8_t readCANTransmitErrorCount();
extern int receiveCANMessage(uint8_t *pnData, int *pnLen, int *pnID, int *pnError);

#endif /* CAN_H_ */
