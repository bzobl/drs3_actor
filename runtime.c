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
 * Revision: 0.4
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
 */


#include <msp430.h>
#include <stdint.h>
#include "runtime.h"


static volatile short nTimerTriggered= 0;
static volatile short nTimerSeen= 0;
static volatile short nADCTriggered= 0;
static volatile short nADCSeen= 0;
static int nApplicationMode= RT_MODE_POWERUP;

static ADCData sADCData;


/**
 * Initialize runtime system.
 * Clears watchdog.
 * Inits clock system.
 * Inits tick timer.
 * Inits ports.
 *
 */
void initRT ( int nPeriodInClockTicks )
{
	/* stop watchdog */
    WDTCTL = WDTPW | WDTHOLD;

    /* set 16MHz DCO values */
    BCSCTL1 = CALBC1_16MHZ;

    /* source SMCLK from DCO with divider 8, see 5.3.3 BCSCTL2, Basic Clock System Control Register 2 in slau144j.pdf */
    BCSCTL2 = DIVS_3;

    DCOCTL = CALDCO_16MHZ;

    /* wait for PLL to settle */
    __delay_cycles(8000000);


    /* setup TICK  timer */
    nTimerTriggered= 0;

    CCTL0 = CCIE;                             // CCR0 interrupt enabled
    CCR0 = nPeriodInClockTicks;
    TACTL = TASSEL_2 + MC_1;                  // source timer from SMCLK, upmode, see 12.3.1 TACTL, Timer_A Control Register in slau144j.pdf


	P1DIR |= BIT0 + BIT6;       			/* 0 LED, 6 LED */
	P2DIR |= BIT5;       					/* 2.5 DOUT on CANSPI */

	return;
}


void setRTtick ( int nPeriodInClockTicks )
{
    CCR0 = nPeriodInClockTicks;
	return;
}


void setLED1 ()
{
	  P1OUT |= BIT0;                            // reset slave
}

void clearLED1 ()
{
	  P1OUT &= ~BIT0;
}

void setLED2 ()
{
	  P1OUT |= BIT6;                            // reset slave
}

void clearLED2 ()
{
	  P1OUT &= ~BIT6;
}

void setDOUT ()
{
	  P2OUT |= BIT5;                            // reset slave
}

void clearDOUT ()
{
	  P2OUT &= ~BIT5;
}









/**
 * initialize ADC
 *
 * NOTE: only pin P1.7 is supported as ADC input (A7)
 *
 */

void initADC ( )
{
	/* */
    ADC10CTL0 = ADC10SHT_2 + ADC10ON + ADC10IE;

    /* channel 7: P1.7 */
	ADC10CTL1 = INCH_7;

	/* enable channel 7 port pin as analog input */
	ADC10AE0 |= 0x80;

	return;
}


void requestADC ()
{
	/* enable conversion and start sampling */
    ADC10CTL0 |= ENC + ADC10SC;

	return;
}



void executeRTloop ()
{
	int 	nModeRequest;

	while ( 1 )
	{
		nModeRequest= RT_NO_MODE;


		if ( nApplicationMode == RT_MODE_POWERUP )
		{
			/* allow interrupts */
			__bis_SR_register(GIE);

			nModeRequest= handleApplicationEvent ( RT_STARTUP, 0 );
			nApplicationMode= RT_MODE_RUNNING;

		}
		else if ( nApplicationMode == RT_MODE_RUNNING )
		{
			if ( nTimerTriggered != nTimerSeen )
			{
				nTimerSeen= nTimerTriggered;
				nModeRequest= handleApplicationEvent ( RT_TICK, 0 );
			}
			else if ( nADCTriggered != nADCSeen )
			{
				nADCSeen= nADCTriggered;

				nModeRequest= handleApplicationEvent ( RT_ADC, (void *) & sADCData );
			}
		}
		else
		{

		}
	}
}


// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
	nADCTriggered++;
	sADCData.nValue= ADC10MEM;
	sADCData.nTimeStamp= TA0R;
}


// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
    nTimerTriggered++;
}


