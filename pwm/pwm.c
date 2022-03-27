#include <XC.h>

#pragma config FNOSC = FRCPLL       // Internal Fast RC oscillator (8 MHz) w/ PLL
#pragma config FPLLIDIV = DIV_2     // Divide FRC before PLL (now 4 MHz)
#pragma config FPLLMUL = MUL_20     // PLL Multiply (now 80 MHz)
#pragma config FPLLODIV = DIV_2     // Divide After PLL (now 40 MHz) see figure 8.1 in datasheet for more info
#pragma config FWDTEN = OFF         // Watchdog Timer Disabled
#pragma config FPBDIV = DIV_1       // PBCLK = SYCLK

// Defines
#define SYSCLK 40000000L

#define PWM_FREQ    200000L
#define DUTY_CYCLE  50

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

// Initially from here:
// http://umassamherstm5.org/tech-tutorials/pic32-tutorials/pic32mx220-tutorials/pwm
void Init_pwm (void)
{
    // OC1 can be assigned to PA0, PB3, PB4, PB15, and PB7(in use).
    // Check TABLE 11-2: OUTPUT PIN SELECTION in datasheet.
    // Set OC1 to pin PA0 (pin 2 of DIP 28) with peripheral pin select
    RPA0Rbits.RPA0R = 0x0005;
 
    // Configure standard PWM mode for output compare module 1
    OC1CON = 0x0006; 
 
	T2CONbits.TCKPS=0x0; // Set pre-scaler to 1
	
    // A write to PRy configures the PWM frequency
    // PR = [FPB / (PWM Frequency * TMR Prescale Value)] – 1
    PR2 = (SYSCLK / (PWM_FREQ*1)) - 1;
 
    // A write to OCxRS configures the duty cycle
    // : OCxRS / PRy = duty cycle
    OC1RS = (PR2 + 1) * ((float)DUTY_CYCLE / 100);

 	T2CON = 0x0;
    T2CONSET = 0x8000;      // Enable Timer2, prescaler 1:1
    OC1CONSET = 0x8000;     // Enable Output Compare Module 1
}

void Set_pwm (unsigned char val)
{
	OC1RS = (PR2 + 1) * ((float)val / 256.0);
}

int main(void)
{
	volatile unsigned long t=0;
	unsigned char myduty=0;

	DDPCON = 0;
	CFGCON = 0;
	
	Init_pwm();

    // loop indefinitely
	while (1)
	{
		t++;
		if(t==20000)
		{
			t = 0;
			Set_pwm(myduty++);
		}
	}
 
    return 1;
}