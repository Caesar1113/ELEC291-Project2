#include <XC.h>
#include <stdio.h>
#include <stdint.h>

#pragma config FNOSC = FRCPLL       // Internal Fast RC oscillator (8 MHz) w/ PLL
#pragma config FPLLIDIV = DIV_2     // Divide FRC before PLL (now 4 MHz)
#pragma config FPLLMUL = MUL_20     // PLL Multiply (now 80 MHz)
#pragma config FPLLODIV = DIV_2     // Divide After PLL (now 40 MHz) see figure 8.1 in datasheet for more info
#pragma config FWDTEN = OFF         // Watchdog Timer Disabled
#pragma config FSOSCEN = OFF        // Secondary Oscillator Enable (Disabled)

// Defines
#define SYSCLK 40000000L
#define LCD_D4 LATBbits.LATB15
#define LCD_D5 LATBbits.LATB14
#define LCD_D6 LATBbits.LATB13
#define LCD_D7 LATBbits.LATB12
#define LCD_RS LATBbits.LATB3
//RW is not connected for this code
#define LCD_E  LATBbits.LATB2

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
	TRISBbits.TRISB3 = 0;
	TRISBbits.TRISB2 = 0;
	
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
	// The accumulator in the 8051 is bit addressable!
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

void main (void)
{
	unsigned char j;
	char c;
	
	waitms(500); // Gives time to putty to start before sending text
	
	// Configure the LCD
	pin_innit();
	LCD_4BIT();

	WriteCommand(0x01);
	while (1) {	
   	// Display something in the LCD
//		LCDprint("LCD 4-bit test:", 1, 1);
//		LCDprint("Hello, World!", 2, 1);
		WriteData(0x80);
	}
}
