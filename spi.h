/*
 * spi.h
 *
 *  Created on: 27.09.2013
 *      Author: dietmar
 */

#ifndef SPI_H_
#define SPI_H_

#include <msp430.h>
#include <stdint.h>

extern void initSPI ( );
extern int receiveSPIData ( uint8_t * pData, int nLen );
extern void transmitSPIData ( uint8_t * pData, int nLen );
extern void transmitSPIByte ( uint8_t nByte );
extern void waituntilSPITXBufferFree ();

#endif /* SPI_H_ */
