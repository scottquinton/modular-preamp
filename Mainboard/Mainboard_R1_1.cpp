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
#include <font.h>

#define RED 0b1111100000000000
#define GREEN 0b0000011111100000
#define BLUE 0b0000000000011111
#define WHITE 0xFFFF
#define BLACK 0x0000

#define CHAR_WIDTH 0x0010 // 16
#define CHAR_HEIGHT 0x0010

// LCD Functions
;void xmitPIXEL(unsigned int xCor, unsigned int yCor, unsigned char red, unsigned char green, unsigned char blue);
void xmitDATA(unsigned char dataByte);
void xmitCMD(unsigned char cmdByte);
void lcdDelay(unsigned char lcdDel);
void xmitHLine(short int xPos, short int yPos, short int length, short int color);
void xmitVLine(short int xPos, short int yPos, short int length, short int color);
void xmitText(short int xStart, short int yStart, short int textChar, short int color);
void drawString(const char* str, short int xStart, short int yStart, short int text_color, short int bg_color);
void drawChar(unsigned char c, short int xStart, short int yStart, short int text_color, short int bg_color);
void drawBox(short int topleft_x, short int topleft_y, short int botright_x, short int botright_y, short int color);
void fillBox(short int topleft_x, short int topleft_y, short int botright_x, short int botright_y, short int color);
void drawLevel(short int x_left, short int x_right, short int y_top, short int y_bot, short int level);
void drawDisplay(unsigned char preset_no);
int min(short int a, short int b);
int max(short int a, short int b);

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
	
	short int qPos[11] = {0,0,0,0,0,0,0,0,0,0,0};
	short int pos = 0;
	short int enc = 0;

	
	short int BKCOL = BLACK;
	fillBox(0, 0, 320, 240, BKCOL);
	drawDisplay('1');
	
	while (1)
	{
		//sendStr("A      B000000C", 2);
		//sendChar(0xff, 2);
		//_delay_ms(250);
		if(USARTE1_STATUS & USART_RXCIF_bm) // If there is unread data from UI controller
		{
			char c = getByte(3);
			char s[15];
			if (c >= '0' && c<='9'){
				enc = (int)(c-'0');
			}
			if(c == '+'){
				qPos[enc] = min(qPos[enc]+1, 63);
			}
			else if(c == '-'){
				qPos[enc] = max(qPos[enc]-1, 0);
			}
			s[0] = 'A';
			for(int i=0; i<5; i++)
				s[i+1] = qPos[i];
			s[6] = 'B';
			for(int i=5; i<10; i++)
				s[i+2] = qPos[i];
			s[12] = 'C';
			for(int i=0; i<13; i++)
				sendChar(s[i], 2);
			//sendChar((unsigned char)qPos[4], 2);
			//short int val = qPos[0];
			//if (val > 255) encoder_val = 255;
			//else if (val < 0) encoder_val = 0;
			//else encoder_val = val;
			int y_top = 0;
			int y_bot = 0;
			int x_left= 0;
			int x_right = 0;
			
			if(enc<5){
				y_top = 51;
				y_bot = 114;
			}
			else{
				y_top = 160;
				y_bot = 223;
			}
			
			/*
			// Top channel levels
			drawBox(10, 50, 50, 115, black);	// Vol level
			drawBox(75, 50, 115, 115, black);	// Gain level
			drawBox(140, 50, 180, 115, black);	// Low level
			drawBox(205, 50, 245, 115, black);	// Mid level
			drawBox(270, 50, 310, 115, black);	// High level
			
			// Bottom channel levels
			drawBox(10, 159, 50, 224, black);	// Vol level
			drawBox(75, 159, 115, 224, black);	// Gain level
			drawBox(140, 159, 180, 224, black);	// Low level
			drawBox(205, 159, 245, 224, black);	// Mid level
			drawBox(270, 159, 310, 224, black);	// High level
			*/
			
			switch(enc) {
				case 0:
				case 5:
					x_left = 271;
					x_right = 309;
					break;
				case 1:
				case 6:
					x_left = 206;
					x_right = 244;
					break;
				case 2:
				case 7:
					x_left = 141;
					x_right = 179;
					break;
				case 3:
				case 8:
					x_left = 76;
					x_right = 114;
					break;
				case 4:
				case 9:
					x_left = 11;
					x_right = 49;
					break;
				default:
					break;
			}
				
			drawLevel(x_left, x_right, y_top, y_bot, qPos[enc]);
			//drawChar(pos, 50, 50, char_color, bg_color);
		}	
	
	}
}

void init(void) 
{
	OSC.CTRL = 0b00000010; // Enable internal 32MHz oscillator
	while((OSC.STATUS & 0b00000010) == 0); // Wait for the internal oscillator to stabilize	
	CCP = 0xD8; // Remove code write lock
	CLK.PSCTRL = 0b00000000; // No external clock prescaler
	CCP = 0xD8; // Remove code write lock
	CLK.CTRL = 0b00000001; // Internal 32MHz Oscillator
	
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
	// Configure SPI interface and speeds etc for AUXILLARY USARTD0 @ 9600bps
	//USARTD0.BAUDCTRLA = 0x0C; // BSEL = 12
	//USARTD0.BAUDCTRLB = 0x40; // BSCALE = 4 (2^(4-1) = 15)
	//USARTD0.CTRLA = 0x00; // Interrupts off
	//USARTD0.CTRLB = 0b00011000; // CLK2X = 0, Enable transmitter and receiver
	//USARTD0.CTRLC = 0b00000011; // Asynchronous, No parity, 1 stop bit, 8 data bits
	
	// Configure SPI interface and speeds etc for Module A USARTD1 @ 57600bps
	USARTD1.BAUDCTRLA = 0x22; // BSEL = 34
	USARTD1.BAUDCTRLB = 0x00; // BSCALE = 0
	USARTD1.CTRLA = 0x00; // Interrupts off
	USARTD1.CTRLB = 0b00011000; // CLK2X = 0, Enable transmitter and receiver
	USARTD1.CTRLC = 0b00000011; // Asynchronous, No parity, 1 stop bit, 8 data bits	
		
	// Configure SPI interface and speeds etc for Module B USARTE0 @ 57600bps
	USARTE0.BAUDCTRLA = 0x22; // BSEL = 34
	USARTE0.BAUDCTRLB = 0x00; // BSCALE = 0
	USARTE0.CTRLA = 0x00; // Interrupts off
	USARTE0.CTRLB = 0b00011000; // CLK2X = 0, Enable transmitter and receiver
	USARTE0.CTRLC = 0b00000011; // Asynchronous, No parity, 1 stop bit, 8 data bits
	
	// Configure SPI interface and speeds etc for UI Controller USARTE1 @ 57600bps
	USARTE1.BAUDCTRLA = 0x22; // BSEL = 34
	USARTE1.BAUDCTRLB = 0x00; // BSCALE = 0
	USARTE1.CTRLA = 0x00; // Interrupts off
	USARTE1.CTRLB = 0b00011000; // CLK2X = 0, Enable transmitter and receiver
	USARTE1.CTRLC = 0b00000011; // Asynchronous, No parity, 1 stop bit, 8 data bits
}

void initLCD(void)
{
	xmitCMD(0x28); // Turn display off
	xmitCMD(0x11); // Exit sleep mode	
	
	xmitCMD(0x36); // Memory access control
	xmitDATA(0x80); // Bottom to top, left to right, rest default
	xmitCMD(0x3A); // Interface Pixel Format
	xmitDATA(0x55); // 65K RGB color format, 16 bits per pixel
	
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
	int len;
	bool done = false;
	while(!done) {
		*sToGet = getByte(chanNum);
		if(*sToGet == '\0')
			done = true;
		else {
			sToGet++;
			len++;
		}
		if(len > 20) {
			done = true;
		}
	}
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

void xmitHLine(short int xPos, short int yPos, short int length, short int color)
{
	unsigned char colorH = (unsigned char)(color >> 8);
	unsigned char colorL = (unsigned char)(color & 0x00FF);
	
	unsigned char xStartH = (unsigned char)(xPos >> 8);
	unsigned char xStartL = (unsigned char)(xPos & 0x00FF);
	unsigned char xEndH = (unsigned char)((xPos + length) >> 8);
	unsigned char xEndL = (unsigned char)((xPos + length) & 0x00FF);
	unsigned char yStart = (unsigned char)yPos;
	unsigned char yEnd = yStart;
		
	xmitCMD(0x36); // Memory access control
	xmitDATA(0x80); // Bottom to top, left to right, rest default
		
	xmitCMD(0x2A); // X Address Set
	xmitDATA(0x00); //
	xmitDATA(yStart); // Start 0
	xmitDATA(0x00); //
	xmitDATA(yEnd); // Finish 239
		
	xmitCMD(0x2B); // Y Address Set
	xmitDATA(xStartH); //
	xmitDATA(xStartL); // Start 0
	xmitDATA(xEndH); //
	xmitDATA(xEndL); // Finish 319
		
	xmitCMD(0x2C); // Start writing pixels
	for(int i=0; i<=length; i++) {
		xmitDATA(colorH);
		xmitDATA(colorL);
			
	}
}

// 60, 160, 120, RED
void xmitVLine(short int xPos, short int yPos, short int length, short int color)
{
	unsigned char colorH = (unsigned char)(color >> 8);
	unsigned char colorL = (unsigned char)(color & 0x00FF);
	unsigned char xStartH = (unsigned char)(xPos >> 8);
	unsigned char xStartL = (unsigned char)(xPos & 0x00FF);
	unsigned char xEndH = xStartH;
	unsigned char xEndL = xStartL;
	unsigned char yStart = (unsigned char)yPos;
	unsigned char yEnd = (unsigned char)yPos + (unsigned char)length;
	
	xmitCMD(0x36); // Memory access control
	xmitDATA(0xA0); // Bottom to top, left to right, rest default
	
	xmitCMD(0x2A); // X Address Set
	xmitDATA(xStartH); //
	xmitDATA(xStartL); // Start 0
	xmitDATA(xEndH); //
	xmitDATA(xEndL); // Finish 319
	
	xmitCMD(0x2B); // /y Address Set
	xmitDATA(0x00); //
	xmitDATA(yStart); // Start 0
	xmitDATA(0x00); //
	xmitDATA(yEnd); // Finish 239

	xmitCMD(0x2C); // Start writing pixels	
	for(int i=0; i<length; i++) {
		xmitDATA(colorH);
		xmitDATA(colorL);
	}
}

int getCharIndex(unsigned char c) {
	int c_val = (int)(c);
	if (c >= 'A' && c <= 'Z') c_val -= ('A' - 10);
	else if (c >= '0' && c <= '9') c_val -= '0';
	else if (c == ' ') c_val = 40;
	return c_val;
}

void drawChar(unsigned char c, short int xStart, short int yStart, short int text_color, short int bg_color)
{
	unsigned char tcolorH = (unsigned char)(text_color >> 8);
	unsigned char tcolorL = (unsigned char)(text_color & 0x00FF);
	unsigned char bgcolorH = (unsigned char)(bg_color >> 8);
	unsigned char bgcolorL = (unsigned char)(bg_color & 0x00FF);
	unsigned char xStartH = (unsigned char)(xStart >> 8);
	unsigned char xStartL = (unsigned char)(xStart & 0x00FF);
	unsigned char xEndH = (unsigned char)((xStart + 15) >> 8);
	unsigned char xEndL = (unsigned char)((xStart + 15) & 0x00FF);
	unsigned char yStartL = (unsigned char)yStart;
	unsigned char yEnd = (unsigned char)(yStart + 15);	
	
	xmitCMD(0x36); // Memory access control
	xmitDATA(0xA0); // Bottom to top, left to right, rest default
	//xmitCMD(0x36); // Memory access control
	//xmitDATA(0xA0); // Bottom to top, left to right, rest default
	
	xmitCMD(0x2A); // X Address Set
	xmitDATA(xStartH); //
	xmitDATA(xStartL); // Start 0
	xmitDATA(xEndH); //
	xmitDATA(xEndL); // Finish 319
	
	xmitCMD(0x2B); // /y Address Set
	xmitDATA(0x00); //
	xmitDATA(yStartL); // Start 0
	xmitDATA(0x00); //
	xmitDATA(yEnd); // Finish 239
	
	int c_index = getCharIndex(c);
	short int chr[16];
	for(int i=0; i<16; i++)
		chr[i] = font[c_index][i];
	
	xmitCMD(0x2C); // Start writing pixels
	for(int i=0; i<16; i++) {
		for(int j=0; j<16; j++) {
			
			if(chr[i] & (1<<(15-j))){
				xmitDATA(tcolorH);
				xmitDATA(tcolorL);
			}
			else{
				xmitDATA(bgcolorH);
				xmitDATA(bgcolorL);
			}
			
		}
	}	
}

void drawDisplay(unsigned char preset_no) {
	short int white = BLACK;
	short int black = WHITE;
	
	// Current preset header
	drawString("PRESET 1", 0, 0, black, white);
	
	// Top channel labels
	drawString("VOL", 7, 30, black, white);
	drawString("GAIN", 63, 30, black, white);
	drawString("LOW", 136, 30, black, white);
	drawString("MID", 201, 30, black, white);
	drawString("HIGH", 256, 30, black, white);
	
	// Bottom channel labels
	drawString("VOL", 7, 139, black, white);
	drawString("GAIN", 63, 139, black, white);
	drawString("LOW", 136, 139, black, white);
	drawString("MID", 201, 139, black, white);
	drawString("HIGH", 256, 139, black, white);
	
	// Top channel levels
	drawBox(10, 50, 50, 115, black);	// Vol level
	drawBox(75, 50, 115, 115, black);	// Gain level
	drawBox(140, 50, 180, 115, black);	// Low level
	drawBox(205, 50, 245, 115, black);	// Mid level
	drawBox(270, 50, 310, 115, black);	// High level
	
	// Bottom channel levels
	drawBox(10, 159, 50, 224, black);	// Vol level
	drawBox(75, 159, 115, 224, black);	// Gain level
	drawBox(140, 159, 180, 224, black);	// Low level
	drawBox(205, 159, 245, 224, black);	// Mid level
	drawBox(270, 159, 310, 224, black);	// High level
	
	// Draw all lines last
	xmitHLine(0, 16, 319, black);	// Header separator
	xmitHLine(0, 125, 319, black);	// Top/Bottom Channel separator
	
	xmitVLine(63, 16, 223, black);	// Vol/Gain separator
	xmitVLine(127, 16, 223, black);	// Gain/Low separator
	xmitVLine(193, 16, 223, black);	// Low/Mid separator
	xmitVLine(257, 16, 223, black);	// Mid/High separator
	
}

void drawBox(short int topleft_x, short int topleft_y, short int botright_x, short int botright_y, short int color){
	short int height = botright_y - topleft_y;
	short int width = botright_x - topleft_x;
	xmitHLine(topleft_x, topleft_y, width, color);	// top
	xmitHLine(topleft_x, botright_y, width, color);	// bottom
	xmitVLine(topleft_x, topleft_y, height, color);	// left
	xmitVLine(botright_x, topleft_y, height, color);	// right
}

void fillBox(short int topleft_x, short int topleft_y, short int botright_x, short int botright_y, short int color){
	short int height = botright_y - topleft_y;
	short int width = botright_x - topleft_x;
	unsigned char colorH = (unsigned char)(color >> 8);
	unsigned char colorL = (unsigned char)(color & 0x00FF);
	unsigned char xStartH = (unsigned char)(topleft_x >> 8);
	unsigned char xStartL = (unsigned char)(topleft_x & 0x00FF);
	unsigned char xEndH = (unsigned char)(botright_x >> 8);
	unsigned char xEndL = (unsigned char)(botright_x & 0x00FF);
	unsigned char yStart = (unsigned char)topleft_y;
	unsigned char yEnd = (unsigned char)botright_y;
	
	xmitCMD(0x36); // Memory access control
	xmitDATA(0xA0); // Bottom to top, left to right, rest default
	
		xmitCMD(0x2A); // X Address Set
		xmitDATA(xStartH); //
		xmitDATA(xStartL); // Start left x
		xmitDATA(xEndH); //
		xmitDATA(xEndL); // Finish right x
		
		xmitCMD(0x2B); // /y Address Set
		xmitDATA(0x00); //
		xmitDATA(yStart); // Start top y
		xmitDATA(0x00); //
		xmitDATA(yEnd); // Finish bottom y
	
	xmitCMD(0x2C); // Start writing pixels
	for(int i=0; i<=width; i++)
	for(int j=0; j<=height; j++) {
		xmitDATA(colorH);
		xmitDATA(colorL);
	}
}

void drawLevel(short int x_left, short int x_right, short int y_top, short int y_bot, short int val){
	short int blue = BLUE;
	short int black = BLACK;
	
	short int level = y_bot - val;			// Get height to draw level (value from 0-255 mapped to height of box)
	if(val>0){
		fillBox(x_left, level, x_right, y_bot, (int)RED);	// Fill in gain level blue
	}
	if(val<63){
		fillBox(x_left, y_top, x_right, level, (int)BLACK);	// Fill in empty space white
	}
}

void drawString(const char* str, short int xStart, short int yStart, short int text_color, short int bg_color) {
	while (*str) {
		drawChar(*str++, xStart, yStart, text_color, bg_color);
		xStart += 16;
	}
}
 
void lcdDelay(unsigned char lcdDel)
{
	for(unsigned char i = 0; i < lcdDel; i++)
		asm("NOP");
}

int min(short int a, short int b){
	return !(b<a)?a:b;
}

int max(short int a, short int b){
	return !(b>a)?a:b;
}