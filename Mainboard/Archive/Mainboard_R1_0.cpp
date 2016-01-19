//////////////////////////////////////////////////////////////////////////
//
// Mainboard_R1_0.cpp
// Chip : ATxmega256A3U
//
// Created: 18/12/2015 4:21:24 PM
// Authors : Andrew Swan
//			 Scott Quinton
//
// Program to control the mainboard of an analog preamplifier pedal.
//
//////////////////////////////////////////////////////////////////////////

#define F_CPU 32000000UL // 32 MHz
#include <avr/io.h>
#include <util/delay.h>

void xmitPIXEL(unsigned int xCor, unsigned int yCor, unsigned char red, unsigned char green, unsigned char blue);
void xmitDATA(unsigned char dataMSB, unsigned char dataLSB);
void xmitDATA_Byte(unsigned char dataByte);
void xmitCMD(unsigned char cmdMSB, unsigned char cmdLSB);
void init(void);

int main(void) {
	
	init();
	
	xmitPIXEL(100, 100, 0xFF, 0xFF, 0x00); 
	
	while (1)
	{
		//PORTB.OUT = 0b00000001; // Flash battery disconnect +24V relay
		//PORTB.OUT = 0b00010000; // Flash channel A +24V relay
		PORTB.OUT = 0b00000010; // Flash channel B +24V relay
		_delay_ms(1000);
		PORTB.OUT = 0b00000000;
		_delay_ms(1000);
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
	
	PORTA.DIR = 0b11111000; // A0, A1, and A2 are ADC inputs, rest outputs
	PORTB.DIR = 0xFF; // All outputs (PB5 = D_C#, PB6 = WR#)
	PORTC.DIR = 0x00; // All switch (x8) inputs
	PORTD.DIR = 0b10111011; // All outputs except RXD0 and RXD1
	PORTE.DIR = 0b10111011; // All outputs except RXE0 and RXE1
	PORTF.DIR = 0xFF; // All LCD interface outputs (But reversed bit by bit)
	
	PORTB.OUT = 0xFF;
	
	//PORTC.INTMASK = 0b00110000; // Pins 4 & 5 are A8 and B8
	//PORTC.INTCTRL = 0b00000010; // Medium Priority Interrupt
	//PORTC.PIN4CTRL = 0x00; // Quadrature Input A8, Sense both edges
	//PORTC.PIN5CTRL = 0x00; // Quadrature Input B8, Sense both edges
	
	//SREG = 0b10000000; // Enable global interrupts
	
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
	
	_delay_ms(2000); // Wait for LCD to exit sleep mode
	
	xmitCMD(0x00, 0x11); // Exit sleep mode
	_delay_ms(500); // Wait for LCD to exit sleep mode
	
	xmitCMD(0x00, 0x29); // Turn display on
	
	xmitCMD(0x00, 0x36); // Memory access control
	//xmitDATA(0x00, 0x80); // Bottom to top, left to right, rest default
	xmitDATA_Byte(0x80); // 
	
	xmitCMD(0x00, 0x3A); // Interface Pixel Format
	xmitDATA_Byte(0x55); // 65K RGB color format, 16 bits per pixel
	//xmitDATA(0x00, 0x55); // Bottom to top, left to right, rest default
	//xmitDATA(0x00, 0x66); // 256K RGB color format, 18 bits per pixel
	
	xmitCMD(0x00, 0xB2); // Porch control
	xmitDATA(0x00, 0x0C); // 
	xmitDATA_Byte(0x0C); // 
	xmitDATA_Byte(0x00); // 
	xmitDATA_Byte(0x33); //
	xmitDATA_Byte(0x33); //

	xmitCMD(0x00, 0xB7); // Gate Control
	xmitDATA(0x00, 0x35); //

	xmitCMD(0x00, 0xBB); // VCOM Control
	xmitDATA(0x00, 0x2B); //

	xmitCMD(0x00, 0xC0); // LCM Control
	xmitDATA(0x00, 0x2C); //

	xmitCMD(0x00, 0xC2); // VDV and VRH Command Enable
	xmitDATA(0x00, 0x01); //
	xmitDATA_Byte(0xFF); //
	
	xmitCMD(0x00, 0xC3); // VRH Set
	xmitDATA(0x00, 0x11); //
		
	xmitCMD(0x00, 0xC4); // VDV Control
	xmitDATA(0x00, 0x20); //
			
	xmitCMD(0x00, 0xC6); // Frame rate control in normal mode
	xmitDATA(0x00, 0x0F); //

	xmitCMD(0x00, 0xD0); // Power Control 1
	xmitDATA(0x00, 0xA4); //
	xmitDATA_Byte(0xA1); //
	
	xmitCMD(0x00, 0xE0); // Positive Voltage Gamma Control
	xmitDATA(0x00, 0xD0); //
	xmitDATA(0x00, 0x00); //
	xmitDATA(0x00, 0x05); //
	xmitDATA(0x00, 0x0E); //
	xmitDATA(0x00, 0x15); //
	xmitDATA(0x00, 0x0D); //
	xmitDATA(0x00, 0x37); //
	xmitDATA(0x00, 0x43); //
	xmitDATA(0x00, 0x47); //
	xmitDATA(0x00, 0x09); //
	xmitDATA(0x00, 0x15); //
	xmitDATA(0x00, 0x12); //
	xmitDATA(0x00, 0x16); //
	xmitDATA(0x00, 0x19); // 

	xmitCMD(0x00, 0xE1); // Negative Voltage Gamma Control
	xmitDATA(0x00, 0xD0); //
	xmitDATA(0x00, 0x00); //
	xmitDATA(0x00, 0x05); //
	xmitDATA(0x00, 0x0D); //
	xmitDATA(0x00, 0x0C); //
	xmitDATA(0x00, 0x06); //
	xmitDATA(0x00, 0x2D); //
	xmitDATA(0x00, 0x44); //
	xmitDATA(0x00, 0x40); //
	xmitDATA(0x00, 0x0E); //
	xmitDATA(0x00, 0x1C); //
	xmitDATA(0x00, 0x18); //
	xmitDATA(0x00, 0x16); //
	xmitDATA(0x00, 0x19); //	

	xmitCMD(0x00, 0x2A); // X Address Set
	xmitDATA(0x00, 0x00); //
	xmitDATA(0x00, 0xEF); //
	
	xmitCMD(0x00, 0x2B); // Y Address Set
	xmitDATA(0x00, 0x00); //
	xmitDATA(0x01, 0x3F); //

	_delay_ms(10);
	

}

void xmitPIXEL(unsigned int xCor, unsigned int yCor, unsigned char red, unsigned char green, unsigned char blue)
{
		
	xmitCMD(0x00, 0x2C); // Memory Write
	xmitDATA(0x00, 0x00); // 
		
	xmitCMD(0x00, 0x2C); // Memory Write
	xmitDATA(0x00, 0x00); //
		
	xmitCMD(0x00, 0x2C); // Memory Write
	xmitDATA(0x00, 0x00); //
	
	xmitCMD(0x00, 0x2C); // Memory Write
	xmitDATA(0x00, 0x00); //	
	
	/*xmitCMD(0x00, 0xC8); // Register Value 1
	xmitDATA(0x00, 0x55); // 
	xmitCMD(0x00, 0xCA); // Register Value 2
	xmitDATA(0x00, 0x55); //	
	xmitCMD(0x00, 0xB0); // RAM Control
	xmitDATA(0x00, 0x00); //
	xmitDATA(0x00, 0x00); //*/
}

void xmitDATA_Byte(unsigned char dataByte)
{
	PORTB.OUT = 0b00100000; // Low active write
	_delay_ms(1);
	PORTF.OUT = dataByte;
	_delay_ms(1);
	PORTB.OUT = 0b01100000; // Write the data
	_delay_ms(1);
}

void xmitDATA(unsigned char dataMSB, unsigned char dataLSB)
{
	PORTB.OUT = 0b00100000; // Low active write
	_delay_ms(1);
	PORTF.OUT = dataMSB;
	_delay_ms(1);
	PORTB.OUT = 0b01100000; // Write the data
	_delay_ms(1);
	PORTB.OUT = 0b00100000; // Low active write
	_delay_ms(1);
	PORTF.OUT = dataLSB;
	_delay_ms(1);
	PORTB.OUT = 0b01100000; // Write the data
	_delay_ms(1);
}

void xmitCMD(unsigned char cmdMSB, unsigned char cmdLSB)
{
	PORTB.OUT = 0b00000000; // Low active write
	_delay_ms(1);
	PORTF.OUT = cmdMSB;
	_delay_ms(10);
	PORTB.OUT = 0b01000000; // Write the command MSB
	_delay_ms(10);	
	PORTB.OUT = 0b00000000; // Low active write
	_delay_ms(1);
	PORTF.OUT = cmdLSB;
	_delay_ms(10);
	PORTB.OUT = 0b01000000; // Write the command LSB
	_delay_ms(10);
}