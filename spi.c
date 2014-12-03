/*
 *  ____  ____ ___
 * / ___||  _ \_ _|
 * \___ \| |_) | |
 *  ___) |  __/| |
 * |____/|_|  |___|
 *
 * Author: dietmar millinger
 * Revision: 0.4
 *
 * NOTES:
 * 1) 1Mbaud at 2MHy SMCLK
 *
 *
 *
 *   ACLK = n/a, MCLK = DCO ~16MHz, SMCLK = DCO/8 (2MHz),  BRCLK = SMCLK/2
 *
 *                    MSP430G2xx3
 *                 -----------------
 *                |             P1.2|-> Data Out (UCA0SIMO)
 *          LED <-|P1.0         P1.1|<- Data In (UCA0SOMI)
 *  Slave reset <-|P1.5         P1.4|-> Serial Clock Out (UCA0CLK)
 *  /CS         <-|P1.3         P1.7|<- INT
 *                 -----------------
 *
 */

#include "spi.h"


/**
 * Setup SPI for use with CAN transceiver
 *
 * see 16.3.1 USCI Initialization and Reset in slau144j.pdf
 *
 */
void initSPI ( )
{
	UCA0CTL1 |= UCSWRST;                     // **Initialize USCI state machine**

	UCA0CTL0 |= UCCKPL + UCMSB + UCMST + UCSYNC;  // 3-pin, 8-bit SPI master
	UCA0CTL1 |= UCSSEL_2;                     // SMCLK
	UCA0BR0 |= 0x00;                          // /1 for 2Mbaud at 2MHz SMCLK, see 15.4.3 in slau144j.pdf
	UCA0BR1 = 0;                              //
	UCA0MCTL = 0;                             // No modulation

	P1SEL = BIT1 + BIT2 + BIT4;				 /* function selection UART pins */
	P1SEL2 = BIT1 + BIT2 + BIT4;

	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**

	//IE2 |= UCA0RXIE;                          // Enable USCI0 RX interrupt
}


#define MAXRINGBUFFER 				2
#define MAXRINGBUFFERMODMASK 		0x01
#define MAXRINGBUFFERSIZE 			10		/* maximum frame size */

static volatile short nSPIReadBufferIndex= 0;
static volatile short nSPIWriteBufferIndex= 0;
static uint8_t aSPIBuffer[MAXRINGBUFFER][MAXRINGBUFFERSIZE+1];
static uint8_t * pSPIBufferWrite= 0;
static uint8_t * pSPIBufferRead= 0;
uint8_t		nSPIInputData= 0;



static int isSPIBufferDataAvailable ( void )
{
    return ( nSPIReadBufferIndex != nSPIWriteBufferIndex ) ;
}

static uint8_t * startSPIBufferRead ( )
{
    if ( nSPIReadBufferIndex == nSPIWriteBufferIndex )
    {
        /* nothing to read */
        return (uint8_t *)0;
    }

    return aSPIBuffer[nSPIReadBufferIndex];
}

static void stopSPIBufferRead ()
{
    nSPIReadBufferIndex= ( nSPIReadBufferIndex+ 1)  & MAXRINGBUFFERMODMASK;
    return;
}

static uint8_t * startSPIBufferWrite ( )
{
    int        nWriteNext= 0;

    nWriteNext= ( nSPIWriteBufferIndex+ 1)  & MAXRINGBUFFERMODMASK;
    if ( nWriteNext == nSPIReadBufferIndex)
    {
        /* no space left */
        return (uint8_t*)0;
    }

    aSPIBuffer[nSPIWriteBufferIndex][0]= 0x00;

    return aSPIBuffer[nSPIWriteBufferIndex];
}

static void stopSPIBufferWrite ()
{
	nSPIWriteBufferIndex= ( nSPIWriteBufferIndex+ 1)  & MAXRINGBUFFERMODMASK;
    return;
}



void waituntilSPITXBufferFree ()
{
    while ( ( IFG2 & UCA0TXIFG ) == 0 )
    {
	    /* TODO: handle infinite loop situation */
    }

    return;
}



void transmitSPIByte ( uint8_t nByte )
{
	while ( ( IFG2 & UCA0TXIFG ) == 0 )
	{
		/* TODO: handle infinite loop situation */
	}

	UCA0TXBUF= nByte;

	return;
}


void transmitSPIData ( uint8_t * pData, int nLen )
{
	short 	nI= 0;

	for ( ; nI < nLen; nI ++ )
	{
		transmitSPIByte ( pData[nI] );
	}
}




static volatile short nSPIRXPos= -1;
static short nSPIRXMax= 0;
static int nSPIReceiveError= 0;

int receiveSPIData ( uint8_t * pData, int nLen )
{
	int 	nLoopCount= nLen;
	int 	nOldData= 0;

	/* clear receive interrupt */
	//uint8_t		nInputData= UCA0RXBUF;


	/* wait until transceiver is no longer active */
	while ( UCA0STAT & 0x01 )
	{
		/* TODO: handle infinite loop situation */
	}

	__delay_cycles (80);


	/* enable SPI interrupt */
	nOldData= UCA0RXBUF;
	IE2 |= UCA0RXIE;


	/* then make receiver sharp */
	pSPIBufferWrite= 0;
	nSPIRXMax= nLen;
	nSPIRXPos= 0;

	/* transmit fake data bytes to stimulate transmission of slave */
	while ( nLoopCount-- )
	{
		transmitSPIByte ( 0x81 );
	}


	nLoopCount= 600;
	//while ( nSPIRXPos != nSPIRXMax && nLoopCount-- )
	while ( nSPIRXPos != nSPIRXMax && nLoopCount-- )
	{
		__delay_cycles (1);
	}

	pData[0]= nSPIInputData;

//	pSPIBufferRead= startSPIBufferRead ();
//	if ( pSPIBufferRead )
	{
//		short 	nI= 0;

		/* copy parts of frame into out buffer */

//		for ( ; nI < nSPIRXPos; nI ++ )
		{
//			pData[nI]= pSPIBufferRead[nI];
		}

//		stopSPIBufferRead ();

	}
//	else
	{
//		nSPIReceiveError++;
	}

	/* disable SPI interrupt */
	IE2 &= ~UCA0RXIE;

	return nSPIRXPos;
}



static void handleSPIRXInterrupt ( uint8_t nData )
{
	if ( nSPIRXPos == 0 )
	{
		pSPIBufferWrite= startSPIBufferWrite ();
	}

	if ( pSPIBufferWrite )
	{
		if ( nSPIRXPos < MAXRINGBUFFERSIZE )
		{
			pSPIBufferWrite[nSPIRXPos++]= nData;
		}

		if ( nSPIRXPos >= nSPIRXMax )
		{
			/* close this buffer and allow reading */

			stopSPIBufferWrite ();
			pSPIBufferWrite= 0;
		}
	}
}



/**
 * SPI receive interrupt
 */

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCIA0RX_ISR(void)
{
	/* read receive register: clears interrupt */
	nSPIInputData= UCA0RXBUF;
	nSPIRXPos++;

	//clearLED1 ();

	/* handle data */
	//handleSPIRXInterrupt ( nSPIInputData );

	//setLED1 ();



}














