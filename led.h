//*****************************************************************************
//	USB LED Functions - AVR_DU Test
//
//	Author: Richard
//	Date:	2026-04-13
//
//*****************************************************************************
#ifndef LED_H
#define LED_H

#include "config.h"				// F_CPU
#include <avr/io.h>
#include <stdbool.h>

// Status LEDs
#define LED_SUSPENDED_PORT			PORTD
#define LED_SUSPENDED_PIN			PIN6_bm
#define LED_ACTIVITY_PORT			PORTA
#define LED_ACTIVITY_PIN			PIN0_bm

typedef enum {
	OFF = false,
	ON = true
}led_t;

void inline led_init(void)
{
	PORTD.DIRSET = LED_SUSPENDED_PIN;
	PORTA.DIRSET = LED_ACTIVITY_PIN;
}

void inline led_suspended(led_t state)
{
	if (state == true) 
		LED_SUSPENDED_PORT.OUTSET = LED_SUSPENDED_PIN;
	else
		LED_SUSPENDED_PORT.OUTCLR = LED_SUSPENDED_PIN;
}

void inline led_activity(led_t state)
{
	if (state == true)
		LED_ACTIVITY_PORT.OUTSET = LED_ACTIVITY_PIN;
	else
		LED_ACTIVITY_PORT.OUTCLR = LED_ACTIVITY_PIN;
}

#endif