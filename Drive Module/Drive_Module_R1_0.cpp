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
void sendChar(char cToSend);
void sendStr(char *sToSend);
char getByte(void);
void getString(char *sToGet);
void decodeString(char *sToDecode);

void sendI2C(unsigned char addr, unsigned char data1, unsigned char data2);
void updatePots(void);

void init(void);
void initUSART(void);
void initI2C(void);

struct driveConfig {
	unsigned char instVol = 0; // Input instrument pre-gain
	unsigned char gain = 0; // LED distortion amount (0 - 100)
	unsigned char bass = 0; // Bandaxall filter LPF setting (0 - 100)
	unsigned char mids = 0; // Bandaxall filter Middle BPF setting (0 - 100)
	unsigned char treble = 0; // Bandaxall filter HPF setting (0 - 100)
	bool cleanOn = true; // Clean or distortion channel
	unsigned char volume = 0; // Master output volume (0 - 100)
};

driveConfig ChanA, ChanB;

int main(void) {
	init();
	initUSART();
	initI2C();
	updatePots();
	
	while (1) {
		
		//updatePots();
		if(USARTD0_STATUS & USART_RXCIF_bm) { // If there is unread data from Main CPU...
			char *x;
			getString(x);
			decodeString(x);
			//unsigned char a = getByte();
			//if(a != 0x10)
			//ChanA.instVol=(4*a);
			//ChanA.instVol=0x00;
		//	ChanA.treble+=20;
			//decodeString(x);
			updatePots();
		}
		//_delay_ms(5);
	}
}

void init(void) {
	OSC.CTRL = 0b00000010; // Enable internal 32MHz oscillator
	while((OSC.STATUS & 0b00000010) == 0); // Wait for the internal oscillator to stabilize
	
	CCP = 0xD8; // Remove code write lock
	CLK.PSCTRL = 0b00000000; // No external clock prescaler
	CCP = 0xD8; // Remove code write lock
	CLK.CTRL = 0b00000001; // Internal 32MHz Oscillator
	
	PORTA.DIR = 0b00001100; // DAC1 and DAC0 outputs (PA2 and PA3)
	
	ADCA.CTRLA = 0x00; // Enable the ADC on PORT A
	ADCA.CTRLB = 0x00; // Disable ADC stuff
	ADCA.REFCTRL = 0x00; // Disable the AREF pins
	
	DACA.CTRLA = 0x00; // Enable the ADC on PORT A
	DACA.CTRLB = 0x00; // Disable ADC stuff
	DACA.CTRLC = 0x00; // Disable the AREF pins
	
	PORTC.DIR = 0b00000011; // i2C lines SCL and SDA (PC1 and PC0)

	PORTC.PIN0CTRL = 0b00101000; // Wired AND configuration with no internal pull-up
	PORTC.PIN1CTRL = 0b00101000; // Wired AND configuration with no internal pull-up
	
	PORTD.DIR = 0b00001000; // All inputs except PD3 (SPI TX)
	PORTD.PIN3CTRL = 0b00000000; // Totem pole configuration
	
	_delay_ms(1000); // Wait for stuff to power up etc
}

void initUSART(void) {
	// Configure SPI interface and speeds etc for USARTD0 @ 57600bps
	USARTD0.BAUDCTRLA = 0x22; // BSEL = 12
	USARTD0.BAUDCTRLB = 0x00; // BSCALE = 4	
	USARTD0.CTRLA = 0x00; // Interrupts off
	USARTD0.CTRLB = 0b00011000; // CLK2X = 0, Enable transmitter and receiver
	USARTD0.CTRLC = 0b00000011; // Asynchronous, No parity, 1 stop bit, 8 data bits
	USARTD0.CTRLD = 0b00000000; // Standard configuration
}

void initI2C(void) {
	TWIC.CTRL = 0x00; // Normal setup, no driver, no timeout
	TWIC.MASTER.BAUD = 0x96; // Gives fi2c = 100kHz
	TWIC.MASTER.CTRLA = 0x08; // Int off, WIEN off, enable TWI master
	TWIC.MASTER.CTRLB = 0x00; // No timeouts or interrupts
	TWIC.MASTER.CTRLC = 0x00; // Only write...
	TWIC.MASTER.STATUS |= 0x01; // Idle....
}

void updatePots(void) {
	sendI2C(0b01011000, 0x00, ChanA.instVol); // Set IC1 Pot 1 to instrument pre-gain	
	sendI2C(0b01011000, 0x20, ChanA.treble); // Set IC1 Pot 2 to current treble	
	sendI2C(0b01011000, 0x40, ChanA.mids); // Set IC1 Pot 3 to current mids	
	sendI2C(0b01011000, 0x60, ChanA.bass); // Set IC1 Pot 4 to current bass	
	if(ChanA.cleanOn) {
		sendI2C(0b01011010, 0x00, 0x00); // Set IC2 Pot 1 to distortion gain
		sendI2C(0b01011010, 0x20, ChanA.gain); // Set IC2 Pot 2 to overdrive gain
		sendI2C(0b01011010, 0x40, 0x00); // Set IC2 Pot 3 to distortion volume
		sendI2C(0b01011010, 0x60, ChanA.volume); // Set IC2 Pot 4 to overdrive volume
	}
	else {
		sendI2C(0b01011010, 0x00, ChanA.gain); // Set IC2 Pot 1 to distortion gain
		sendI2C(0b01011010, 0x20, 0x00); // Set IC2 Pot 2 to overdrive gain
		sendI2C(0b01011010, 0x40, ChanA.volume); // Set IC2 Pot 3 to distortion volume
		sendI2C(0b01011010, 0x60, 0x00); // Set IC2 Pot 4 to overdrive volume	
	}
	sendI2C(0b01011100, 0x00, ChanB.instVol); // Set IC3 Pot 1 to instrument pre-gain
	sendI2C(0b01011100, 0x20, ChanB.treble); // Set IC3 Pot 2 to current treble
	sendI2C(0b01011100, 0x40, ChanB.mids); // Set IC3 Pot 3 to current mids
	sendI2C(0b01011100, 0x60, ChanB.bass); // Set IC3 Pot 4 to current bass
	if(ChanA.cleanOn) {
		sendI2C(0b01011110, 0x00, 0x00); // Set IC4 Pot 1 to distortion gain
		sendI2C(0b01011110, 0x20, ChanB.gain); // Set IC4 Pot 2 to overdrive gain
		sendI2C(0b01011110, 0x40, 0x00); // Set IC4 Pot 3 to distortion volume
		sendI2C(0b01011110, 0x60, ChanB.volume); // Set IC4 Pot 4 to overdrive volume
		
	}
	else {
		sendI2C(0b01011110, 0x00, ChanB.gain); // Set IC4 Pot 1 to distortion gain
		sendI2C(0b01011110, 0x20, 0x00); // Set IC4 Pot 2 to overdrive gain
		sendI2C(0b01011110, 0x40, ChanB.volume); // Set IC4 Pot 3 to distortion volume
		sendI2C(0b01011110, 0x60, 0x00); // Set IC4 Pot 4 to overdrive volume
	}
}

void sendI2C(unsigned char addr, unsigned char data1, unsigned char data2) {
	int loopCnt = 0;
	TWIC.MASTER.ADDR = addr; // Start bit + address
	while(!(TWIC.MASTER.STATUS & 32));	// Wait for CLKHOLD to go high
	while(TWIC.MASTER.STATUS & 16) { // Resend if no NACK received
		TWIC.MASTER.ADDR = addr;
		while(!(TWIC.MASTER.STATUS & 32));
		loopCnt++;
		if(loopCnt > 50)
			return;
	}
	TWIC.MASTER.DATA = data1; // Send data1
	while(!(TWIC.MASTER.STATUS & 32));
	TWIC.MASTER.DATA = data2; // Send data2
	while(!(TWIC.MASTER.STATUS & 32));
	TWIC.MASTER.CTRLC = 0X03; // Send STOP	
	_delay_ms(1);
}

void sendChar(char cToSend) {
	while(!(USARTD0_STATUS & USART_DREIF_bm));
	USARTD0_DATA = cToSend;
}

void sendStr(char *sToSend) {
	while(*sToSend)
	sendChar(*sToSend++);
}

char getByte(void) {
	while(!(USARTD0_STATUS & USART_RXCIF_bm));
	return USARTD0_DATA;
}

void getString(char *strToGet) {
	int len = 0;
	bool done = false;
	while(!done) {
		*strToGet = getByte();
		if(*strToGet == 'C')
			done = true;
		else {
			len++;
			strToGet++;
		}
		if(len == 20) {
			done = true;
		}
	}
}


void decodeString(char *sToDecode) {
	if(*sToDecode++ == 'A') {
		ChanA.treble = *sToDecode++;
		ChanA.mids = *sToDecode++;
		ChanA.bass = *sToDecode++;
		ChanA.gain = *sToDecode++;
		ChanA.instVol = *sToDecode++;
		sToDecode++;
		ChanB.treble = *sToDecode++;
		ChanB.mids = *sToDecode++;
		ChanB.bass = *sToDecode++;
		ChanB.gain = *sToDecode++;
		ChanB.instVol = *sToDecode++;	
	}
	ChanA.mids = 0xff;
	ChanA.instVol = 0xff;
}