//////////////////////////////////////////////////////////////////////////
//
// Guitar_Module_R1_0.cpp
// Chip : ATxmega16E5
//
// Created: 18/12/2015 4:21:24 PM
// Authors : Andrew Swan
//			 Scott Quinton
//
// Program to control the guitar module of an analog preamplifier pedal.
// Communicates with preamp modules using SPI lanes.
//
//////////////////////////////////////////////////////////////////////////

#define F_CPU 32000000UL // 32 MHz
#include <avr/io.h>
#include <util/delay.h>

void init(void);

int main(void)
{
	init();
	/* Replace with your application code */
	while (1)
	{
	}
}

void init(void) {
	//while(OSC.STATUS == 1); // Wait for the oscillator to stabilize
	CLK.PSCTRL = 0x00; // No external clock prescaler
	OSC.XOSCCTRL = 0x00; // Configure External Oscillator
	OSC.CTRL = 0x00; // Turn on external oscillator
	_delay_ms(1000);
	//while(OSC.STATUS == 1); // Wait for the oscillator to stabilize
	
	PORTA.DIR = 0xff; // All inputs
	PORTC.DIR = 0xff; // All inputs
	PORTD.DIR = 0b11110111; // All inputs except PD3 (SPI TX)
	// Configure SPI interface and speeds etc
	
	// Configure event system and set up port change interrupts
}

