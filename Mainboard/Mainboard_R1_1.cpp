//////////////////////////////////////////////////////////////////////////
//
// Mainboard_R1_1.cpp
// Chip : ATxmega256A3U
//
// Created: 08/03/2016 4:21:24 PM
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
void xmitDATA(unsigned char dataByte);
void xmitCMD(unsigned char cmdByte);
void lcdDelay(unsigned char lcdDel);

// USART Functions
void sendChar(char cToSend, int chanNum);
void sendStr(char *sToSend, int chanNum);
char getByte(int chanNum);
void getString(char *sToGet, int chanNum);

void init(void);
void initUSART(void);
void initLCD(void);
void xmitPlaid(void);

struct userConfig
{
	char *name; // Name of the channel
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

int userChan = 0; // Currently selected preset number
int numConfig = 0; // Number of different user presets to cycle through
static int maxConfig = 10; // Maximum number of different user presets to cycle through
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

	userConfig userQueue[maxConfig];
	
	userQueue[numConfig].name = "HEAVY DIST 1"; // This is a heavily distorted preset
	userQueue[numConfig].cleanOn = false;
	userQueue[numConfig].bass = 75;
	userQueue[numConfig].mids = 85;
	userQueue[numConfig].treble = 75;
	userQueue[numConfig].gain = 85;
	userQueue[numConfig].i_ValveA = 140;
	userQueue[numConfig].i_ValveB = 155; // Buffer tube should be HOT
	userQueue[numConfig].auxVol = 0;
	userQueue[numConfig].hphVol = 0;
	userQueue[numConfig].volume = 50;
	userQueue[numConfig].v_Valve = 180;
	numConfig++;	
	
	userQueue[numConfig].name = "CLEAN GROOVE 1"; // This is a bass heavy clean preset
	userQueue[numConfig].cleanOn = true;
	userQueue[numConfig].bass = 85;
	userQueue[numConfig].mids = 75;
	userQueue[numConfig].treble = 75;
	userQueue[numConfig].gain = 25;
	userQueue[numConfig].i_ValveA = 140;
	userQueue[numConfig].i_ValveB = 155; // Buffer tube should be HOT
	userQueue[numConfig].auxVol = 0;
	userQueue[numConfig].hphVol = 0;
	userQueue[numConfig].volume = 50;
	userQueue[numConfig].v_Valve = 180;
	numConfig++;
	
	init();
	initUSART();
	initLCD();
	
	PORTB.OUTSET = 0b00010000; // Turn on Both Relays
	PORTB.OUTSET = 0b00000010; // Turn on Both Relays
	
	xmitCMD(0x2C); // Start writing pixels
	
	while (1)
	{
		xmitPlaid();
	}
}

void init(void) 
{
	OSC.CTRL = 0b00000010; // Enable internal 32MHz oscillator
	//OSC.CTRL = 0b00001000; // Enable external 32MHz oscillator
	//OSC.XOSCCTRL = 0b11000000; // Configure XOSC for High speed operation, high power XTAL1 and XTAL2
	
	while((OSC.STATUS & 0b00000010) == 0); // Wait for the internal oscillator to stabilize
    //while((OSC.STATUS & 0b00001000) == 0); // Wait for the external oscillator to stabilize
	
	CCP = 0xD8; // Remove code write lock
	CLK.PSCTRL = 0b00000000; // No external clock prescaler
	CCP = 0xD8; // Remove code write lock
	CLK.CTRL = 0b00000001; // Internal 32MHz Oscillator
	//CLK.CTRL = 0b00000011; // External Oscillator (32MHz)
	
	PORTA.DIR = 0b11111000; // A0, A1, and A2 are ADC inputs, rest outputs
	ADCA.CTRLA = 0x00; // Enable the ADC on PORT A
	ADCA.CTRLB = 0x00; // Disable ADC stuff
	ADCA.REFCTRL = 0x00; // Disable the AREF pins
	PORTA.OUT = 0x00;
	
	PORTB.DIR = 0xFF; // All outputs (PB5 = D_C#, PB6 = WR#)
	ADCB.CTRLA = 0x00; // Disable the ADC on PORT B
	ADCB.CTRLB = 0x00; // Disable ADC stuff
	ADCB.REFCTRL = 0x00; // Disable the AREF pins
	PORTB.PIN0CTRL = 0b00000000; // Totem Pole Configuration (BAT_DISC)
	PORTB.PIN1CTRL = 0b00000000; // Totem Pole Configuration (CHA_DISC)
	PORTB.PIN4CTRL = 0b00000000; // Totem Pole Configuration (CHB_DISC)
	PORTB.PIN5CTRL = 0b00000000; // Totem Pole Configuration (LCD D_C#)
	PORTB.PIN6CTRL = 0b00000000; // Totem Pole Configuration (LCD WR#)
	PORTB.OUT = 0x00;

	
	PORTC.DIR = 0x00; // All switch (x8) inputs
	PORTD.DIR = 0b10111011; // All outputs except RXD0 and RXD1
	PORTE.DIR = 0b10111011; // All outputs except RXE0 and RXE1
	PORTF.DIR = 0xFF; // All LCD interface outputs (But reversed bit by bit)
	PORTF.OUT = 0x00;
	
	//SREG = 0b10000000; // Enable global interrupts

	_delay_ms(1000); // Wait for LCD to power up etc.

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
	xmitCMD(0x28); // Turn display off
	
	xmitCMD(0x11); // Exit sleep mode	
	
	xmitCMD(0x36); // Memory access control
	//xmitDATA(0x00, 0x80); // Bottom to top, left to right, rest default
	xmitDATA(0x80); // Bottom to top, left to right, rest default
	
	xmitCMD(0x3A); // Interface Pixel Format
	xmitDATA(0x55); // 65K RGB color format, 16 bits per pixel
	//xmitDATA(0x00, 0x55); // 65K RGB color format, 16 bits per pixel
	//xmitDATA(0x00, 0x66); // 256K RGB color format, 18 bits per pixel
	
	xmitCMD(0xB2); // Porch control
	xmitDATA(0x0C); //
	xmitDATA(0x0C); //
	xmitDATA(0x00); //
	xmitDATA(0x33); //
	xmitDATA(0x33); //

	xmitCMD(0xB7); // Gate Control
	xmitDATA(0x35); //

	xmitCMD(0xBB); // VCOM Control
	xmitDATA(0x2B); //

	xmitCMD(0xC0); // LCM Control
	xmitDATA(0x2C); //

	xmitCMD(0xC2); // VDV and VRH Command Enable
	xmitDATA(0x01); //
	xmitDATA(0xFF); //
	
	xmitCMD(0xC3); // VRH Set
	xmitDATA(0x11); //
	
	xmitCMD(0xC4); // VDV Control
	xmitDATA(0x20); //
	
	xmitCMD(0xC6); // Frame rate control in normal mode
	xmitDATA(0x0F); //

	xmitCMD(0xD0); // Power Control 1
	xmitDATA(0xA4); //
	xmitDATA(0xA1); //
	
	xmitCMD(0xE0); // Positive Voltage Gamma Control
	xmitDATA(0xD0); //
	xmitDATA(0x00); //
	xmitDATA(0x05); //
	xmitDATA(0x0E); //
	xmitDATA(0x15); //
	xmitDATA(0x0D); //
	xmitDATA(0x37); //
	xmitDATA(0x43); //
	xmitDATA(0x47); //
	xmitDATA(0x09); //
	xmitDATA(0x15); //
	xmitDATA(0x12); //
	xmitDATA(0x16); //
	xmitDATA(0x19); //

	xmitCMD(0xE1); // Negative Voltage Gamma Control
	xmitDATA(0xD0); //
	xmitDATA(0x00); //
	xmitDATA(0x05); //
	xmitDATA(0x0D); //
	xmitDATA(0x0C); //
	xmitDATA(0x06); //
	xmitDATA(0x2D); //
	xmitDATA(0x44); //
	xmitDATA(0x40); //
	xmitDATA(0x0E); //
	xmitDATA(0x1C); //
	xmitDATA(0x18); //
	xmitDATA(0x16); //
	xmitDATA(0x19); //

	xmitCMD(0x2A); // X Address Set
	xmitDATA(0x00); //
	xmitDATA(0x00); // Start 0
	xmitDATA(0x00); //
	xmitDATA(0xEF); // Finish 239
	
	xmitCMD(0x2B); // Y Address Set
	xmitDATA(0x00); //
	xmitDATA(0x00); // Start 0
	xmitDATA(0x01); //
	xmitDATA(0x3F); // Finish 319

	xmitCMD(0x29); // Turn display on
	_delay_ms(50);
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
	xmitCMD(0x2C); // Memory Write
	//xmitDATA(0x00); //
	//xmitDATA(0x1F); // Blue
}

void xmitDATA(unsigned char dataByte)
{
	PORTB.OUTSET = 0b00100000; // D/C# high for data
	PORTF.OUT = dataByte;
	//lcdDelay(1);
	PORTB.OUTCLR = 0b01000000; // WR goes low
	//lcdDelay(1);
	PORTB.OUTSET = 0b01000000; // WR goes high
	//lcdDelay(1);
}

void xmitCMD(unsigned char cmdByte)
{
	PORTB.OUTCLR = 0b00100000; // D/C# low for command
	PORTF.OUT = cmdByte;
	lcdDelay(10);
	PORTB.OUTCLR = 0b01000000; // WR goes low
	//lcdDelay(1);
	PORTB.OUTSET = 0b01000000; // WR goes high
	//lcdDelay(1);
}

void xmitPlaid(void) 
{
	for(int i=0; i<20; i++) 
		for(int j=0; j<240; j++) {
			xmitDATA(4*i); //
			xmitDATA(j); // Blue?
		}
}
 
void lcdDelay(unsigned char lcdDel)
{
	for(unsigned char i = 0; i < lcdDel; i++)
		asm("NOP");
}
