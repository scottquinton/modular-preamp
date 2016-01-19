//////////////////////////////////////////////////////////////////////////
//
// Mainboard_R1_1.cpp
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

// LCD Functions
void xmitPIXEL(unsigned int xCor, unsigned int yCor, unsigned char red, unsigned char green, unsigned char blue);
void xmitDATA(unsigned char dataMSB, unsigned char dataLSB);
void xmitDATA_Byte(unsigned char dataByte);
void xmitCMD(unsigned char cmdMSB, unsigned char cmdLSB);

// USART Functions
void sendChar(char cToSend, int chanNum);
void sendStr(char *sToSend, int chanNum);
char getByte(int chanNum);
void getString(char *sToGet, int chanNum);

void init(void);
void initUSART(void);
void initLCD(void);

struct userConfig
{
	int gain; // LED distortion amount (0 - 100)
	int volume; // Master output volume (0 - 100)
	int auxVol; // Auxillary input volume (0 - 100)
	int hphVol; // Headphone output volume (0 - 100)
	int bass; // Bandaxall filter LPF setting (0 - 100)
	int treble; // Bandaxall filter HPF setting (0 - 100)
	int mids; // Bandaxall filter Middle BPF setting (0 - 100)
	bool cleanOn; // Clean or distortion channel
	int i_ValveA; // Vacuum tube B heater current (125 - 175 mA)
	int i_ValveB; // Vacuum tube B heater current (125 - 175 mA)
	int v_Valve; // Vacuum tube plate voltage (to both tubes) (100 - 200 V)
};

static int numConfig = 10; // Number of different user presets to cycle through
static int i_min = 125; // Minimum tube heater current
static int i_max = 175; // Maximum tube heater current
static int v_min = 100; // Minimum tube plate voltage supply
static int v_max = 200; // Maximum tube plate voltage supply

int main(void) 
{
	char msg_AUX;
	char msg_MA;
	char msg_MB;
	char msg_UI;
	bool MA_ON = false;
	bool MB_ON = false;

	userConfig userQueue[numConfig];
	
	userQueue[0].cleanOn = false;
	userQueue[0].bass = 75;
	userQueue[0].mids = 85;
	userQueue[0].treble = 75;
	userQueue[0].gain = 85;
	userQueue[0].i_ValveA = 140;
	userQueue[0].i_ValveB = 155; // Buffer tube should be HOT
	userQueue[0].auxVol = 0;
	userQueue[0].hphVol = 0;
	userQueue[0].volume = 50;
	userQueue[0].v_Valve = 180;
	
	init();
	initUSART();
	
	//initLCD();
	//xmitPIXEL(100, 100, 0xFF, 0xFF, 0x00); 

	while (1)
	{
		sendChar('A', 3);
		sendChar('B', 1);
		
		if(USARTD0_STATUS & USART_RXCIF_bm) // If there is unread data from AUX...
		{
			// AUX unused for now
			msg_AUX = getByte(0);		
		}
		
		if(USARTD1_STATUS & USART_RXCIF_bm) // If there is unread data from module A...
		{
			msg_MA = getByte(1);
			if(msg_MA == 'B') 
				MA_ON = true;
		}
		
		if(USARTE0_STATUS & USART_RXCIF_bm) // If there is unread data from module B...
		{
			msg_MB = getByte(2);
		}
		
		if(USARTE1_STATUS & USART_RXCIF_bm) // If there is unread data from UI CPU...
		{
			msg_UI = getByte(3);
			if(msg_UI == 'A') 
				MB_ON = true;
		}

		if(MA_ON && MB_ON)
			PORTB.OUT = 0b00010010; 
		else if(MA_ON)
			PORTB.OUT = 0b00010000; 
		else if(MB_ON)
			PORTB.OUT = 0b00000010; 
		else	
			PORTB.OUT = 0b00000000; 
			
		_delay_ms(100);
	}
}

void init(void) 
{
	//OSC.CTRL = 0b00000010; // Enable internal 32MHz oscillator
	OSC.CTRL = 0b00001000; // Enable external 32MHz oscillator
	OSC.XOSCCTRL = 0b11000000; // Configure XOSC for High speed operation, high power XTAL1 and XTAL2
	
	//while((OSC.STATUS & 0b00000010) == 0); // Wait for the internal oscillator to stabilize
    while((OSC.STATUS & 0b00001000) == 0); // Wait for the external oscillator to stabilize
	
	CCP = 0xD8; // Remove code write lock
	CLK.PSCTRL = 0b00000000; // No external clock prescaler
	CCP = 0xD8; // Remove code write lock
	//CLK.CTRL = 0b00000001; // Internal 32MHz Oscillator
	CLK.CTRL = 0b00000011; // External Oscillator (32MHz)
	
	PORTA.DIR = 0b11111000; // A0, A1, and A2 are ADC inputs, rest outputs
	ADCA.CTRLA = 0x00; // Enable the ADC on PORT A
	ADCA.CTRLB = 0x00; // Disable ADC stuff
	ADCA.REFCTRL = 0x00; // Disable the AREF pins
	
	PORTB.DIR = 0xFF; // All outputs (PB5 = D_C#, PB6 = WR#)
	ADCB.CTRLA = 0x00; // Disable the ADC on PORT B
	ADCB.CTRLB = 0x00; // Disable ADC stuff
	ADCB.REFCTRL = 0x00; // Disable the AREF pins
	PORTB.PIN0CTRL = 0b00111000; // Wired AND configuration with internal pull-up (BAT_DISC)
	PORTB.PIN1CTRL = 0b00111000; // Wired AND configuration with internal pull-up (CHA_DISC)
	PORTB.PIN4CTRL = 0b00111000; // Wired AND configuration with internal pull-up (CHB_DISC)
	PORTB.PIN5CTRL = 0b00111000; // Wired AND configuration with internal pull-up (LCD D_C#)
	PORTB.PIN6CTRL = 0b00111000; // Wired AND configuration with internal pull-up (LCD WR#)

	
	PORTC.DIR = 0x00; // All switch (x8) inputs
	PORTD.DIR = 0b10111011; // All outputs except RXD0 and RXD1
	PORTE.DIR = 0b10111011; // All outputs except RXE0 and RXE1
	PORTF.DIR = 0xFF; // All LCD interface outputs (But reversed bit by bit)
	
	//PORTC.INTMASK = 0b00110000; // Pins 4 & 5 are A8 and B8
	//PORTC.INTCTRL = 0b00000010; // Medium Priority Interrupt
	//PORTC.PIN4CTRL = 0x00; // Quadrature Input A8, Sense both edges
	//PORTC.PIN5CTRL = 0x00; // Quadrature Input B8, Sense both edges
	
	//SREG = 0b10000000; // Enable global interrupts
	
	_delay_ms(2000); // Wait for LCD to power up etc.
}

void initUSART(void)
{
	// Configure SPI interface and speeds etc for USARTD0 @ 9600bps
	USARTD0.BAUDCTRLA = 0x0C; // BSEL = 12
	USARTD0.BAUDCTRLB = 0x40; // BSCALE = 4 (2^(4-1) = 15)
	USARTD0.CTRLA = 0x00; // Interrupts off
	USARTD0.CTRLB = 0b00011000; // CLK2X = 0, Enable transmitter and receiver
	USARTD0.CTRLC = 0b00000011; // Asynchronous, No parity, 1 stop bit, 8 data bits
	
	// Configure SPI interface and speeds etc for USARTD1 @ 9600bps
	USARTD1.BAUDCTRLA = 0x0C; // BSEL = 12
	USARTD1.BAUDCTRLB = 0x40; // BSCALE = 4 (2^(4-1) = 15)
	USARTD1.CTRLA = 0x00; // Interrupts off
	USARTD1.CTRLB = 0b00011000; // CLK2X = 0, Enable transmitter and receiver
	USARTD1.CTRLC = 0b00000011; // Asynchronous, No parity, 1 stop bit, 8 data bits	
		
	// Configure SPI interface and speeds etc for USARTE0 @ 9600bps
	USARTE0.BAUDCTRLA = 0x0C; // BSEL = 12
	USARTE0.BAUDCTRLB = 0x40; // BSCALE = 4 (2^(4-1) = 15)
	USARTE0.CTRLA = 0x00; // Interrupts off
	USARTE0.CTRLB = 0b00011000; // CLK2X = 0, Enable transmitter and receiver
	USARTE0.CTRLC = 0b00000010; // Asynchronous, No parity, 1 stop bit, 7 data bits
	
	// Configure SPI interface and speeds etc for USARTE1 @ 9600bps
	USARTE1.BAUDCTRLA = 0x0C; // BSEL = 12
	USARTE1.BAUDCTRLB = 0x40; // BSCALE = 4 (2^(4-1) = 15)
	USARTE1.CTRLA = 0x00; // Interrupts off
	USARTE1.CTRLB = 0b00011000; // CLK2X = 0, Enable transmitter and receiver
	USARTE1.CTRLC = 0b00000010; // Asynchronous, No parity, 1 stop bit, 7 data bits
}

void initLCD(void)
{
	xmitCMD(0x00, 0x11); // Exit sleep mode
	_delay_ms(420); // Wait for LCD to exit sleep mode
	
	xmitCMD(0x00, 0x36); // Memory access control
	//xmitDATA(0x00, 0x80); // Bottom to top, left to right, rest default
	xmitDATA_Byte(0x80); // Bottom to top, left to right, rest default
	
	xmitCMD(0x00, 0x3A); // Interface Pixel Format
	xmitDATA_Byte(0x55); // 65K RGB color format, 16 bits per pixel
	//xmitDATA(0x00, 0x55); // 65K RGB color format, 16 bits per pixel
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
	xmitDATA(0x00, 0xEF); // 0 - 239
	
	xmitCMD(0x00, 0x2B); // Y Address Set
	xmitDATA(0x00, 0x00); //
	xmitDATA(0x01, 0x3F); // 0 - 319
	
	_delay_ms(25);
	xmitCMD(0x00, 0x29); // Turn display on
	_delay_ms(25);
}

void sendChar(char cToSend, int chanNum)
{
	switch(chanNum)
	{
		case 0:
			while(!(USARTD0_STATUS & USART_DREIF_bm));
			USARTD0_DATA = cToSend;
			break;
		case 1:
			while(!(USARTD1_STATUS & USART_DREIF_bm));
			USARTD1_DATA = cToSend;
			break;
		
		case 2:
			while(!(USARTE0_STATUS & USART_DREIF_bm));
			USARTE0_DATA = cToSend;
			break;
		case 3:
			while(!(USARTE1_STATUS & USART_DREIF_bm));
			USARTE1_DATA = cToSend;
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
		case 1:
			while(!(USARTD1_STATUS & USART_RXCIF_bm));
			temp = USARTD1_DATA;
			break;
		
		case 2:
			while(!(USARTE0_STATUS & USART_RXCIF_bm));
			temp = USARTE0_DATA;
			break;
		case 3:
			while(!(USARTE1_STATUS & USART_RXCIF_bm));
			temp = USARTE1_DATA;
			break;
		default:
			break;
	}
	return temp;
}

void getString(char *sToGet, int chanNum)
{
	
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