#include <XC.h>
#include <sys/attribs.h>
#include <stdio.h>
#include <stdlib.h>

#pragma config FNOSC = FRCPLL       // Internal Fast RC oscillator (8 MHz) w/ PLL
#pragma config FPLLIDIV = DIV_2     // Divide FRC before PLL (now 4 MHz)
#pragma config FPLLMUL = MUL_20     // PLL Multiply (now 80 MHz)
#pragma config FPLLODIV = DIV_2     // Divide After PLL (now 40 MHz) see figure 8.1 in datasheet for more info
#pragma config FWDTEN = OFF         // Watchdog Timer Disabled
#pragma config FPBDIV = DIV_1       // PBCLK = SYCLK

// Defines
#define SYSCLK 40000000L
#define DEF_FREQ 22050L
#define Baud2BRG(desired_baud)( (SYSCLK / (16*desired_baud))-1)

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

void uart_putc (unsigned char c)
{
    while( U2STAbits.UTXBF); // wait while TX buffer full
    U2TXREG = c; // send single character to transmit buffer
}

void uart_puts (char * buff)
{
	while (*buff)
	{
		uart_putc(*buff);
		buff++;
	}
}

unsigned char uart_getc (void)
{
	unsigned char c;
	
	while( !U2STAbits.URXDA); // wait (block) until data available in RX buffer
	c=U2RXREG;
	return c;
}

void SetupTimer1 (void)
{
	// Explanation here:
	// https://www.youtube.com/watch?v=bu6TTZHnMPY
	__builtin_disable_interrupts();
	PR1 =(SYSCLK/DEF_FREQ)-1; // since SYSCLK/FREQ = PS*(PR1+1)
	TMR1 = 0;
	T1CONbits.TCKPS = 0; // Pre-scaler: 1
	T1CONbits.TCS = 0; // Clock source
	T1CONbits.ON = 1;
	IPC1bits.T1IP = 5;
	IPC1bits.T1IS = 0;
	IFS0bits.T1IF = 0;
	IEC0bits.T1IE = 1;
	
	INTCONbits.MVEC = 1; //Int multi-vector
	__builtin_enable_interrupts();
}

volatile unsigned int Tick_Counter=0;
volatile unsigned char Second_Flag=1;

void __ISR(_TIMER_1_VECTOR, IPL5SOFT) Timer1_Handler(void)
{
	unsigned char c;
	
	LATBbits.LATB6 = !LATBbits.LATB6; // Toggle pin RB6 (used to check the right frequency)
	IFS0CLR=_IFS0_T1IF_MASK; // Clear timer 1 interrupt flag, bit 4 of IFS0
	
	if(Second_Flag==0)
	{
		Tick_Counter++;
		if(Tick_Counter==22050)
		{
		   Second_Flag=1;
		   Tick_Counter=0;
		}
	}
	else
	{
		Tick_Counter=0;
	}
}

/* Pinout for DIP28 PIC32MX130:

                                   MCLR (1   28) AVDD 
  VREF+/CVREF+/AN0/C3INC/RPA0/CTED1/RA0 (2   27) AVSS 
        VREF-/CVREF-/AN1/RPA1/CTED2/RA1 (3   26) AN9/C3INA/RPB15/SCK2/CTED6/PMCS1/RB15
   PGED1/AN2/C1IND/C2INB/C3IND/RPB0/RB0 (4   25) CVREFOUT/AN10/C3INB/RPB14/SCK1/CTED5/PMWR/RB14
  PGEC1/AN3/C1INC/C2INA/RPB1/CTED12/RB1 (5   24) AN11/RPB13/CTPLS/PMRD/RB13
   AN4/C1INB/C2IND/RPB2/SDA2/CTED13/RB2 (6   23) AN12/PMD0/RB12
     AN5/C1INA/C2INC/RTCC/RPB3/SCL2/RB3 (7   22) PGEC2/TMS/RPB11/PMD1/RB11
                                    VSS (8   21) PGED2/RPB10/CTED11/PMD2/RB10
                     OSC1/CLKI/RPA2/RA2 (9   20) VCAP
                OSC2/CLKO/RPA3/PMA0/RA3 (10  19) VSS
                         SOSCI/RPB4/RB4 (11  18) TDO/RPB9/SDA1/CTED4/PMD3/RB9
         SOSCO/RPA4/T1CK/CTED9/PMA1/RA4 (12  17) TCK/RPB8/SCL1/CTED10/PMD4/RB8
                                    VDD (13  16) TDI/RPB7/CTED3/PMD5/INT0/RB7
                    PGED3/RPB5/PMD7/RB5 (14  15) PGEC3/RPB6/PMD6/RB6
*/

#define PIN_PERIOD (PORTB&(1<<5))

// GetPeriod() seems to work fine for frequencies between 200Hz and 700kHz.
long int GetPeriod (int n)
{
	int i;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	
    _CP0_SET_COUNT(0); // resets the core timer count
	while (PIN_PERIOD!=0) // Wait for square wave to be 0
	{
		if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
	}

    _CP0_SET_COUNT(0); // resets the core timer count
	while (PIN_PERIOD==0) // Wait for square wave to be 1
	{
		if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
	}
	
    _CP0_SET_COUNT(0); // resets the core timer count
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD!=0) // Wait for square wave to be 0
		{
			if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
		}
		while (PIN_PERIOD==0) // Wait for square wave to be 1
		{
			if(_CP0_GET_COUNT() > (SYSCLK/4)) return 0;
		}
	}

	return  _CP0_GET_COUNT();
}

int main(void)
{
    unsigned int T2_overflow;
	long int count;
	float T, f;

	DDPCON = 0;
	CFGCON = 0;
	
	TRISBbits.TRISB6 = 0;
	LATBbits.LATB6 = 0;	
	INTCONbits.MVEC = 1;
	
	SetupTimer1(); // Setup timer 1 and its interrupt
    UART2Configure(115200);  // Configure UART2 for a baud rate of 115200

	// Wait 1 second so PuTTY has a chance to start and display first message
    Second_Flag = 0;
    while(!Second_Flag);

	printf("\x1b[2J\x1b[1;1H"); // Clear screen using ANSI escape sequence.

	printf("Two timers test. Timer 1 is used as reference and Timer 2\r\n"
	       "is used to measure a frequency applied to pin 7 (RB3).\r\n"
	       "This version also measures period at pin 14 (RB5)\r\n");

	PR2 = 0xffff; // When TMR2 hits 0xffff resets back to zero
	T2CONbits.TCKPS = 0; // Pre-scaler: 1.
	T2CONbits.TCS = 1; // External clock
	
	// Select the pin to use as external clock for timer 2.  These are the options:
	// 0 = RPA0, 1 = RPB3, 2 = RPB4, 3 = RPB15, 4 = RPB7
	// Datasheet -> TABLE 11-1: INPUT PIN SELECTION
    ANSELB &= ~(1<<3); // Set RB3 as a digital I/O
    TRISB |= (1<<3);   // configure pin RB3 as input
    CNPUB |= (1<<3);   // Enable pull-up resistor for RB3
	T2CKRbits.T2CKR = 1; // Use RPB3 (pin 7) as input clock

    ANSELB &= ~(1<<5); // Set RB5 as a digital I/O (pin 14)
    TRISB |= (1<<5);   // configure pin RB5 as input
    CNPUB |= (1<<5);   // Enable pull-up resistor for RB5
	
	while(1)
	{
		TMR2 = 0; // Reset timer count
		T2_overflow=0; // Reset overflow count
		T2CONbits.ON = 1; // Start timer  
	    Second_Flag=0;
	    while(!Second_Flag) // while one second has not passed check overflow and wait
		{
			// Check for overflow of timer 2
			if(IFS0&_IFS0_T2IF_MASK)
			{
				IFS0CLR=_IFS0_T2IF_MASK; // Clear overflow flag
				T2_overflow++; // Increment overflow counter
			}
		}
		T2CONbits.ON = 0; // Stop Timer
	    
	    // ANSI escape sequences (https://en.wikipedia.org/wiki/ANSI_escape_code)
	    printf("\x1b[5;1H"); // Go to column 1 of row 5
	    printf("\x1b[31m"); // red letters
	    printf("\x1b#3f=%dHz", T2_overflow*0x10000+TMR2);
	    printf("\x1b[0K"); // Erase to end of line
	    printf("\x1b[6;1H"); // Go to column 1 of row 5
	    printf("\x1b#4f=%dHz", T2_overflow*0x10000+TMR2);
	    printf("\x1b[0K"); // Erase to end of line
	    printf("\x1b[37m"); // white letters
	    printf("\x1b[0K"); // Erase to end of line

	    printf("\x1b[35m"); // magenta letters
		count=GetPeriod(1000);
		if(count>0)
		{
			T=(count*2.0)/(SYSCLK*1000.0);
			f=1/T;
	    	printf("\x1b[8;1H"); // Go to column 1 of row 8
			printf("\x1b#3f=%.2fHz, Count=%ld", f, count);
	        printf("\x1b[0K"); // Erase to end of line
	    	printf("\x1b[9;1H"); // Go to column 1 of row 9
			printf("\x1b#4f=%.2fHz, Count=%ld", f, count);
	        printf("\x1b[0K"); // Erase to end of line
		}
		else
		{
	    	printf("\x1b[8;1H"); // Go to column 1 of row 8
			printf("\x1b#3NO SIGNAL");
	        printf("\x1b[0K"); // Erase to end of line
	    	printf("\x1b[9;1H"); // Go to column 1 of row 9
			printf("\x1b#4NO SIGNAL");
	        printf("\x1b[0K"); // Erase to end of line
		}
	    printf("\x1b[37m"); // white letters
	    printf("\x1b[1;1H"); // Move the cursor away. Go to column 1 of row 1
    }  
 
    return 1;
}