//*****************************************************************************
//
//	USART.c
//
//	To use printf(), #include <stdio.h> in your code includes section
//
//	Author:		Richard
//	Created:	2026-11-07
//
//*****************************************************************************

#include "config.h"
#include <avr/io.h>
#include <stdio.h>		// printf
#include "usart.h"

// Setup stream
FILE USART_stream = FDEV_SETUP_STREAM(usart_printChar, NULL, _FDEV_SETUP_WRITE);			// Print only
//FILE USART_stream = FDEV_SETUP_STREAM(USART_printChar, USART_inputChar, _FDEV_SETUP_RW);	// Print and input


// Initialize the USART, returns the internal BAUD register value
uint16_t usart_init(BaudRate_t baud)
{
	// Bind stream to stdout to use printf
	stdout = &USART_stream;
	
	// Configure output pins (AVR16DU14 pinout)
	USART_TX_PORT.DIRSET = USART_TX_PIN;			// TXD
		
	// Configure USART mode
	USART0.CTRLC =	USART_CMODE_ASYNCHRONOUS_gc |	// Async mode
					USART_PMODE_DISABLED_gc |		// No parity
					USART_SBMODE_1BIT_gc |			// 1 stop bit
					USART_CHSIZE_8BIT_gc;			// 8 bits
					
	// Enable transmitter
	USART0.CTRLB = USART_TXEN_bm; // | USART_RXEN_bm;
	
	// Configure baud rate last to avoid initial garbage byte
	USART0.BAUD = (uint16_t)USART_BAUD_RATE(baud);	

	return USART0.BAUD;
}

// Print Character
int usart_printChar(char data, FILE *stream)
{
	// Wait for empty transmit buffer
	while (!(USART0.STATUS & USART_DREIF_bm));

	USART0.TXDATAL = data;

	return 0;
}


// Character Input Function
//static int usart_inputChar(FILE *stream)
//{
	//// Wait for data to be received
	//while (!(USART0.STATUS & USART_RXCIF_bm));
//
	//// Return received data
	//return USART0.RXDATAL;
//}
//
//// Print Character
//static void usart_putChar(char data)
//{
	//// Wait for empty transmit buffer
	//while (!(USART0.STATUS & USART_DREIF_bm));
//
	//USART0.TXDATAL = data;
//}
//
//// Check if any character is available (non-blocking)
//int usart_isCharAvailable(void)
//{
	//return (USART0.STATUS & USART_RXCIF_bm);
//}