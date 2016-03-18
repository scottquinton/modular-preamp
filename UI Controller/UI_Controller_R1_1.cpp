//////////////////////////////////////////////////////////////////////////
//
// UI_Controller_R1_1.cpp
// Chip : ATxmega16E5
//
// Created: 18/12/2015 4:21:24 PM
// Authors : Andrew Swan
//			 Scott Quinton
//
// Program to control the user interface of an analog preamplifier pedal.
//
//////////////////////////////////////////////////////////////////////////

#define F_CPU 32000000UL // 32 MHz
#include <avr/io.h>
#include <util/delay.h>

// USART Functions
void sendChar(char cToSend, int chanNum);
void sendStr(char *sToSend, int chanNum);
char getByte(int chanNum);

void init(void);
void initUSART(void);

void decodeA(unsigned char chanQ);
void decodeB(unsigned char chanQ);
void get_msgS(unsigned char qChan);

unsigned long int qPos[11] = {0,0,0,0,0,0,0,0,0,0,0};
bool oldA[11] = {0,0,0,0,0,0,0,0,0,0,0};
bool oldB[11] = {0,0,0,0,0,0,0,0,0,0,0};
bool qInc = 0; // Up / Down Tracking
unsigned char qNum;
char* msgL = "Q0_0000"; // Quadrature position string
char* msgS = "Q0+"; // Quadrature change string, 0-A (11 knobs)

int main(void)
{
	init();
	initUSART();
	
	while (1)
	{
		if(((PORTA.INTFLAGS & 0xff) != 0) || ((PORTC.INTFLAGS & 0xff) != 0)  || ((PORTD.INTFLAGS & 0xff) != 0) ) {
			if((PORTA.INTFLAGS & 0b00010000) != 0) { // A0 has been triggered
				qNum = 0;
				decodeA(qNum);
				PORTA.INTFLAGS = PORTA.INTFLAGS | 0b00010000;
			}
			else if((PORTA.INTFLAGS & 0b00100000) != 0) { // B0 has been triggered
				qNum = 0;
				decodeB(qNum);
				PORTA.INTFLAGS = PORTA.INTFLAGS | 0b00100000;
			}
			else if((PORTA.INTFLAGS & 0b01000000) != 0) { // A1 has been triggered
				qNum = 1;
				decodeA(qNum);
				PORTA.INTFLAGS = PORTA.INTFLAGS | 0b01000000;
			}
			else if((PORTA.INTFLAGS & 0b10000000) != 0) { // B1 has been triggered
				qNum = 1;
				decodeB(qNum);
				PORTA.INTFLAGS = PORTA.INTFLAGS | 0b10000000;
			}
			else if((PORTD.INTFLAGS & 0b00000001) != 0) { // A2 has been triggered
				qNum = 2;
				decodeA(qNum);
				PORTD.INTFLAGS = PORTD.INTFLAGS | 0b00000001;
			}
			else if((PORTD.INTFLAGS & 0b00000010) != 0) { // B2 has been triggered
				qNum = 2;
				decodeB(qNum);
				PORTD.INTFLAGS = PORTD.INTFLAGS | 0b00000010;
			}
			else if((PORTD.INTFLAGS & 0b00010000) != 0) { // A3 has been triggered
				qNum = 3;
				decodeA(qNum);
				PORTD.INTFLAGS = PORTD.INTFLAGS | 0b00010000;
			}
			else if((PORTD.INTFLAGS & 0b00100000) != 0) { // B3 has been triggered
				qNum = 3;
				decodeB(qNum);
				PORTD.INTFLAGS = PORTD.INTFLAGS | 0b00100000;
			}
			else if((PORTD.INTFLAGS & 0b01000000) != 0) { // A4 has been triggered
				qNum = 4;
				decodeA(qNum);
				PORTD.INTFLAGS = PORTD.INTFLAGS | 0b01000000;
			}
			else if((PORTD.INTFLAGS & 0b10000000) != 0) { // B4 has been triggered
				qNum = 4;
				decodeB(qNum);
				PORTD.INTFLAGS = PORTD.INTFLAGS | 0b10000000;
			}
			else if((PORTA.INTFLAGS & 0b00001000) != 0) { // A5 has been triggered
				qNum = 5;
				decodeA(qNum);
				PORTA.INTFLAGS = PORTA.INTFLAGS | 0b00001000;
			}
			else if((PORTA.INTFLAGS & 0b00000100) != 0) { // B5 has been triggered
				qNum = 5;
				decodeB(qNum);
				PORTA.INTFLAGS = PORTA.INTFLAGS | 0b00000100;
			}
			else if((PORTA.INTFLAGS & 0b00000010) != 0) { // A6 has been triggered
				qNum = 6;
				decodeA(qNum);
				PORTA.INTFLAGS = PORTA.INTFLAGS | 0b00000010;
			}
			else if((PORTA.INTFLAGS & 0b00000001) != 0) { // B6 has been triggered
				qNum = 6;
				decodeB(qNum);
				PORTA.INTFLAGS = PORTA.INTFLAGS | 0b00000001;
			}
			else if((PORTC.INTFLAGS & 0b10000000) != 0) { // A7 has been triggered
				qNum = 7;
				decodeA(qNum);
				PORTC.INTFLAGS = PORTC.INTFLAGS | 0b10000000;
			}
			else if((PORTC.INTFLAGS & 0b01000000) != 0) { // B7 has been triggered
				qNum = 7;
				decodeB(qNum);
				PORTC.INTFLAGS = PORTC.INTFLAGS | 0b01000000;
			}
			else if((PORTC.INTFLAGS & 0b00100000) != 0) { // A8 has been triggered
				qNum = 8;
				decodeA(qNum);
				PORTC.INTFLAGS = PORTC.INTFLAGS | 0b00100000;
			}
			else if((PORTC.INTFLAGS & 0b00010000) != 0) { // B8 has been triggered
				qNum = 8;
				decodeB(qNum);
				PORTC.INTFLAGS = PORTC.INTFLAGS | 0b00010000;
			}
			else if((PORTC.INTFLAGS & 0b00001000) != 0) { // A9 has been triggered
				qNum = 9;
				decodeA(qNum);
				PORTC.INTFLAGS = PORTC.INTFLAGS | 0b00001000;
			}
			else if((PORTC.INTFLAGS & 0b00000100) != 0) { // B9 has been triggered
				qNum = 9;
				decodeB(qNum);
				PORTC.INTFLAGS = PORTC.INTFLAGS | 0b00000100;
			}
			else if((PORTC.INTFLAGS & 0b00000001) != 0) { // A10 has been triggered
				qNum = 10;
				decodeA(qNum);
				PORTC.INTFLAGS = PORTC.INTFLAGS | 0b00000001;
			}
			else if((PORTC.INTFLAGS & 0b00000010) != 0) { // B10 has been triggered
				qNum = 10;
				decodeB(qNum);
				PORTC.INTFLAGS = PORTC.INTFLAGS | 0b00000010;
			}
			get_msgS(qNum);
			sendStr(msgS, 0);
		}
	}
}

void init(void)
{
	OSC.CTRL = 0b00000010; // Enable internal 32MHz oscillator
	//OSC.CTRL = 0b00001000; // Enable external 32MHz oscillator
	//OSC.XOSCCTRL = 0b11010000; // Configure XOSC for High speed operation, high power XTAL1 and XTAL2
	
	while((OSC.STATUS & 0b00000010) == 0); // Wait for the internal oscillator to stabilize
	//while((OSC.STATUS & 0b00001000) == 0); // Wait for the external oscillator to stabilize
	
	CCP = 0xD8; // Remove code write lock
	CLK.PSCTRL = 0b00000000; // No external clock prescaler
	CCP = 0xD8; // Remove code write lock
	CLK.CTRL = 0b00000001; // Internal 32MHz Oscillator
	//CLK.CTRL = 0b00000011; // External Oscillator (32MHz)
	
	PORTA.DIR = 0x00; // All inputs
	PORTC.DIR = 0x00; // All inputs
	PORTD.DIR = 0b00001000; // All inputs except PD3 (SPI TX)
	
	PORTA.INTMASK = 0xff;
	PORTC.INTMASK = 0xff; // Pins 4 & 5 are A8 and B8
	PORTD.INTMASK = 0xf7;
	PORTC.INTCTRL = 0b00000010; // Medium Priority Interrupt
	PORTC.PIN4CTRL = 0x00; // Quadrature Input A8, Sense both edges
	PORTC.PIN5CTRL = 0x00; // Quadrature Input B8, Sense both edges
	
	SREG = 0b10000000; // Enable global interrupts
	
	_delay_ms(1000); // Wait for stuff to power up etc
	
	oldA[0] = (PORTA.IN & 0b00010000);
	oldB[0] = (PORTA.IN & 0b00100000);
	oldA[1] = (PORTA.IN & 0b01000000);
	oldB[1] = (PORTA.IN & 0b10000000);
	oldA[2] = (PORTD.IN & 0b00000001);
	oldB[2] = (PORTD.IN & 0b00000010);
	oldA[3] = (PORTD.IN & 0b00010000);
	oldB[3] = (PORTD.IN & 0b00100000);
	oldA[4] = (PORTD.IN & 0b01000000);
	oldB[4] = (PORTD.IN & 0b10000000);
	oldA[5] = (PORTA.IN & 0b00001000);
	oldB[5] = (PORTA.IN & 0b00000100);
	oldA[6] = (PORTA.IN & 0b00000010);
	oldB[6] = (PORTA.IN & 0b00000001);
	oldA[7] = (PORTC.IN & 0b10000000);
	oldB[7] = (PORTC.IN & 0b01000000);
	oldA[8] = (PORTC.IN & 0b00100000);
	oldB[8] = (PORTC.IN & 0b00010000);
	oldA[9] = (PORTC.IN & 0b00001000);
	oldB[9] = (PORTC.IN & 0b00000100);
	oldA[10] = (PORTC.IN & 0b00000001);
	oldB[10] = (PORTC.IN & 0b00000010);
}

void initUSART(void)
{
	// Configure SPI interface and speeds etc for USARTD0 @ 9600bps
	USARTD0.BAUDCTRLA = 0x0C; // BSEL = 12
	USARTD0.BAUDCTRLB = 0x40; // BSCALE = 4 (2^(4-1) = 15)
	USARTD0.CTRLA = 0x00; // Interrupts off
	USARTD0.CTRLB = 0b00011000; // CLK2X = 0, Enable transmitter and receiver
	USARTD0.CTRLC = 0b00000011; // Asynchronous, No parity, 1 stop bit, 8 data bits
	USARTD0.CTRLD = 0b00000000; // Asynchronous, No parity, 1 stop bit, 8 data bits
}

void sendChar(char cToSend, int chanNum)
{
	switch(chanNum)
	{
		case 0:
		while(!(USARTD0_STATUS & USART_DREIF_bm));
		USARTD0_DATA = cToSend;
		break;
		default:
		break;
	}
}

void sendStr(char *sToSend, int chanNum)
{
	while(*sToSend)
	sendChar(*sToSend++, chanNum);
}

char getByte(int chanNum)
{
	char temp = 0x00;
	switch(chanNum)
	{
		case 0:
		while(!(USARTD0_STATUS & USART_RXCIF_bm));
		temp = USARTD0_DATA;
		break;
		default:
		break;
	}
	return temp;
}

void decodeB(unsigned char chanQ){
	oldB[chanQ] = !oldB[chanQ];
	if (oldB[chanQ] == oldA[chanQ]){
		qPos[chanQ]++;
		qInc = 1;
	}
	else{
		qPos[chanQ]--;
		qInc = 0;
	}
}

void decodeA(unsigned char chanQ) {
	oldA[chanQ] = !oldA[chanQ];
	if (oldA[chanQ] != oldB[chanQ]){
		qPos[chanQ]++;
		qInc = 1;
	}
	else{
		qPos[chanQ]--;
		qInc = 0;
	}
}


void get_msgS(unsigned char qChan) {
	switch(qChan) {
		case 0:
		if(qInc)
		msgS = "Q0+";
		else
		msgS = "Q0-";
		break;
		case 1:
		if(qInc)
		msgS = "Q1+";
		else
		msgS = "Q1-";
		break;
		case 2:
		if(qInc)
		msgS = "Q2+";
		else
		msgS = "Q2-";
		break;
		case 3:
		if(qInc)
		msgS = "Q3+";
		else
		msgS = "Q3-";
		break;
		case 4:
		if(qInc)
		msgS = "Q4+";
		else
		msgS = "Q4-";
		break;
		case 5:
		if(qInc)
		msgS = "Q5+";
		else
		msgS = "Q5-";
		break;
		case 6:
		if(qInc)
		msgS = "Q6+";
		else
		msgS = "Q6-";
		break;
		case 7:
		if(qInc)
		msgS = "Q7+";
		else
		msgS = "Q7-";
		break;
		case 8:
		if(qInc)
		msgS = "Q8+";
		else
		msgS = "Q8-";
		break;
		case 9:
		if(qInc)
		msgS = "Q9+";
		else
		msgS = "Q9-";
		break;
		case 10:
		if(qInc)
		msgS = "QA+";
		else
		msgS = "QA-";
		break;
		default:
		break;
	}
}