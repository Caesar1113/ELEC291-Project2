// This program uses SPI to communicate with the nRF24L01.
//
// These are the pins used to connect the nRF24L01 and the PIC32MX130.  There
// are a couple of pictures that show the connections in the bread-boarded
// circuit in the project folder.
//
// PIN 3:  MOSI
// PIN 4:  CSn (RB0)
// PIN 5:  MISO
// PIN 25: SCK
// PIN 15: CE (RB6)
//
// Jesus Calvino-Fraga (2018-2020)
//

#include <XC.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nrf24.h"
 
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
	    putchar(c); // Echo is active in blcoking mode
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

uint8_t spi_transfer(uint8_t tx)
{
	SPI1BUF = tx; // write to buffer for TX
	while(SPI1STATbits.SPIRBF==0); // wait for transfer complete
	return SPI1BUF; // read and return the received value
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
    
    // CSn is assigned to RB0, pin 4.  Also configure as digital output pin.
    ANSELB &= ~(1<<0); // Set RB0 as a digital I/O
	TRISBbits.TRISB0 = 0;
	LATBbits.LATB0 = 1;	

    // CE is assigned to RB6, pin 15.  Also configure as digital output pin.
	TRISBbits.TRISB6 = 0;
	LATBbits.LATB6 = 1;	

	SPI1CON = 0; // Stops and resets the SPI1.
	rData=SPI1BUF; // clears the receive buffer
	SPI1STATCLR=0x40; // clear the Overflow
	// SPI ON, 8 bits transfer, SMP=1, Master, SPI mode (0,1)? Anyhow, the nRF24L01
	// samples in the falling edge of the clock, therefore SMP must be '1'
	SPI1CON=0x10008320;
	SPI1BRG=128; // About 150kHz clock frequency
}

void safe_gets(char *s, int n)
{
	unsigned char j=0;
	unsigned char c;
	
	while(1)
	{
		c=_mon_getc(1);
		if ( (c=='\n') || (c=='\r') ) break;
		if(j<(n-1))
		{
			s[j]=c;
			j++;
		}
	}
	s[j]=0;
}

uint8_t temp;
uint8_t data_array[32];
uint8_t tx_address[] = "TXADD";
uint8_t rx_address[] = "RXADD";
 
void main(void) 
{
	DDPCON = 0;
	CFGCON = 0;

    UART2Configure(115200);  // Configure UART2 for a baud rate of 115200

	delayMs(500); // Give PuTTY a chance to start before sending text
	
	printf("\x1b[2J\x1b[1;1H"); // Clear screen using ANSI escape sequence.
	printf ("nRF24L01 SPI test program\r\n"
	        "File: %s\r\n"
	        "Compiled: %s, %s\r\n\r\n",
	        __FILE__, __DATE__, __TIME__);

	// Pin 14 (RB5) has the test push button
    ANSELB &= ~(1<<5); // Set RB5 as a digital I/O
    TRISB |= (1<<5);   // configure pin RB5 as input
    CNPUB |= (1<<5);   // Enable pull-up resistor for RB5

	// Pin 10 (RA3) selects between transmitter and receiver after a reset.
    TRISA |= (1<<3);   // configure pin RA3 as input
    CNPUA |= (1<<3);   // Enable pull-up resistor for RA3
    
	config_SPI();
    
    nrf24_init(); // init hardware pins
    nrf24_config(120,32); // Configure channel and payload size

    /* Set the device addresses */
    if(PORTA&(1<<3))
    {
    	printf("Set as transmitter\r\n");
	    nrf24_tx_address(tx_address);
	    nrf24_rx_address(rx_address);
    }
    else
    {
    	printf("Set as receiver\r\n");
	    nrf24_tx_address(rx_address);
	    nrf24_rx_address(tx_address);
    }

    while(1)
    {    
        if(nrf24_dataReady())
        {
            nrf24_getData(data_array);
        	printf("IN: %s\r\n", data_array);
        }
        
        if(U2STAbits.URXDA) // Something arrived from the serial port?
        {
        	safe_gets(data_array, sizeof(data_array));
		    printf("\r\n");    
	        nrf24_send(data_array);        
		    while(nrf24_isSending());
		    temp = nrf24_lastMessageStatus();
			if(temp == NRF24_MESSAGE_LOST)
		    {                    
		        printf("> Message lost\r\n");    
		    }
			nrf24_powerDown();
    		nrf24_powerUpRx();
		}
		
		if((PORTB&(1<<5))==0)
		{
			while((PORTB&(1<<5))==0);
			strcpy(data_array, "Button test");
	        nrf24_send(data_array);
		    while(nrf24_isSending());
		    temp = nrf24_lastMessageStatus();
			if(temp == NRF24_MESSAGE_LOST)
		    {                    
		        printf("> Message lost\r\n");    
		    }
			nrf24_powerDown();
    		nrf24_powerUpRx();
		}
    }
}

