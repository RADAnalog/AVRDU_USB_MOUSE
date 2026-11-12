//*****************************************************************************
//	AVR16DU14 - Windows Session Keep-Alive "mouse shaker"
//
//	Purpose: A minimal HID Mouse implementation that acts as a virtual "mouse shaker" to
//	prevent an attached Windows Host session from timing out.  The device (automatically)
//  replies to Host EP1.IN requests with a NAK and then once every 20 seconds clears the NAK.
//	The mouse report contains coordinate values that are fixed at a relative (0,0) movement.
//	Windows accepts any valid data as "activity" so this approach prevents the Windows
//  session from timing out without actually moving the mouse.
//
//                          _____
//                    GND 1|*    |14 VDD+
//          (IN only) PF6 2|RST  |13 PD7
//                   UPDI 3|     |12 PD6 -> Suspend LED
//         ACT_LED <- PA0 4|TxD  |11 PD5
//           0.47u*   PA1 5|RxD  |10 PD4
//		 GND --||--- VUSB 6|     |9  PC3
//                     D- 7|_____|8  D+
//
//	*Ceramic 0.47u X7R 50V MLCC, e.g. Murata GRM21BR71H474KA88K. 
//	MLCC voltage should be over-spec'd because higher voltage MLCC's maintain their
//	capacitance value over a wider voltage bias.
//
//	NOTES: 
//	The AVRxxDU datasheet refers to a "transaction" as an IN or OUT packet followed by ACK/NAK.
//	USB refers to a "transaction" as the completion of Token->Data->Handshake sequence.
//	
//	This device can't push anything on USB. The host polls, and the endpoint answers 
//	with data instead of a NAK. When NAK is cleared, the report goes out on the Host’s next poll.
//
//  Author : Richard
//	Created: 2026-Oct-22 - Inherit from AVR16DU14 original project
//	Updated: 2026-Oct-31 - NAK timer, refactor code
//
//*****************************************************************************
#include "config.h"			// F_CPU
#include <avr/io.h>
#include <avr/xmega.h>		// _PROTECTED_WRITE
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>			// printf
#include <stdbool.h>
#include "usb.h"
#include "led.h"
//#include "usart.h"


//*****************************************************************************
//	Function Definitions
//*****************************************************************************
void init(void);


//*****************************************************************************
//	Main
//*****************************************************************************
int main(void)
{
	init();
	led_init();
	usb_init();
	
	// Enable interrupts
	sei();

	// Connect to USB bus
	usb_connect();
	
	while (1)
	{
		if (!usb_isSuspended())
		{
			led_activity(ON);
			led_suspended(OFF);
			
			if (usb_getState() == Usb_State_Configured)
			{
				// Test if the timer timed out and atomically clear
				if (usb_mouseReportReadyToSend_getAndClear())
				{
					// Arm endpoint so the host collects the report on its next IN token
					usb_mouseReportEnable();
				}
				// Test if the endpoint has been received by the Host and atomically clear
				else if (usb_mouseReportSent_getAndClear())
				{
					// Blink Activity LED off briefly to show report was sent
					led_activity(OFF);
					_delay_ms(50);
					led_activity(ON);
				}
			}
		}
		else
		{
			led_activity(OFF);
			led_suspended(ON);
		}	
	}
}

//*****************************************************************************
//	Initialize AVR
//*****************************************************************************
void init(void)
{
	// Set CPU clock to 24MHz, clock autotune mode
	_PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_24M_gc | CLKCTRL_RUNSTBY_bm | CLKCTRL_AUTOTUNE_SOF_gc);
	
	// Wait for system clock to stabilize
	while((CLKCTRL.MCLKSTATUS & CLKCTRL_OSCHFS_bm) == 0);
}
