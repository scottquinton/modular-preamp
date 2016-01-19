//////////////////////////////////////////////////////////////////////////
//
// UI_Controller_R1_0.cpp
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

void init(void);

int main(void)
{
	bool x = false;
	init();
	/* Replace with your application code */
	while (1)
	{
		if(PORTC.INTFLAGS != 0 && x == false) {
			if((PORTC.INTFLAGS & 0b00010000) != 0) {
				PORTC.INTFLAGS = PORTC.INTFLAGS | 0b00010000;
				// A0 has been triggered
			}
			else if((PORTC.INTFLAGS & 0b00100000) != 0) {
				PORTC.INTFLAGS = PORTC.INTFLAGS | 0b00100000;
				// B0 has been triggered
			}
			PORTD.OUT = 0b00001000;
			x = true;
		}
		else if(PORTC.INTFLAGS != 0 && x == true) {
			if((PORTC.INTFLAGS & 0b00010000) != 0) {
				PORTC.INTFLAGS = PORTC.INTFLAGS | 0b00010000;
				// A0 has been triggered
			}
			else if((PORTC.INTFLAGS & 0b00100000) != 0) {
				PORTC.INTFLAGS = PORTC.INTFLAGS | 0b00100000;
				// B0 has been triggered
			}
			PORTD.OUT = 0b00000000;
			x = false;
		}
		//PORTD.OUT = 0b00001000;
		//_delay_ms(1000);
		//PORTD.OUT = 0b00000000;
		//_delay_ms(1000);
	}
}

void init(void) {

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
	
	PORTC.INTMASK = 0b00110000; // Pins 4 & 5 are A8 and B8
	PORTC.INTCTRL = 0b00000010; // Medium Priority Interrupt
	PORTC.PIN4CTRL = 0x00; // Quadrature Input A8, Sense both edges
	PORTC.PIN5CTRL = 0x00; // Quadrature Input B8, Sense both edges
	
	SREG = 0b10000000; // Enable global interrupts
	
	// Configure SPI interface and speeds etc
	//USARTD0.BAUDCTRLA = ;
	//USARTD0.BAUDCTRLB = ;
	//USARTD0.CTRLA = ;
	//USARTD0.CTRLB = ;
	//USARTD0.CTRLC = ;
	//USARTD0.CTRLD = ;
	//USARTD0.
	// CLK2X = 0
	// BSEL = 12, BSCALE = 4, Fosc = 32 MHz, 9600 Baud
}

