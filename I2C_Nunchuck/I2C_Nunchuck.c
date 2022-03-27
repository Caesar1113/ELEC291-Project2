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
    // Peripheral Pin Select
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

void Init_I2C2(void)
{
	// Configure pin RB2, used for SDA2 (pin 6 of DIP28) as digital I/O
    ANSELB &= ~(1<<2); // Set RB2 as a digital I/O
    TRISB |= (1<<2);   // configure pin RB2 as input
    CNPUB |= (1<<2);   // Enable pull-up resistor for RB2
	// Configure pin RB3, used for SCL2 (pin 7 of DIP28) as digital I/O
    ANSELB &= ~(1<<3); // Set RB3 as a digital I/O
    TRISB |= (1<<3);   // configure pin RB3 as input
    CNPUB |= (1<<3);   // Enable pull-up resistor for RB3

	I2C2BRG = 0x0C6; // SCL2 = 100kHz with SYSCLK = 40MHz (See table 24-1 in page 24-32, 830 in pdf)
	I2C2CONbits.ON=1;//turn on I2C
}
 
int I2C_byte_write(unsigned char saddr, unsigned char maddr, unsigned char data)
{
	I2C2CONbits.SEN = 1;
	while(I2C2CONbits.SEN); //Wait till Start sequence is completed

	I2C2TRN = (saddr << 1);
	while(I2C2STATbits.TRSTAT); // Check Tx complete

	I2C2TRN = maddr;
	while(I2C2STATbits.TRSTAT); // Check Tx complete

	I2C2TRN = data;
	while(I2C2STATbits.TRSTAT); // Check Tx complete

	I2C2CONbits.PEN = 1; // Terminate communication with stop signal
	while(I2C2CONbits.PEN); // Wait till stop sequence is completed

	return 0;
}

int I2C_burst_write(unsigned char saddr, unsigned char maddr, int byteCount, unsigned char* data)
{
	I2C2CONbits.SEN = 1;
	while(I2C2CONbits.SEN); //Wait till Start sequence is completed

	I2C2TRN = (saddr << 1);
	while(I2C2STATbits.TRSTAT); // Check Tx complete

	I2C2TRN = maddr;
	while(I2C2STATbits.TRSTAT); // Check Tx complete

    for (; byteCount > 0; byteCount--)
    {
		I2C2TRN = *data++; // send data
		while(I2C2STATbits.TRSTAT); // Check Tx complete
	}

	I2C2CONbits.PEN = 1; // Terminate communication with stop signal
	while(I2C2CONbits.PEN); // Wait till stop sequence is completed

	return 0;
}

int I2C_burstRead(char saddr, char maddr, int byteCount, unsigned char* data)
{
	// First we send the address we want to read from:
	I2C2CONbits.SEN = 1;
	while(I2C2CONbits.SEN); //Wait till Start sequence is completed

	I2C2TRN = (saddr << 1);
	while(I2C2STATbits.TRSTAT); // Check Tx complete

	I2C2TRN = maddr;
	while(I2C2STATbits.TRSTAT); // Check Tx complete

	I2C2CONbits.PEN = 1; // Terminate communication with stop signal
	while(I2C2CONbits.PEN); // Wait till stop sequence is completed
	
	// Second: we gatter the data sent by the slave device
	I2C2CONbits.SEN = 1;
	while(I2C2CONbits.SEN); //Wait till Start sequence is completed
	
	I2C2TRN = ((saddr << 1) | 1); // The receive address has the least significant bit set to 1
	while(I2C2STATbits.TRSTAT); // Check Tx complete
  
    for (; byteCount > 0; byteCount--)
    {
    	I2C2CONbits.RCEN=1;
		while(I2C2CONbits.RCEN); // Wait for a byte to arrive
		*data++=I2C2RCV;
		if(byteCount==0) // We are done, send NACK
		{
			I2C2CONbits.ACKDT=1; // Selects NACK
			I2C2CONbits.ACKEN = 1;
			while(I2C2CONbits.ACKEN); // Wait till NACK sequence is completed
		}
		else // Not done yet, send an ACK 
		{
			I2C2CONbits.ACKDT=0; // Selects ACK
			I2C2CONbits.ACKEN = 1;
			while(I2C2CONbits.ACKEN); // Wait till ACK sequence is completed
		}
	}
	
	I2C2CONbits.PEN = 1; // Terminate communication with stop signal
	while(I2C2CONbits.PEN); // Wait till stop sequence is completed

	return 0;
}

void nunchuck_init(int print_extension_type)
{
	unsigned char buf[6];
	
	I2C_byte_write(0x52, 0xF0, 0x55);
	I2C_byte_write(0x52, 0xFB, 0x00);
		 
	// Read the extension type from the register block.
	// For the original Nunchuk it should be: 00 00 a4 20 00 00.
	I2C_burstRead(0x52, 0xFA, 6, buf);
	if(print_extension_type)
	{
		printf("Extension type: %02x  %02x  %02x  %02x  %02x  %02x\r\n", 
			buf[0],  buf[1], buf[2], buf[3], buf[4], buf[5]);
	}

	// Send the crypto key (zeros), in 3 blocks of 6, 6 & 4.
	buf[0]=0; buf[1]=0; buf[2]=0; buf[3]=0; buf[4]=0; buf[5]=0;
	
	I2C_byte_write(0x52, 0xF0, 0xAA);
	I2C_burst_write(0x52, 0x40, 6, buf);
	I2C_burst_write(0x52, 0x40, 6, buf);
	I2C_burst_write(0x52, 0x40, 4, buf);
}

void nunchuck_getdata(unsigned char * s)
{
	unsigned char i;

	// Start measurement
	I2C_burstRead(0x52, 0x00, 6, s);

	// Decrypt received data
	for(i=0; i<6; i++)
	{
		s[i]=(s[i]^0x17)+0x17;
	}
}
 
void main(void) 
{
	unsigned char rbuf[6];
 	int joy_x, joy_y, off_x, off_y, acc_x, acc_y, acc_z;
 	char but1, but2;
	
	CFGCON = 0;

    UART2Configure(115200);  // Configure UART2 for a baud rate of 115200
    Init_I2C2(); // Configure I2C2

	delayMs(1000); // Give PuTTY a chance to start before sending text
	
	printf("\x1b[2J\x1b[1;1H"); // Clear screen using ANSI escape sequence.
	printf ("PIC32MX130 I2C WII Nunchuck test program\r\n"
	        "File: %s\r\n"
	        "Compiled: %s, %s\r\n\r\n",
	        __FILE__, __DATE__, __TIME__);
    
	nunchuck_init(1);
	delayMs(100);

	nunchuck_getdata(rbuf);

	off_x=(int)rbuf[0]-128;
	off_y=(int)rbuf[1]-128;
	printf("Offset_X:%4d Offset_Y:%4d\r\n", off_x, off_y);

	while(1)
	{
		nunchuck_getdata(rbuf);

		joy_x=(int)rbuf[0]-128-off_x;
		joy_y=(int)rbuf[1]-128-off_y;
		acc_x=rbuf[2]*4; 
		acc_y=rbuf[3]*4;
		acc_z=rbuf[4]*4;

		but1=(rbuf[5] & 0x01)?1:0;
		but2=(rbuf[5] & 0x02)?1:0;
		if (rbuf[5] & 0x04) acc_x+=2;
		if (rbuf[5] & 0x08) acc_x+=1;
		if (rbuf[5] & 0x10) acc_y+=2;
		if (rbuf[5] & 0x20) acc_y+=1;
		if (rbuf[5] & 0x40) acc_z+=2;
		if (rbuf[5] & 0x80) acc_z+=1;
		
		printf("Buttons(Z:%c, C:%c) Joystick(%4d, %4d) Accelerometer(%3d, %3d, %3d)\x1b[0J\r",
			   but1?'1':'0', but2?'1':'0', joy_x, joy_y, acc_x, acc_y, acc_z);
		fflush(stdout);
		delayMs(100);
	}
}
