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
#include <avr/interrupt.h>

#define A0_MASK 0b00010000
#define B0_MASK 0b00100000
#define A1_MASK 0b01000000
#define B1_MASK 0b10000000
#define A2_MASK 0b00000001
#define B2_MASK 0b00000010
#define A3_MASK 0b00010000
#define B3_MASK 0b00100000
#define A4_MASK 0b01000000
#define B4_MASK 0b10000000
#define A5_MASK 0b00001000
#define B5_MASK 0b00000100
#define A6_MASK 0b00000010
#define B6_MASK 0b00000001
#define A7_MASK 0b10000000
#define B7_MASK 0b01000000
#define A8_MASK 0b00100000
#define B8_MASK 0b00010000
#define A9_MASK 0b00001000
#define B9_MASK 0b00000100
#define A10_MASK 0b00000001
#define B10_MASK 0b00000010

// USART Functions
void sendChar(volatile unsigned char cToSend);
void sendStr(void);
volatile unsigned char getByte(void);

void init(void);
void initUSART(void);

void decodeA(void);
void decodeB(void);
void get_msg(void);
void bufferMsg(void);

volatile unsigned long int qPos[11] = {0,0,0,0,0,0,0,0,0,0,0};
volatile bool oldA[11] = {0,0,0,0,0,0,0,0,0,0,0};
volatile bool oldB[11] = {0,0,0,0,0,0,0,0,0,0,0};
volatile bool qInc = 0; // Up / Down Tracking
volatile unsigned char qNum;
volatile unsigned char msg[200][4];// = {"Q0+", "Q0+", "Q0+", "Q0+", "Q0+", "Q0+", "Q0+", "Q0+", "Q0+", "Q0+"}; // Quadrature change string, 0-A (11 knobs)
volatile unsigned char mToSend = 0; // Output buffer counter

int main(void)
{
	init();
	initUSART();
	
	//unsigned int index = 100;
	
	while (1)
	{
		if(mToSend != 0) {
			sendStr();
		}
		else {
			SREG = 0b00000000; // Disable global interrupts
			oldA[0] = (PORTA.IN & A0_MASK);
			oldB[0] = (PORTA.IN & B0_MASK);
			oldA[1] = (PORTA.IN & A1_MASK);
			oldB[1] = (PORTA.IN & B1_MASK);
			oldA[2] = (PORTD.IN & A2_MASK);
			oldB[2] = (PORTD.IN & B2_MASK);
			oldA[3] = (PORTD.IN & A3_MASK);
			oldB[3] = (PORTD.IN & B3_MASK);
			oldA[4] = (PORTD.IN & A4_MASK);
			oldB[4] = (PORTD.IN & B4_MASK);
			oldA[5] = (PORTA.IN & A5_MASK);
			oldB[5] = (PORTA.IN & B5_MASK);
			oldA[6] = (PORTA.IN & A6_MASK);
			oldB[6] = (PORTA.IN & B6_MASK);
			oldA[7] = (PORTC.IN & A7_MASK);
			oldB[7] = (PORTC.IN & B7_MASK);
			oldA[8] = (PORTC.IN & A8_MASK);
			oldB[8] = (PORTC.IN & B8_MASK);
			oldA[9] = (PORTC.IN & A9_MASK);
			oldB[9] = (PORTC.IN & B9_MASK);
			oldA[10] = (PORTC.IN & A10_MASK);
			oldB[10] = (PORTC.IN & B10_MASK);		
			SREG = 0b10000000; // Enable global interrupts
		}
		_delay_ms(4);
	}
}

ISR(PORTA_INT_vect)
{
	if((PORTA.INTFLAGS & A0_MASK) != 0) { // A0 has been triggered
		qNum = 0;
		decodeA();
		//PORTA.INTFLAGS = PORTA.INTFLAGS || A0_MASK;
	}
	if((PORTA.INTFLAGS & B0_MASK) != 0) { // B0 has been triggered
		qNum = 0;
		decodeB();
		//PORTA.INTFLAGS = PORTA.INTFLAGS || B0_MASK;
	}
	if((PORTA.INTFLAGS & A1_MASK) != 0) { // A1 has been triggered
		qNum = 1;
		decodeA();
		//PORTA.INTFLAGS = PORTA.INTFLAGS || A1_MASK;
	}
	if((PORTA.INTFLAGS & B1_MASK) != 0) { // B1 has been triggered
		qNum = 1;
		decodeB();
		//PORTA.INTFLAGS = PORTA.INTFLAGS || B1_MASK;
	}
	if((PORTA.INTFLAGS & A5_MASK) != 0) { // A5 has been triggered
		qNum = 5;
		decodeA();
		//PORTA.INTFLAGS = PORTA.INTFLAGS || A5_MASK;
	}
	if((PORTA.INTFLAGS & B5_MASK) != 0) { // B5 has been triggered
		qNum = 5;
		decodeB();
		//PORTA.INTFLAGS = PORTA.INTFLAGS || B5_MASK;
	}
	if((PORTA.INTFLAGS & A6_MASK) != 0) { // A6 has been triggered
		qNum = 6;
		decodeA();
		//PORTA.INTFLAGS = PORTA.INTFLAGS || A6_MASK;
	}
	if ((PORTA.INTFLAGS & B6_MASK) != 0) { // B6 has been triggered
		qNum = 6;
		decodeB();
		//PORTA.INTFLAGS = PORTA.INTFLAGS || B6_MASK;
	}
	PORTA.INTFLAGS = 0xff;
}

ISR(PORTC_INT_vect)
{
	if((PORTC.INTFLAGS & A7_MASK) != 0) { // A7 has been triggered
		qNum = 7;
		decodeA();
	//	PORTC.INTFLAGS = PORTC.INTFLAGS || A7_MASK;
	}
	if((PORTC.INTFLAGS & B7_MASK) != 0) { // B7 has been triggered
		qNum = 7;
		decodeB();
	//	PORTC.INTFLAGS = PORTC.INTFLAGS || B7_MASK;
	}
	if((PORTC.INTFLAGS & A8_MASK) != 0) { // A8 has been triggered
		qNum = 8;
		decodeA();
	//	PORTC.INTFLAGS = PORTC.INTFLAGS || A8_MASK;		
	}
	if((PORTC.INTFLAGS & B8_MASK) != 0) { // B8 has been triggered
		qNum = 8;
		decodeB();
	//	PORTC.INTFLAGS = PORTC.INTFLAGS || B8_MASK;		
	}
	if((PORTC.INTFLAGS & A9_MASK) != 0) { // A9 has been triggered
		qNum = 9;
		decodeA();
	//	PORTC.INTFLAGS = PORTC.INTFLAGS || A9_MASK;		
	}
	if((PORTC.INTFLAGS & B9_MASK) != 0) { // B9 has been triggered
		qNum = 9;
		decodeB();
	//	PORTC.INTFLAGS = PORTC.INTFLAGS || B9_MASK;		
	}
	if((PORTC.INTFLAGS & A10_MASK) != 0) { // A10 has been triggered
		qNum = 10;
		decodeA();
	//	PORTC.INTFLAGS = PORTC.INTFLAGS || A10_MASK;		
	}
	if((PORTC.INTFLAGS & B10_MASK) != 0) { // B10 has been triggered
		qNum = 10;
		decodeB();
	//	PORTC.INTFLAGS = PORTC.INTFLAGS || B10_MASK;		
	}
	PORTC.INTFLAGS = 0xff;
}

ISR(PORTD_INT_vect)
{
	if((PORTD.INTFLAGS & A2_MASK) != 0) { // A2 has been triggered
		qNum = 2;
		decodeA();
	//	PORTD.INTFLAGS = PORTD.INTFLAGS || A2_MASK;		
	}
	if((PORTD.INTFLAGS & B2_MASK) != 0) { // B2 has been triggered
		qNum = 2;
		decodeB();
	//	PORTD.INTFLAGS = PORTD.INTFLAGS || B2_MASK;			
	}
	if((PORTD.INTFLAGS & A3_MASK) != 0) { // A3 has been triggered
		qNum = 3;
		decodeA();
	//	PORTD.INTFLAGS = PORTD.INTFLAGS || A3_MASK;			
	}
	if((PORTD.INTFLAGS & B3_MASK) != 0) { // B3 has been triggered
		qNum = 3;
		decodeB();
	//	PORTD.INTFLAGS = PORTD.INTFLAGS || B3_MASK;				
	}
	if((PORTD.INTFLAGS & A4_MASK) != 0) { // A4 has been triggered
		qNum = 4;
		decodeA();
	//	PORTD.INTFLAGS = PORTD.INTFLAGS || A4_MASK;			
	}
	if((PORTD.INTFLAGS & B4_MASK) != 0) { // B4 has been triggered
		qNum = 4;
		decodeB();
	//	PORTD.INTFLAGS = PORTD.INTFLAGS || B4_MASK;		
	}
	PORTD.INTFLAGS = 0xff;
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
	PORTD.DIR = 0b00001100; // All inputs except PD2 and PD3 (SPI TX)
	
	PORTA.INTMASK = 0xff;
	PORTC.INTMASK = 0xff; // Pins 4 & 5 are A8 and B8
	PORTD.INTMASK = 0xf3;
	
	PORTA.INTCTRL = 0b00000001; // Medium Priority Interrupt
	PORTC.INTCTRL = 0b00000001; // Medium Priority Interrupt
	PORTD.INTCTRL = 0b00000001; // Medium Priority Interrupt
	
	PORTC.PIN4CTRL = 0x00; // Quadrature Input A8, Sense both edges
	PORTC.PIN5CTRL = 0x00; // Quadrature Input B8, Sense both edges
	PMIC_CTRL = 0b00000001; // Enable all low-level interrupts
	
	PORTA.INTFLAGS = 0xff;
	PORTC.INTFLAGS = 0xff;
	PORTD.INTFLAGS = 0xff;
	
	SREG = 0b10000000; // Enable global interrupts
	
	_delay_ms(1000); // Wait for stuff to power up etc
	
	oldA[0] = (PORTA.IN & A0_MASK);
	oldB[0] = (PORTA.IN & B0_MASK);
	oldA[1] = (PORTA.IN & A1_MASK);
	oldB[1] = (PORTA.IN & B1_MASK);
	oldA[2] = (PORTD.IN & A2_MASK);
	oldB[2] = (PORTD.IN & B2_MASK);
	oldA[3] = (PORTD.IN & A3_MASK);
	oldB[3] = (PORTD.IN & B3_MASK);
	oldA[4] = (PORTD.IN & A4_MASK);
	oldB[4] = (PORTD.IN & B4_MASK);
	oldA[5] = (PORTA.IN & A5_MASK);
	oldB[5] = (PORTA.IN & B5_MASK);
	oldA[6] = (PORTA.IN & A6_MASK);
	oldB[6] = (PORTA.IN & B6_MASK);
	oldA[7] = (PORTC.IN & A7_MASK);
	oldB[7] = (PORTC.IN & B7_MASK);
	oldA[8] = (PORTC.IN & A8_MASK);
	oldB[8] = (PORTC.IN & B8_MASK);
	oldA[9] = (PORTC.IN & A9_MASK);
	oldB[9] = (PORTC.IN & B9_MASK);
	oldA[10] = (PORTC.IN & A10_MASK);
	oldB[10] = (PORTC.IN & B10_MASK);
}

void bufferMsg(void) {
	if(mToSend<=200) {
		get_msg();
		mToSend++;
	}
}

void initUSART(void)
{
	// Configure SPI interface and speeds etc for USARTD0 @ 57600bps
	USARTD0.BAUDCTRLA = 0x22; // BSEL = 34
	USARTD0.BAUDCTRLB = 0x00; // BSCALE = 0
	USARTD0.CTRLA = 0x00; // Interrupts off
	USARTD0.CTRLB = 0b00011000; // CLK2X = 0, Enable transmitter and receiver
	USARTD0.CTRLC = 0b00000011; // Asynchronous, No parity, 1 stop bit, 8 data bits
	USARTD0.CTRLD = 0b00000000; // Asynchronous, No parity, 1 stop bit, 8 data bits
}



void sendChar(volatile unsigned char cToSend)
{
	while(!(USARTD0_STATUS & USART_DREIF_bm));
	USARTD0_DATA = cToSend;
}

void sendStr(void)
{
	volatile unsigned char mIndex = mToSend-1;
	volatile unsigned char index = 0;
	while(msg[mIndex][index] != '\0') {
		sendChar(msg[mIndex][index]);
		index++;
	}
	mToSend--;
}

volatile unsigned char getByte(void)
{
	while(!(USARTD0_STATUS & USART_RXCIF_bm));
	return USARTD0_DATA;
}

void decodeB(void){
	oldB[qNum] = !oldB[qNum];
	if (oldB[qNum] == oldA[qNum]){
		qPos[qNum]++;
		qInc = 1;
	}
	else{
		qPos[qNum]--;
		qInc = 0;
	}
	bufferMsg();
}

void decodeA(void) {
	oldA[qNum] = !oldA[qNum];
	if (oldA[qNum] != oldB[qNum]){
		qPos[qNum]++;
		qInc = 1;
	}
	else{
		qPos[qNum]--;
		qInc = 0;
	}
	bufferMsg();
}


void get_msg(void) {
	msg[mToSend][0] = 'Q';
	if(qNum>=0 && qNum < 10)
		msg[mToSend][1] = qNum + (unsigned char)('0');
	else
		msg[mToSend][1] = 'A';
	if(qInc)
		msg[mToSend][2] = '+';
	else
		msg[mToSend][2] = '-';
}