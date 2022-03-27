#include <XC.h>
#include <stdio.h>
#include <stdlib.h>
 
// Configuration Bits (somehow XC32 takes care of this)
#pragma config FNOSC = FRCPLL       // Internal Fast RC oscillator (8 MHz) w/ PLL
#pragma config FPLLIDIV = DIV_2     // Divide FRC before PLL (now 4 MHz)
#pragma config FPLLMUL = MUL_20     // PLL Multiply (now 80 MHz)
#pragma config FPLLODIV = DIV_2     // Divide After PLL (now 40 MHz)
 
#pragma config FWDTEN = OFF         // Watchdog Timer Disabled
#pragma config FPBDIV = DIV_1       // PBCLK = SYCLK

// Defines
#define SYSCLK 40000000L
#define Baud2BRG(desired_baud)( (SYSCLK / (16*desired_baud))-1)

// Use the core timer to wait for 1 ms.
void wait_1ms(void)
{
    unsigned int ui;
    _CP0_SET_COUNT(0); // resets the core timer count

    // get the core timer count
    while ( _CP0_GET_COUNT() < (SYSCLK/(2*1000)) );
}

void delayMs(int len)
{
	while(len--) wait_1ms();
}

void UART2Configure(int baud_rate)
{
    // Peripheral Pin Select (Check TABLE 11-1: INPUT PIN SELECTION in page 130 of "DS60001168")
    U2RXRbits.U2RXR = 4;    //SET RX to RB8
    RPB9Rbits.RPB9R = 2;    //SET RB9 to TX

    U2MODE = 0;         // disable autobaud, TX and RX enabled only, 8N1, idle=HIGH
    U2STA = 0x1400;     // enable TX and RX
    U2BRG = Baud2BRG(baud_rate); // U2BRG = (FPb / (16*baud)) - 1
    
    U2MODESET = 0x8000;     // enable UART2
}

// Needed to by scanf() and gets()
int _mon_getc(int canblock)
{
	char c;
	
    if (canblock)
    {
	    while( !U2STAbits.URXDA); // wait (block) until data available in RX buffer
	    c=U2RXREG;
	    if(c=='\r') c='\n'; // When using PUTTY, pressing <Enter> sends '\r'.  Ctrl-J sends '\n'
		return (int)c;
    }
    else
    {
        if (U2STAbits.URXDA) // if data available in RX buffer
        {
		    c=U2RXREG;
		    if(c=='\r') c='\n';
			return (int)c;
        }
        else
        {
            return -1; // no characters to return
        }
    }
}

char SPI_Write(char a)
{
	SPI1BUF = a; // write to buffer for TX
	while(SPI1STATbits.SPIRBF==0); // wait for transfer complete
	return SPI1BUF; // read the received value
}

/* Pinout for DIP28 PIC32MX130
1 MCLR                                    28 AVDD 
2 VREF+/CVREF+/AN0/C3INC/RPA0/CTED1/RA0   27 AVSS 
3 VREF-/CVREF-/AN1/RPA1/CTED2/RA1         26 AN9/C3INA/RPB15/SCK2/CTED6/PMCS1/RB15
4 PGED1/AN2/C1IND/C2INB/C3IND/RPB0/RB0    25 CVREFOUT/AN10/C3INB/RPB14/SCK1/CTED5/PMWR/RB14
5 PGEC1/AN3/C1INC/C2INA/RPB1/CTED12/RB1   24 AN11/RPB13/CTPLS/PMRD/RB13
6 AN4/C1INB/C2IND/RPB2/SDA2/CTED13/RB2    23 AN12/PMD0/RB12
7 AN5/C1INA/C2INC/RTCC/RPB3/SCL2/RB3      22 PGEC2/TMS/RPB11/PMD1/RB11
8 VSS                                     21 PGED2/RPB10/CTED11/PMD2/RB10
9 OSC1/CLKI/RPA2/RA2                      20 VCAP
10 OSC2/CLKO/RPA3/PMA0/RA3                19 VSS
11 SOSCI/RPB4/RB4                         18 TDO/RPB9/SDA1/CTED4/PMD3/RB9
12 SOSCO/RPA4/T1CK/CTED9/PMA1/RA4         17 TCK/RPB8/SCL1/CTED10/PMD4/RB8
13 VDD                                    16 TDI/RPB7/CTED3/PMD5/INT0/RB7
14 PGED3/RPB5/PMD7/RB5                    15 PGEC3/RPB6/PMD6/RB6
*/

// These are the pins used for SPI:
// PIN 3:  MOSI
// PIN 4:  CSn
// PIN 5:  MISO
// PIN 25: SCK

void config_SPI(void)
{
	int rData;

	// SDI1 can be assigned to any of these pins (table TABLE 11-1: INPUT PIN SELECTION):
	//0000 = RPA1; 0001 = RPB5; 0010 = RPB1; 0011 = RPB11; 0100 = RPB8
	SDI1Rbits.SDI1R=0b0010; //SET SDI1 to RB1, pin 5 of DIP28
    ANSELB &= ~(1<<1); // Set RB1 as a digital I/O
    TRISB |= (1<<1);   // configure pin RB1 as input
	
	// SDO1 can be configured to any of these pins by writting 0b0011 to the corresponding register.
	// Check TABLE 11-2: OUTPUT PIN SELECTION (assuming the pin exists in the dip28 package): 
	// RPA1, RPB5, RPB1, RPB11, RPB8, RPA8, RPC8, RPA9, RPA2, RPB6, RPA4
	// RPB13, RPB2, RPC6, RPC1, RPC3
	RPA1Rbits.RPA1R=0b0011; // config RA1 (pin 3) for SD01
	
	// SCK1 is assigned to pin 25 and can not be changed, but it MUST be configured as digital I/O
	// because it is configured as analog input by default.
    ANSELB &= ~(1<<14); // Set RB14 as a digital I/O
    TRISB |= (1<<14);   // configure RB14 as input
    
    // CSn is assigned to RB0, pin 4.  Also onfigure as digital output pin.
    ANSELB &= ~(1<<0); // Set RB0 as a digital I/O
	TRISBbits.TRISB0 = 0;
	LATBbits.LATB0 = 1;	

	SPI1CON = 0; // Stops and resets the SPI1.
	rData=SPI1BUF; // clears the receive buffer
	SPI1STATCLR=0x40; // clear the Overflow
	SPI1CON=0x10008120; // SPI ON, 8 bits transfer, SMP=1, Master,  SPI mode unknown (looks like 0,0)
	SPI1BRG=128; // About 150kHz clock frequency
}

// Read 10 bits from the MCP3008 ADC converter using the recommended
// format in the datasheet.
unsigned int volatile GetADC(char channel)
{
	unsigned char val;
	unsigned int adc;
	
	LATBbits.LATB0 = 0;  // Select/enable ADC.
	
	val=SPI_Write(0x01);

	val=SPI_Write((channel*0x10)|0x80); // Send single/diff* bit, D2, D1, and D0 bits.
	adc=((val & 0x03)*0x100); // val contains the high part of the result.
	
	val=SPI_Write(0x55); // Dummy transmission to get low part of result.
	adc+=val; // val contains the low part of the result.

	LATBbits.LATB0 = 1;	// Deselect ADC.
	
	return adc;
}

// The Voltage reference input to the MCP3008.  For best results, measure it and put here.
#define VREF 3.3
 
void main(void) 
{
	unsigned char rbuf[6];
 	int joy_x, joy_y, off_x, off_y, acc_x, acc_y, acc_z;
 	char but1, but2;
	
	DDPCON = 0;
	CFGCON = 0;

    UART2Configure(115200);  // Configure UART2 for a baud rate of 115200

	delayMs(500); // Give PuTTY a chance to start before sending text
	
	printf("\x1b[2J\x1b[1;1H"); // Clear screen using ANSI escape sequence.
	printf ("MCP3008 SPI test program\r\n"
	        "File: %s\r\n"
	        "Compiled: %s, %s\r\n\r\n",
	        __FILE__, __DATE__, __TIME__);
    
	config_SPI();
    
    while(1)
	{
		printf("V0=%5.3f, V1=%5.3f\r", (GetADC(0)*VREF)/1023.0, (GetADC(1)*VREF)/1023.0);
		delayMs(500);
	}

}

