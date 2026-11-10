//*****************************************************************************
//
//	USART.h
//
//	Specific to AVR16DU14
//
//	Author:		Richard
//	Created:	2026-11-07
//
//*****************************************************************************
#ifndef USART_H
#define USART_H

#include "config.h"
#include <avr/io.h>

//#define USART_BAUD_RATE(BAUD_RATE) ((64ul * F_CPU) / (16ul * (uint32_t)BAUD_RATE))
#define USART_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

// TXD pin assignment (AVR16DU14)
#define USART_TX_PORT		PORTA
#define USART_TX_PIN		PIN0_bm

// Valid Baud rates
typedef enum BaudRate {
	BAUD_9600   = 9600UL,
	BAUD_115200 = 115200UL,
	BAUD_230400 = 230400UL,
	BAUD_921600 = 921600UL
} BaudRate_t;

uint16_t usart_init(BaudRate_t baud);
int usart_printChar(char data, FILE *stream);
//static int usart_inputChar(FILE *stream);
//static void usart_putChar(char data);
//int usart_isCharAvailable(void);

#endif
