#include <XC.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#pragma config FNOSC = FRCPLL       // Internal Fast RC oscillator (8 MHz) w/ PLL
#pragma config FPLLIDIV = DIV_2     // Divide FRC before PLL (now 4 MHz)
#pragma config FPLLMUL = MUL_20     // PLL Multiply (now 80 MHz)
#pragma config FPLLODIV = DIV_2     // Divide After PLL (now 40 MHz) 
#pragma config FWDTEN = OFF         // Watchdog Timer Disabled
#pragma config FPBDIV = DIV_1       // PBCLK = SYCLK
#pragma config FSOSCEN = OFF        // Turn off secondary oscillator on A4 and B4

// Defines

#define SYSCLK 40000000L
#define Baud2BRG(desired_baud)( (SYSCLK / (16*desired_baud))-1)

#define LCD_D4 LATBbits.LATB15
#define LCD_D5 LATBbits.LATB14
#define LCD_D6 LATBbits.LATB13
#define LCD_D7 LATBbits.LATB12
#define LCD_RS LATAbits.LATA2 
//RW is not connected for this code
#define LCD_E  LATAbits.LATA3

#define CHARS_PER_LINE 16

char mystr[CHARS_PER_LINE+1];

// Function to extract k bits from p position
// and returns the extracted value as integer
// Credit to https://www.geeksforgeeks.org/extract-k-bits-given-position-number/



int bitExtracted(int number, int k, int p) //needs an int
{
    return (((1 << k) - 1) & (number >> (p - 1)));
}

void wait_1ms(void)
{
    unsigned int ui;
    _CP0_SET_COUNT(0); // resets the core timer count

    // get the core timer count
    while ( _CP0_GET_COUNT() < (SYSCLK/(2*1000)) );
}

void waitms(int len)
{
	while(len--) wait_1ms();
}


void pin_innit(void)
{
	TRISBbits.TRISB15 = 0;
	TRISBbits.TRISB14 = 0;
	TRISBbits.TRISB13 = 0;
	TRISBbits.TRISB12 = 0;
	TRISAbits.TRISA2 = 0;
	TRISAbits.TRISA3 = 0;
	
	LATBbits.LATB15 = 0;
	LATBbits.LATB14 = 0;
	LATBbits.LATB13 = 0;                                                                                                                                          
	LATBbits.LATB12 = 0;
	LATAbits.LATA2 = 0; 
	LATAbits.LATA3 = 0;
	
	LCD_D4 = 0;
	LCD_D5 = 0;
	LCD_D6 = 0;
	LCD_D7 = 0;
	
	LCD_RS = 0;
	LCD_E = 0;
}

void LCD_pulse(void){
	LCD_E = 1;
	waitms(1);
	LCD_E = 0;
}

void LCD_command (unsigned char x)
{
	int number = x;
	
	LCD_D7= bitExtracted(number,1,8);
	LCD_D6= bitExtracted(number,1,7);
	LCD_D5= bitExtracted(number,1,6);
	LCD_D4= bitExtracted(number,1,5);
	LCD_pulse();
	waitms(1);

	LCD_D7=bitExtracted(number,1,4);
	LCD_D6=bitExtracted(number,1,3);
	LCD_D5=bitExtracted(number,1,2);
	LCD_D4=bitExtracted(number,1,1);
	LCD_pulse();
}

void WriteData (unsigned char x)
{
	LCD_RS=1;
	LCD_command(x);
	waitms(2);
}

void WriteCommand (unsigned char x)
{
	LCD_RS=0;
	LCD_command(x);
	waitms(5);
}


void LCD_4BIT (void)
{
	LCD_E=0; // Resting state of LCD's enable is zero
	//LCD_RW=0; // We are only writing to the LCD in this program
	waitms(20);
	// First make sure the LCD is in 8-bit mode and then change to 4-bit mode
	WriteCommand(0x33);
	WriteCommand(0x33);
	WriteCommand(0x32); // Change to 4-bit mode

	// Configure the LCD
	WriteCommand(0x28);
	WriteCommand(0x0c);
	WriteCommand(0x01); // Clear screen command (takes some time)
	waitms(20); // Wait for clear screen command to finsih.
}

void LCDprint(char* string, unsigned char line, int clear)
{
	int j;

	WriteCommand(line==2?0xc0:0x80);
	waitms(5);
	for(j=0; string[j]!=0; j++)	WriteData(string[j]);// Write the message
	if(clear==1) for(; j<CHARS_PER_LINE; j++) WriteData(' '); // Clear the rest of the line
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

void main (void)
{
	DDPCON = 0;
	CFGCON = 0;
    UART2Configure(115200);  // Configure UART2 for a baud rate of 115200
    
	unsigned char j;
	char c;
	
	waitms(500); // Gives time to putty to start before sending text
	
	// Configure the LCD
	pin_innit();
	LCD_4BIT();

	WriteCommand(0x01);
	while (1) {	
   	// Display something in the LCD
		LCDprint("LCD 4-bit test:", 1, 1);
		LCDprint("Hello, World!", 2, 1);
	//	WriteData(0x80);
	}
}
