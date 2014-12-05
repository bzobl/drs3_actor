/*
 *   ____    _    _   _
 *  / ___|  / \  | \ | |
 * | |     / _ \ |  \| |
 * | |___ / ___ \| |\  |
 *  \____/_/   \_\_| \_|
 *
 *
 * Author: dietmar millinger
 * Revision: 0.3
 *
 * NOTES:
 * 1) this driver supports only non extended addressing
 * 2) this driver supports only 250kB and 500kB
 * 3) this driver supports no rx interrupt handling
 *
 *
 *
 *   ACLK = n/a, MCLK = SMCLK = DCO ~16MHz, BRCLK = SMCLK/2
 *
 *                    MSP430G2xx3
 *                 -----------------
 *                |             P1.2|-> Data Out(UCA0SIMO)
 *          LED <-|P1.0         P1.1|<- Data In(UCA0SOMI)
 *  Slave reset <-|P1.5         P1.4|-> Serial Clock Out(UCA0CLK)
 *  /CS         <-|P1.3         P1.7|<- INT
 *                 -----------------
 *
 */
#include "spi.h"
#include "can.h"
#include "mcp2515.h"

/* HARDWARE DEPENDENT PARTS */
static void initCANPorts()
{
	P1OUT = BIT5 + BIT3;        // P1 setup for LED & CAN board
	P1DIR |= BIT5 + BIT3;       // 0 LED, 3 CS, 5 RESET
}

static void selectCANController()
{
  P1OUT &= ~BIT3;             // clear port pin for CS
}

static void unselectCANController()
{
  P1OUT |= BIT3;              // set port pin for CS
}

static void resetCANController()
{
  P1OUT &= ~BIT5;             // Now with SPI signals initialized,
  __delay_cycles(32);
  P1OUT |= BIT5;              // reset slave
}

static int readCANByte(uint8_t * pnData)
{
	/* write out dummy byte in order to clock in a response from the controller */
	return receiveSPIData(pnData, 1);
}

/* CONTROLLER DEPENDENT PARTS */
static void writeCANRegister(short nAddress, uint8_t nData)
{
	selectCANController();
	transmitSPIByte(MCP_WRITE);
	transmitSPIByte(nAddress);
	transmitSPIByte(nData);
	waituntilSPITXBufferFree();
	__delay_cycles(160);

	unselectCANController();
}

static uint8_t readCANRegister(short nAddress, uint8_t nDefault)
{
	uint8_t	nStatus = nDefault;
	uint8_t	nReturnValue = 0;

	selectCANController();
	transmitSPIByte(MCP_READ);
	transmitSPIByte(nAddress);
	nReturnValue = receiveSPIData(& nStatus, 1);
	unselectCANController();
	if(nReturnValue != 1)
		return nDefault;
	return nStatus;
}

uint8_t nStatus = 0;
volatile uint8_t nReadValue = 0;

void setupCANController()
{
	initCANPorts();
	initSPI();

	/* issue hardware reset */
	resetCANController();

	/* wait for slave to initialize */
	__delay_cycles(75);

	/* reset command */
	selectCANController();
   	transmitSPIByte(MCP_RESET);
	unselectCANController();

	__delay_cycles(160);

	do
	{
		nStatus = readCANRegister(MCP_CANSTAT, 0x00);
		__delay_cycles(10000);
	}
	while((nStatus & MODE_MASK) != MODE_CONFIG);

	/* masks are left to default values */

	writeCANRegister(MCP_CNF1, 0x01 /*(SJW1 +(2-1))*/);                           // CNF1
	writeCANRegister(MCP_CNF2, 0x9a /*(BTLMODE +(3-1)*8 +(1-1))*/);               // CNF2
	writeCANRegister(MCP_CNF3, 0x07 /*(SOF_DISABLE + WAKFIL_DISABLE +(3-1))*/);   // CNF3

	/* enable buffer 0 without acceptance mask */
	writeCANRegister(MCP_RXB0CTRL, 0x60);

	do
	{
		// Put the MCP2515 into Normal Mode
		writeCANRegister(MCP_CANCTRL, MODE_NORMAL + CLKOUT_ENABLE + CLKOUT_PS2);

		nStatus = 0;
		selectCANController();
		transmitSPIByte(MCP_READ);
		transmitSPIByte(MCP_CANSTAT);
		readCANByte(& nStatus);
		unselectCANController();
		__delay_cycles(1000);
	}
	while((nStatus & MODE_MASK) != MODE_NORMAL);
}

/**
 * readCANErrorFlags
 */
uint8_t readCANErrorFlags()
{
	return readCANRegister(MCP_EFLG, 0x00);
}

uint8_t readCANTransmitErrorCount()
{
	return readCANRegister(MCP_TEC, 0xAA);
}

/**
 * sendCANMessage
 * transmit message in buffer 0
 */
int receiveCANMessage(uint8_t * pnData, int * pnLen, int * pnID, int * pnError)
{
	int nCANINTF = 0x00;
	int nIDL = 0;
	int nIDH = 0;
	int nDLC = 0;
	int nI = 0;

	/* read status of buffers */
	nCANINTF = readCANRegister(MCP_CANINTF, 0x00);
	if(nCANINTF & MCP_RX0IF)
	{
		/* buffer 0 is not empty */

		/* get ID */
		nIDL = readCANRegister(MCP_RXB0CTRL+RXBSIDL, 0xFF);
		nIDH = readCANRegister(MCP_RXB0CTRL+RXBSIDH, 0xFF);
		*pnID = ((nIDL & 0xE0)>>5) +((nIDH<<3));

		/* get DLC */
		nDLC = readCANRegister(MCP_RXB0CTRL+RXBDLC, 0x00);
		*pnLen = (nDLC & 0x0F);

		/* get data */
		for(nI = 0; nI < *pnLen; nI++) {
			pnData[nI] = readCANRegister(MCP_RXB0CTRL+RXBD0+nI, 0x00);
		}

		/* clear interrupt register */
		writeCANRegister(MCP_CANINTF,(uint8_t)(nCANINTF &(~ MCP_RX0IF)));

		return 1;
	}

	if(nCANINTF & MCP_RX1IF) {
		/* get ID */
		nIDL = readCANRegister(MCP_RXB1CTRL+RXBSIDL, 0xFF);
		nIDH = readCANRegister(MCP_RXB1CTRL+RXBSIDH, 0xFF);
		*pnID = ((nIDL & 0xE0)>>5) +((nIDH<<3));

		/* get DLC */
		nDLC = readCANRegister(MCP_RXB1CTRL+RXBDLC, 0x00);
		*pnLen = (nDLC & 0x0F);

		/* get data */
		for(nI = 0; nI < *pnLen; nI++) {
			pnData[nI] = readCANRegister(MCP_RXB1CTRL+RXBD0+nI, 0x00);
		}

		/* clear interrupt register */
		writeCANRegister(MCP_CANINTF,(uint8_t)(nCANINTF &(~ MCP_RX1IF)));

		return 1;
	}

	return 0;
}

/**
 * sendCANMessage
 * transmit message in buffer 0
 */
int sendCANMessage(uint8_t * pnData, int nLen, int nID)
{
	uint8_t nControlRegister = 0xFF;
	int nI = 0;

	/* read status of buffer 0 */
	nControlRegister = readCANRegister(MCP_TXB0CTRL, 0xFF);
	if(nControlRegister & MCP_TXREQ) {
		/* buffer is not empty */
		return CAN_ERR_TXBUFFERFULL;
  }

	/* mask standard identifiers */
	nID = nID & 0x007FF;

  /* setup ID */
	writeCANRegister(MCP_TXB0CTRL+TXBSIDH,(uint8_t)(nID>>3));
	writeCANRegister(MCP_TXB0CTRL+TXBSIDL,(uint8_t)(nID<<5));

  /* setup length */
	writeCANRegister(MCP_TXB0CTRL+TXBDLC, nLen);

	/* copy data into buffer */
	for(nI = 0; nI<nLen; nI++) {
    writeCANRegister(MCP_TXB0CTRL+TXBD0 + nI, pnData[nI]);
	}

	writeCANRegister(MCP_TXB0CTRL,(uint8_t) MCP_TXREQ);

	/* transmit command for buffer 0 */
	selectCANController();
	transmitSPIByte(MCP_RTS_TX0);
	unselectCANController();

	return CAN_OK;
}
