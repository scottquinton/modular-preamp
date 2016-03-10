//////////////////////////////////////////////////////////////////////////
//
// Drive_Module_R1_0.cpp
// Chip : ATxmega16E5
//
// Created: 18/12/2015 4:21:24 PM
// Authors : Andrew Swan
//			 Scott Quinton
//
// Program to control the guitar drive module of an analog preamplifier pedal.
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

int main(void)
{
	bool x = false;
	char temp;
	
	init();
	initUSART();

	while (1)
	{
		/*if(PORTC.INTFLAGS != 0 && x == false) {
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
		if(USARTD0_STATUS & USART_RXCIF_bm) // If there is unread data from Main CPU...
		{
			temp = getByte(0);
			sendChar(temp, 0);
		}*/
		PORTA.OUT = 0x00;
		_delay_ms(1000);
		PORTA.OUT = 0b00001100;
		_delay_ms(1000);
	}
}

void init(void)
{
	OSC.CTRL = 0b00000010; // Enable internal 32MHz oscillator
	
	//OSC.XOSCCTRL = 0b11010000; // Configure XOSC for High speed operation, high power XTAL1 and XTAL2
	//_delay_ms(500);
	//OSC.CTRL = 0b00001000; // Enable external 32MHz oscillator
	while((OSC.STATUS & 0b00000010) == 0); // Wait for the internal oscillator to stabilize
	//OSC.CTRL = 0b00001000; // Enable external 32MHz oscillator
	//while((OSC.STATUS & 0b00001000) == 0); // Wait for the external oscillator to stabilize
	
	CCP = 0xD8; // Remove code write lock
	CLK.PSCTRL = 0b00000000; // No external clock prescaler
	CCP = 0xD8; // Remove code write lock
	CLK.CTRL = 0b00000001; // Internal 32MHz Oscillator
	//CLK.CTRL = 0b00000011; // External Oscillator (32MHz)
	
	PORTA.DIR = 0b00001100; // DAC1 and DAC0 outputs (PA2 and PA3)
	
	ADCA.CTRLA = 0x00; // Enable the ADC on PORT A
	ADCA.CTRLB = 0x00; // Disable ADC stuff
	ADCA.REFCTRL = 0x00; // Disable the AREF pins
	
	DACA.CTRLA = 0x00; // Enable the ADC on PORT A
	DACA.CTRLB = 0x00; // Disable ADC stuff
	DACA.CTRLC = 0x00; // Disable the AREF pins
	
	PORTC.DIR = 0b00000011; // i2C lines SCL and SDA (PC1 and PC0)
	PORTD.DIR = 0b00001000; // All inputs except PD3 (SPI TX)
	
	PORTA.PIN2CTRL = 0b00111000; // Wired AND configuration with internal pull-up
	PORTA.PIN3CTRL = 0b00111000; // Wired AND configuration with internal pull-up
	
	//SREG = 0b10000000; // Enable global interrupts
	
	_delay_ms(1000); // Wait for stuff to power up etc
}

void initUSART(void)
{
	// Configure SPI interface and speeds etc for USARTD0 @ 9600bps
	USARTD0.BAUDCTRLA = 0x06; // BSEL = 6
	//USARTD0.BAUDCTRLA = 0x0C; // BSEL = 12
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