#include <XC.h>

#pragma config FNOSC = FRCPLL       // Internal Fast RC oscillator (8 MHz) w/ PLL
#pragma config FPLLIDIV = DIV_2     // Divide FRC before PLL (now 4 MHz)
#pragma config FPLLMUL = MUL_20     // PLL Multiply (now 80 MHz)
#pragma config FPLLODIV = DIV_2     // Divide After PLL (now 40 MHz) see figure 8.1 in datasheet for more info
#pragma config FWDTEN = OFF         // Watchdog Timer Disabled
#pragma config FSOSCEN = OFF        // Secondary Oscillator Enable (Disabled)

// Defines
#define SYSCLK 40000000L

void main (void)
{
	volatile unsigned long t=0;
	
	DDPCON = 0;
	
	TRISBbits.TRISB6 = 0;
	LATBbits.LATB6 = 0;	
	INTCONbits.MVEC = 1;

	TRISBbits.TRISB4 = 0;
	LATBbits.LATB4 = 0;	
	INTCONbits.MVEC = 1;

	TRISAbits.TRISA2 = 0;
	LATAbits.LATA2 = 0;	
	INTCONbits.MVEC = 1;
	
	TRISAbits.TRISA3 = 0;
	LATAbits.LATA3 = 0;	
	INTCONbits.MVEC = 1;
	
	TRISAbits.TRISA4 = 0;
	LATAbits.LATA4 = 0;	
	INTCONbits.MVEC = 1;

	while (1)
	{
		t++;
		if(t==500000)
		{
			t = 0;
			LATBbits.LATB6 = !LATBbits.LATB6; // Blink led on RB6
			LATBbits.LATB4 = !LATBbits.LATB4; // Blink led on RB4
			LATAbits.LATA2 = !LATAbits.LATA2; // Blink led on RA2
			LATAbits.LATA3 = !LATAbits.LATA3; // Blink led on RA3
			LATAbits.LATA4 = !LATAbits.LATA4; // Blink led on RA4
		}
	}

}

