/*
Project Group Members:
Sizhe Yan 22164982
John Ye 43883347
Xuchen Zhang 61531547
Jack Ye 12815999
Shuo Wu 30408751
M.Berke Karadayi 77766392

Additional Notes & Bonus Features:
All basic function are completed(To speed up the collection process, we feed the coins to the robot in a reasonable manners to shorten the video)

Bonus features including LCD display, displaying the number of coins collected and when it's finished, the LCD screen shows "Mission Complete". 
Also, when picking up the coins, LED will blink. We also control the speed of the arm rotates. 
When it's collect the coin, it will slow down to make it more reliable, 
when the arm is repositioning, it goes fast to shorten the time to collect the coins. 
In the end, the robot will dance to celebrate (robot move back and forth).

https://youtu.be/7JAxhl0nGXI link of our video in YouTube
Here is the backup link of our demo video https://b23.tv/uFK1Qi7
*/

#include <XC.h>
#include <sys/attribs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
// Configuration Bits (somehow XC32 takes care of this)
#pragma config FNOSC = FRCPLL       // Internal Fast RC oscillator (8 MHz) w/ PLL
#pragma config FPLLIDIV = DIV_2     // Divide FRC before PLL (now 4 MHz)
#pragma config FPLLMUL = MUL_20     // PLL Multiply (now 80 MHz)
#pragma config FPLLODIV = DIV_2     // Divide After PLL (now 40 MHz) 
#pragma config FWDTEN = OFF         // Watchdog Timer Disabled
#pragma config FPBDIV = DIV_1       // PBCLK = SYCLK
#pragma config FSOSCEN = OFF        // Turn off secondary oscillator on A4 and B4

// Defines
#define SYSCLK 40000000L
#define FREQ 100000L // We need the ISR for timer 1 every 10 us
#define Baud2BRG(desired_baud)( (SYSCLK / (16*desired_baud))-1)


#define EdgeVoltage 0.1
#define EdgeVoltage2 0.1
#define MinCoin1Period 19040
#define MaxCoin1period 19060

#define NoCoinPeriod 7200.0


#define LCD_D4 LATAbits.LATA2
#define LCD_D5 LATAbits.LATA3	
#define LCD_D6 LATBbits.LATB13
#define LCD_D7 LATBbits.LATB12

#define LCD_RS LATBbits.LATB10 
//RW is not connected for this code
#define LCD_E  LATAbits.LATA4
#define LED LATBbits.LATB6

#define CHARS_PER_LINE 16

volatile int ISR_pwm1=150, ISR_pwm2=60, ISR_cnt=0;

// The Interrupt Service Routine for timer 1 is used to generate one or more standard
// hobby servo signals.  The servo signal has a fixed period of 20ms and a pulse width
// between 0.6ms and 2.4ms.
void __ISR(_TIMER_1_VECTOR, IPL5SOFT) Timer1_Handler(void)
{
	IFS0CLR=_IFS0_T1IF_MASK; // Clear timer 1 interrupt flag, bit 4 of IFS0

	ISR_cnt++;
	if(ISR_cnt==ISR_pwm1)
	{
		LATBbits.LATB15 = 0;
	}
	if(ISR_cnt==ISR_pwm2)
	{
		LATBbits.LATB14 = 0;
	}
	if(ISR_cnt>=2000)
	{
		ISR_cnt=0; // 2000 * 10us=20ms
		LATBbits.LATB15 = 1;
		LATBbits.LATB14 = 1;
	}
}

void SetupTimer1 (void)
{
	// Explanation here: https://www.youtube.com/watch?v=bu6TTZHnMPY
	__builtin_disable_interrupts();
	PR1 =(SYSCLK/FREQ)-1; // since SYSCLK/FREQ = PS*(PR1+1)
	TMR1 = 0;
	T1CONbits.TCKPS = 0; // 3=1:256 prescale value, 2=1:64 prescale value, 1=1:8 prescale value, 0=1:1 prescale value
	T1CONbits.TCS = 0; // Clock source
	T1CONbits.ON = 1;
	IPC1bits.T1IP = 5;
	IPC1bits.T1IS = 0;
	IFS0bits.T1IF = 0;
	IEC0bits.T1IE = 1;
	
	INTCONbits.MVEC = 1; //Int multi-vector

}

// Use the core timer to wait for 1 ms.
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

void uart_puts(char * s)
{
	while(*s)
	{
		putchar(*s);
		s++;
	}
}

char HexDigit[]="0123456789ABCDEF";
void PrintNumber(long int val, int Base, int digits)
{ 
	int j;
	#define NBITS 32
	char buff[NBITS+1];
	buff[NBITS]=0;

	j=NBITS-1;
	while ( (val>0) | (digits>0) )
	{
		buff[j--]=HexDigit[val%Base];
		val/=Base;
		if(digits!=0) digits--;
	}
	uart_puts(&buff[j+1]);
}

// Good information about ADC in PIC32 found here:
// http://umassamherstm5.org/tech-tutorials/pic32-tutorials/pic32mx220-tutorials/adc
void ADCConf(void)
{
    AD1CON1CLR = 0x8000;    // disable ADC before configuration
    AD1CON1 = 0x00E0;       // internal counter ends sampling and starts conversion (auto-convert), manual sample
    AD1CON2 = 0;            // AD1CON2<15:13> set voltage reference to pins AVSS/AVDD
    AD1CON3 = 0x0f01;       // TAD = 4*TPB, acquisition time = 15*TAD 
    AD1CON1SET=0x8000;      // Enable ADC
}

int ADCRead(char analogPIN)
{
    AD1CHS = analogPIN << 16;    // AD1CHS<16:19> controls which analog pin goes to the ADC
 
    AD1CON1bits.SAMP = 1;        // Begin sampling
    while(AD1CON1bits.SAMP);     // wait until acquisition is done
    while(!AD1CON1bits.DONE);    // wait until conversion done
 
    return ADC1BUF0;             // result stored in ADC1BUF0
}

void ConfigurePins(void)
{
    // Configure pins as analog inputs
    ANSELBbits.ANSB2 = 1;   // set RB2 (AN4, pin 6 of DIP28) as analog pin
    TRISBbits.TRISB2 = 1;   // set RB2 as an input
    ANSELBbits.ANSB3 = 1;   // set RB3 (AN5, pin 7 of DIP28) as analog pin
    TRISBbits.TRISB3 = 1;   // set RB3 as an input
    
	// Configure digital input pin to measure signal period
	ANSELB &= ~(1<<5); // Set RB5 as a digital I/O (pin 14 of DIP28)
    TRISB |= (1<<5);   // configure pin RB5 as input
    CNPUB |= (1<<5);   // Enable pull-up resistor for RB5
    
    // Configure output pins
	TRISAbits.TRISA0 = 0; // pin  2 of DIP28
	TRISAbits.TRISA1 = 0; // pin  3 of DIP28
	TRISBbits.TRISB0 = 0; // pin  4 of DIP28
	TRISBbits.TRISB1 = 0; // pin  5 of DIP28
	TRISBbits.TRISB4 = 0; // pin 11 of DIP28
		
	TRISBbits.TRISB15 = 0;
	LATBbits.LATB15 = 0;
	
	TRISBbits.TRISB14 = 0;
	LATBbits.LATB14 = 0;
	INTCONbits.MVEC = 1;
	
	TRISAbits.TRISA2 = 0;
	TRISAbits.TRISA3 = 0;
	TRISBbits.TRISB13 = 0;
	TRISBbits.TRISB12 = 0;
	TRISBbits.TRISB10 = 0;
	TRISAbits.TRISA4 = 0;
	TRISBbits.TRISB6 = 0;

	LATAbits.LATA2 = 0;
	LATAbits.LATA3 = 0;
	LATBbits.LATB13 = 0;
	LATBbits.LATB12 = 0;
	LATBbits.LATB6 = 0;

	LATBbits.LATB10 = 0;
	LATAbits.LATA4 = 0;
	
	LCD_D4 = 0;
	LCD_D5 = 0;
	LCD_D6 = 0;
	LCD_D7 = 0;
	
	LCD_RS = 0;
	LCD_E = 0;
}

/*    // Configure pins as analog inputs
    ANSELBbits.ANSB2 = 1;   // set RB2 (AN4, pin 6 of DIP28) as analog pin
    TRISBbits.TRISB2 = 1;   // set RB2 as an input
    ANSELBbits.ANSB3 = 1;   // set RB3 (AN5, pin 7 of DIP28) as analog pin
    TRISBbits.TRISB3 = 1;   // set RB3 as an input
    
    //pin 6,7 is for detectors
    
    
	// Configure digital input pin to measure signal period
	ANSELB &= ~(1<<5); // Set RB5 as a digital I/O (pin 14 of DIP28)
    TRISB |= (1<<5);   // configure pin RB5 as input
    CNPUB |= (1<<5);   // Enable pull-up resistor for RB5
    
    // Configure output pins
	TRISAbits.TRISA0 = 0; // pin  2 of DIP28 
	TRISAbits.TRISA1 = 0; // pin  3 of DIP28
	TRISBbits.TRISB0 = 0; // pin  4 of DIP28
	TRISBbits.TRISB1 = 0; // pin  5 of DIP28
	
	//pin 2-5 is for motor
	
	TRISAbits.TRISA2 = 0; // pin  9 of DIP28
	TRISAbits.TRISA3 = 0; // pin 10 of DIP28
	
	//pin 9,10 is for arm
	
	TRISBbits.TRISB4 = 0; // pin 11 of DIP28
	//could be used as LED
	
	INTCONbits.MVEC = 1;*/
	
//...............................................................arm related.................................
void MoveArm(){
	ISR_pwm1=150, ISR_pwm2=60;
	LED=!LED;
	waitms(500);
	LED=!LED;
	ISR_pwm2=250;
	waitms(500);
	LED=!LED;
	ISR_pwm1=230;
	waitms(500);
	LED=!LED;
	ISR_pwm1=170;
	waitms(500);
	LED=!LED;
	while (ISR_pwm2 > 100){
		ISR_pwm2--;
		waitms(6);
	}	
//	ISR_pwm2=100;
	waitms(500);
	LED=!LED;
	while (ISR_pwm1 > 100){
		ISR_pwm1--;
		waitms(6);
	}	
	//ISR_pwm1=100;
    waitms(500);
    LED = 0;
}  

 
void ArmInit(){

	waitms(100);
	ISR_pwm1=150;
 	waitms(500);
   	ISR_pwm2=60;   //picking up coins (might want to add longer delay)
 	waitms(500);
 	
 	
} 
 
 	
void StartMagnet(){
	TRISBbits.TRISB4 = 1;	
}   	
void StopMagnet(){
	TRISBbits.TRISB4 = 0;
}



//.................................................servo................................
void MoveForward(){
//wheel 1
	TRISAbits.TRISA0 = 0; //pin 2
	TRISAbits.TRISA1 = 1; //pin 3
//wheel 2
	TRISBbits.TRISB0 = 1; // pin  4 of DIP28
	TRISBbits.TRISB1 = 0; // pin  5 of DIP28
}

void MoveBackward(){
//wheel 1
	TRISAbits.TRISA0 = 1;
	TRISAbits.TRISA1 = 0;
//wheel 2
	TRISBbits.TRISB0 = 0; // pin  4 of DIP28
	TRISBbits.TRISB1 = 1; // pin  5 of DIP28
	waitms(175);
}

void Stop(){
//wheel 1
	TRISAbits.TRISA0 = 0;
	TRISAbits.TRISA1 = 0;
//wheel 2
	TRISBbits.TRISB0 = 0; // pin  4 of DIP28
	TRISBbits.TRISB1 = 0; // pin  5 of DIP28
}

void TurnDirectionForCoin(){
//wheel 1

// int t=0;
	TRISAbits.TRISA0 = 1;
	TRISAbits.TRISA1 = 0;
//wheel 2
	TRISBbits.TRISB0 = 1; // pin  4 of DIP28
	TRISBbits.TRISB1 = 0; // pin  5 of DIP28
	
//	for(t=0;t<=5000;t++){
//	TRISBbits.TRISB4=!TRISBbits.TRISB4;
//	}
	
	waitms(20);//Time needed to turn to pick a coin
}
void TurnAnotherDirection(){
//wheel 1

// int t=0;
	TRISAbits.TRISA0 = 0;
	TRISAbits.TRISA1 = 1;
//wheel 2
	TRISBbits.TRISB0 = 0; // pin  4 of DIP28
	TRISBbits.TRISB1 = 1; // pin  5 of DIP28
	
//	for(t=0;t<=5000;t++){
//	TRISBbits.TRISB4=!TRISBbits.TRISB4;
//	}
	
	waitms(20);//Time needed to turn to pick a coin
}
void TurnDirectionForWall(){
//wheel 1

//	int t=0;
	TRISAbits.TRISA0 = 1;
	TRISAbits.TRISA1 = 0;
//wheel 2
	TRISBbits.TRISB0 = 1; // pin  4 of DIP28
	TRISBbits.TRISB1 = 0; // pin  5 of DIP28
	
//	for(t=0;t<=5000;t++){
//	TRISBbits.TRISB4=!TRISBbits.TRISB4;
//	}
	
	waitms(75);//Time needed to turn to pick a coin
}


void MoveSlow(){
	MoveForward();
	waitms(5);
	Stop();
	waitms(5);
	MoveForward();
	
}

//............................................................lcd display...................
int bitExtracted(int number, int k, int p) //needs an int
{
    return (((1 << k) - 1) & (number >> (p - 1)));
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


//.................................................................Detection..........................
int getEdge(){
	int EdgeCounter;
	EdgeCounter=0;
    int adcval;
    float voltage;
	while(EdgeCounter<50001){
	EdgeCounter++;	 	
		if(EdgeCounter==50000)
		{
        	adcval = ADCRead(5); // note that we call pin AN5 (RB3) by it's analog number
        	voltage=adcval*3.3/1023.0;
			EdgeCounter = 0;
			printf("Volage: %f\r\n",voltage);
				return voltage;
}}
}
int getEdge2(){
	int EdgeCounter;
	EdgeCounter=0;
    int adcval;
    float voltage;
    	  	   		
	while(EdgeCounter<50001){
	EdgeCounter++;	
		if(EdgeCounter==50000)
		{
        	adcval = ADCRead(4); // note that we call pin AN5 (RB2) by it's analog number
        	voltage=adcval*3.3/1023.0;
			EdgeCounter = 0;
			printf("Volage2: %f\r\n",voltage);
				return voltage;
}}
}
int getCoin(){
	long int CoinCounter=0;
//	float T, f;
	CoinCounter=GetPeriod(20);
	printf("%d\n",CoinCounter);
	if(CoinCounter>0)
		{	  	   		
//			T=(CoinCounter*2.0)/(SYSCLK*100.0);
//			f=1/T;
			if(CoinCounter<NoCoinPeriod)
				return 1;
			else
				return 0;
		}
	else 
		return 0;
}

void dance(){

	TurnDirectionForCoin();
	waitms(2000);
	TurnAnotherDirection();
	waitms(2000);
	Stop();

}

// In order to keep this as nimble as possible, avoid
// using floating point or printf() on any of its forms!
void main(void)
{	
	unsigned char buf[8];
	int EdgeCounter=0;
	volatile unsigned long t=0;
    int adcval;
    char tempstring []= "# of Coins: " ;

    long int v;
	unsigned long int count, f;
	unsigned char LED_toggle=0;
	float EdgeDetected=0;
	float EdgeDetected2=0;
	int CoinDetected=0;
	volatile unsigned long CurrentPeriod=0;
	DDPCON = 0;
	CFGCON = 0;
	int Coins=0;
    UART2Configure(115200);  // Configure UART2 for a baud rate of 115200
    ConfigurePins();
 
    ADCConf(); // Configure ADC    
	LCD_4BIT();
	WriteCommand(0x01);
//	float EdgeVoltage;
 
	while(1)
	{	LCDprint("# of Coins",1,1);
		sprintf(tempstring,"%d",Coins);
		LCDprint(tempstring,2,1);
		EdgeCounter++;
		EdgeDetected=getEdge();
		EdgeDetected2=getEdge2();
		CoinDetected=getCoin();
		

//..............................................main function

//..............................................................turning to pick coin
		MoveForward(); 
		//printf("%d\n",CoinDetected);
		if(CoinDetected==0 && EdgeDetected<1.0&& EdgeDetected2<0.2){
			//while(CoinDetected==0 && EdgeDetected<1.0&& EdgeDetected2<0.2){
			printf("%d\n",CoinDetected);
		//	}

   		}
   		else if (CoinDetected==1){
   			SetupTimer1();
			__builtin_enable_interrupts();
   			Coins=Coins+1;
   			MoveBackward();
   			waitms(55);
   			Stop();
   			TurnDirectionForCoin();
   			Stop();
   			waitms(60);//Time needed to finish the turn direction operatoion(for picking coin)
   			StartMagnet();
  			MoveArm();
   			waitms(1000);
   			StopMagnet();
   			ArmInit();		
   			__builtin_disable_interrupts();
 		}  
 		else if (EdgeDetected>EdgeVoltage|| EdgeDetected2>EdgeVoltage2){
 			printf("Edge detected");
   			MoveBackward();
			waitms(500);
 			Stop();
 			TurnDirectionForWall();
 			waitms(200);//Time needed to finish the turn direction operatoion(for picking coin)
   			
   		}
		
    		EdgeDetected=0;
    		EdgeDetected2=0;
   			CoinDetected=0;
   		
		if (Coins==20)
		{
			LCDprint("Mission Complete",1,1);
			LCDprint("            ",2,1);
			dance();
			Stop();
			return;
		}
	}
}