/*
* ----------------------------------------------------------------------------
* THE COFFEEWARE LICENSEù (Revision 1
* <ihsan@kehribar.me> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a coffee in return.
* -----------------------------------------------------------------------------
* Please define your platform specific functions in this file ...
* -----------------------------------------------------------------------------
*/

#include <XC.h>
#include <stdint.h>

/* ------------------------------------------------------------------------- */
void nrf24_setupPins()
{
	// The pins are configured in file SPI_nRF24L01.c, in the function config_SPI

}
/* ------------------------------------------------------------------------- */
void nrf24_ce_digitalWrite(uint8_t state)
{
    if(state)
    {
    	LATBbits.LATB6 = 1;
    }
    else
    {
    	LATBbits.LATB6 = 0;
    }
}
/* ------------------------------------------------------------------------- */
void nrf24_csn_digitalWrite(uint8_t state)
{
    if(state)
    {
    	LATBbits.LATB0 = 1;
    }
    else
    {
    	LATBbits.LATB0 = 0;
    }
}
/* ------------------------------------------------------------------------- */
void nrf24_sck_digitalWrite(uint8_t state)
{
    // Nothing to do.  The SPI hardware handles it.
}
/* ------------------------------------------------------------------------- */
void nrf24_mosi_digitalWrite(uint8_t state)
{
    // Nothing to do.  The SPI hardware handles it.
}
/* ------------------------------------------------------------------------- */
uint8_t nrf24_miso_digitalRead()
{
    // Nothing to do.  The SPI hardware handles it.
}
/* ------------------------------------------------------------------------- */
