//*****************************************************************************
//	USB DU internal Device and Endpoint management functions
//
//	This header should only be included in usb.c to maintain private scope
//	Place this header after usb.h in usb.c
//
//	ALWAYS_INLINE will force inline regardless of optimization level
//
//	Note: Setup interrupt automatically clears the TRNCOMPL flag.
//
//	Author:		Richard
//	Created:	2026-04-15
//	Updated:	2026-10-26
//
//*****************************************************************************
#ifndef USB_INTERNAL_H
#define USB_INTERNAL_H

// Specifier forces inline independent of optimization level
#define ALWAYS_INLINE inline __attribute__((always_inline))

//*****************************************************************************
//	External Declarations
//*****************************************************************************
extern Usb_Endpoint_Table_t endpointTable;


//*****************************************************************************
//	USB Device functions (private)
//*****************************************************************************

// Wait for the PLL to lock
static ALWAYS_INLINE void usbWaitPllLock(void)
{
	while ((CLKCTRL.USBPLLSTATUS & CLKCTRL_PLLS_bm) == 0);
}

// Attach to bus
static ALWAYS_INLINE void usbAttach(void)
{
	USB0.CTRLB |= USB_ATTACH_bm;
}

// Detach from bus
static ALWAYS_INLINE void usbDetach(void)
{
	USB0.CTRLB &= ~USB_ATTACH_bm;
}

// Set device address
static ALWAYS_INLINE void usbSetDeviceAddress(uint8_t addr)
{
	// Mask address to ensure bit7 is always '0'
	USB0_ADDR = addr & 0x7F;
}

// Enable Resume interrupt
static ALWAYS_INLINE void usbEnableResume(void)
{
	USB0.INTCTRLA |= USB_RESUME_bm;
}

// Disable Resume interrupt
static ALWAYS_INLINE void usbDisableResume(void)
{
	USB0.INTCTRLA &= ~USB_RESUME_bm;
}

// Enable Suspend interrupt
static ALWAYS_INLINE void usbEnableSuspend(void)
{
	USB0.INTCTRLA |= USB_SUSPEND_bm;
}

// Disable Suspend interrupt
static ALWAYS_INLINE void usbDisableSuspend(void)
{
	USB0.INTCTRLA &= ~USB_SUSPEND_bm;
}

//*****************************************************************************
//	USB Endpoint functions (private)
//*****************************************************************************	

// Wait for Read-Modify-Write operation to complete (Datasheet 27.3.3.1)
static ALWAYS_INLINE void waitUntilRMWSet(void)
{
	while ((USB0.INTFLAGSB & USB_RMWBUSY_bm) != 0)
	{
		// Wait loop
	}
}

// Stalls the Control endpoint until cleared
static ALWAYS_INLINE void usbEnableStallRequest(void)
{
	endpointTable.EP[EP0].IN.CTRL |= USB_DOSTALL_bm;
	endpointTable.EP[EP0].OUT.CTRL |= USB_DOSTALL_bm;
}

// Clears the stall request on the Control endpoint
static ALWAYS_INLINE void usbClearStallRequest(void)
{
	endpointTable.EP[EP0].IN.CTRL &= ~USB_DOSTALL_bm;
	endpointTable.EP[EP0].OUT.CTRL &= ~USB_DOSTALL_bm;
}

// Wait for IN transaction to complete on specified endpoint
static ALWAYS_INLINE void usbWaitInTransactionComplete(uint8_t ep)
{
	while ((endpointTable.EP[ep].IN.STATUS & USB_TRNCOMPL_bm) == 0)
	{
		// Exit loop if host has issued a Reset
		if ((USB0.INTFLAGSA & USB_RESET_bm) != 0)
		{
			break;
		}
	}
}

// Wait for OUT transaction to complete on specified endpoint
static ALWAYS_INLINE void usbWaitOutTransactionComplete(uint8_t ep)
{
	while ((endpointTable.EP[ep].OUT.STATUS & USB_TRNCOMPL_bm) == 0)
	{
		// Exit loop if host has issued a Reset
		if ((USB0.INTFLAGSA & USB_RESET_bm) != 0)
		{
			break;
		}
	}
}

// Clear Setup on Control endpoint
static ALWAYS_INLINE void usbClearSetup(void)
{
	// Clear IN 'setup complete' flag
	waitUntilRMWSet();
	USB0.STATUS[EP0].INCLR = USB_EPSETUP_bm;

	// Clear OUT 'setup complete' flag
	waitUntilRMWSet();
	USB0.STATUS[EP0].OUTCLR = USB_EPSETUP_bm;
}

// Clear 'transaction complete' flag on specified IN endpoint 
static ALWAYS_INLINE void usbClearInTransaction(uint8_t ep)
{
	waitUntilRMWSet();
	USB0.STATUS[ep].INCLR = USB_TRNCOMPL_bm;
}

// Clear 'transaction complete' flag on specified OUT endpoint
static ALWAYS_INLINE void usbClearOutTransaction(uint8_t ep)
{
	waitUntilRMWSet();
	USB0.STATUS[ep].OUTCLR = USB_TRNCOMPL_bm;
}

// Clear NAK on specified IN endpoint
static ALWAYS_INLINE void usbClearInNak(uint8_t ep)
{
	waitUntilRMWSet();
	USB0.STATUS[ep].INCLR = USB_BUSNAK_bm;
}

// Clear NAK on specified OUT endpoint
static ALWAYS_INLINE void usbClearOutNak(uint8_t ep)
{
	waitUntilRMWSet();
	USB0.STATUS[ep].OUTCLR = USB_BUSNAK_bm;
}

#endif