//*****************************************************************************
//	USB Protocol Functions - AVR_DU series
//
//	HID Mouse implementation with Control (EP0) and Interrupt (EP1.IN) channels
//	
//	1. EP0 IN multipacket during enumeration - be aware if you reduce BUFSIZE
//	----------
//	The first GET_DESCRIPTOR Device request from the host is wLength = 8,
//	because the host doesn't yet know bMaxPacketSize0. It reads the first 8 bytes,
//	ends the data stage early, and then issues a bus RESET.  If BUFSIZE = 8 or 16,
//	and in multipacket mode, the device will assume further IN tokens will arrive
//	requesting the remaining bytes.  The host does not issue further IN tokens in
//	this case and TRNCOMPL never asserts.  The busy-wait loop hangs forever.  To
//  prevent this condition, set the BUFIZE >= Device_Descriptor size.
//
//	2. Notes on using printf to display the setup packet
//	----------
//	Class-specific requests such as HID SET_IDLE, SET_PROTOCOL, and GET_REPORT, typically
//	arrive after SET_CONFIGURATION, but a host may send some class requests earlier.
//
//	80 06 0600 0000 000A = Get Device Qualifier - stall response required for single speed devices.
//	21 0a 0000 0000 0000 = HID Set Idle - wValueL=0 means NAK request if data has not changed since last request.
//          ^^
//	The host gives a control transfer status stage roughly 50 ms, so a long debug string could cause problems.
//
//	Author:		Richard
//	Created:	2026-04-13 - Initial HID implementation
//	Updated:	2026-10-25 - Windows session keep alive mouse implementation
//
//*****************************************************************************
#include "config.h"							// F_CPU
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <stdio.h>							// printf
#include <stdbool.h>
#include <string.h>							// memcpy
#include <stddef.h>							// wchar_t
#include "usb.h"							// Structs, Enums, Externals
#include "usb_internal.h"					// Device and Endpoint inline functions
#include "led.h"							// LED functions
#include "mouse_descriptor.h"				// Mouse Descriptor

// Specifier forces inline independent of optimization level
#define ALWAYS_INLINE inline __attribute__((always_inline))

// Number of seconds before sending Mouse Report
#define NAK_TIMEOUT_SEC			20

//*****************************************************************************
//	Private Function declarations
//*****************************************************************************
static void usb_handleSetupRequest(void);
static void usb_getDescriptor(void);
static void usb_sendDescriptor(const void *descriptor, uint16_t descriptorLength);
static void usb_getStringDescriptor(void);
static void usb_setAddress(void);
static void usb_setConfiguration(void);
static void usb_hidSetIdle(void);
static void usb_hidGetReport(void);
static void usb_hidSetReport(void);

//*****************************************************************************
//	Variable declarations
//*****************************************************************************
//	Endpoint table  
Usb_Endpoint_Table_t endpointTable;

//	Endpoint buffers
Usb_SetupPacket_t setupBuffer;						// EP0.OUT Setup packet buffer (from Host)
uint8_t controlBuffer[64];							// EP0.IN Control packet buffer (to Host)
Usb_HidMouseReport_t hidMouseReportBuffer;			// EP1.IN Mouse report buffer (to Host)

// ISR modified flags
volatile Usb_State_t usbState = Usb_State_Default;	// Current USB state
volatile bool usbSuspended = false;					// Suspend=true, Resume=false
volatile bool usbMouseReportRequestFlag = false;
volatile bool usbMouseReportSentFlag = false;

//*****************************************************************************
//	Initialize USB interface
//	- Configure USB peripheral
//	- Initialize Endpoints
//	- Setup default Mouse Report data
//	- Initialize TCA0 (NAK clear timer)
//	- Enable Device and Endpoint interrupts
//*****************************************************************************
void usb_init(void)
{
	// -- Configure USB peripheral --
	
	// Enable internal 3.3 V regulator
	SYSCFG.VUSBCTRL = SYSCFG_USBVREG_bm;
	
	// Set the maximum endpoint number and enable USB peripheral
	USB0.CTRLA = EP_MAX_ADDR | USB_ENABLE_bm;

	// Enable has requested the PLL, wait for lock
	usbWaitPllLock();		
	
	
	// -- Initialize Endpoints --
	
	// Initialize Endpoint table pointer
	USB0.EPPTR = (uint16_t)endpointTable.EP;
	
	// Initialize EP0.OUT - Setup packet
	endpointTable.EP[EP0].OUT.CTRL		=	USB_TYPE_CONTROL_gc |	
											USB_TCDSBL_bm |					// Disable global interrupt sync
											USB_BUFSIZE_DEFAULT_BUF8_gc;	// Setup packet must be 8 bytes
	endpointTable.EP[EP0].OUT.DATAPTR	=	(uint16_t)&setupBuffer;			// Setup packet buffer
	endpointTable.EP[EP0].OUT.STATUS	=	0;
	endpointTable.EP[EP0].OUT.CNT		=	0;
	endpointTable.EP[EP0].OUT.MCNT		=	0;
	
	
	// Initialize EP0.IN - Control packet (Descriptors) --
	
	endpointTable.EP[EP0].IN.CTRL		=	USB_TYPE_CONTROL_gc |
											USB_TCDSBL_bm |					// Disable global interrupt sync
											USB_MULTIPKT_bm | USB_AZLP_bm |	// Multipacket and Automatic ZLP
											USB_BUFSIZE_DEFAULT_BUF64_gc;	// Must match DeviceDescriptor.MaxPacketSize0
	endpointTable.EP[EP0].IN.DATAPTR	=	(uint16_t)controlBuffer;		// Control IN and OUT buffer (Datasheet 27.3.2.2 SETUP)
	endpointTable.EP[EP0].IN.STATUS		=	0;
	endpointTable.EP[EP0].IN.CNT		=	0;
	endpointTable.EP[EP0].IN.MCNT		=	0;
	
	// Initialize Interrupt EP1.IN packet (Mouse report) --
	endpointTable.EP[EP1].IN.CTRL		=	USB_TYPE_BULKINT_gc |
											USB_BUFSIZE_DEFAULT_BUF8_gc;	// Must match .HidReportInEndpoint.wMaxPacketSize
	endpointTable.EP[EP1].IN.DATAPTR	=	(uint16_t)&hidMouseReportBuffer;// HID Report buffer
	endpointTable.EP[EP1].IN.STATUS		=	0;
	endpointTable.EP[EP1].IN.CNT		=	0;
	endpointTable.EP[EP1].IN.MCNT		=	0;	
	
	
	// -- Initialize Mouse report data --

	// Windows Host assumes mouse activity when NAK is cleared, no relative movement is necessary,
	// this data does not change.
	hidMouseReportBuffer.buttons = 0b00000000;
	hidMouseReportBuffer.x = 0x00;
	hidMouseReportBuffer.y = 0x00;
				
	// Number of bytes to transfer on EP1.IN
	endpointTable.EP[EP1].IN.CNT = 3;
	
	
	// -- Initialize TCA0: free running EP1.IN NAK clear timer --
	
	// Enable overflow interrupt
	TCA0_SINGLE_INTCTRL = TCA_SINGLE_OVF_bm;
	
	// PER = ((F_CPU / DIV1024 * 1sec) - 1)
	TCA0_SINGLE_PER = 23437U;
	
	// Configure prescaler and start
	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc | TCA_SINGLE_ENABLE_bm;	
	
	
	// -- Enable USB interrupts --
	
	// Enable Device interrupts
	USB0.INTCTRLA = USB_RESET_bm | USB_STALLED_bm;

	// Enable Endpoint interrupts
	USB0.INTCTRLB = USB_SETUP_bm | USB_TRNCOMPL_bm;
}

//*****************************************************************************
//	Public Functions and Accessors
//*****************************************************************************
// Attach to USB bus
void usb_connect(void)
{
	usbAttach();
	
	usbState = Usb_State_Powered;
}

// Detach from USB bus
void usb_disconnect(void)
{
	usbDetach();
	
	usbState = Usb_State_Detached;
}

// Returns the value of the USB state
Usb_State_t usb_state(void)
{
	return usbState;
}

// Returns the value of the Suspend flag: true=Suspend, false=Resume
bool usb_isSuspended(void)
{
	return usbSuspended;
}

// Arms the endpoint: Host will collect the Report on its next IN token
void usb_mouseReportArm(void)
{
	usbMouseReportSentFlag = false;	
	
	usbClearInNak(EP1);
}

// Returns true/false if timer has signaled report is to be sent and clears the request flag
bool usb_mouseReportReadyToSend_testAndClear(void)
{
	bool ready;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		ready = usbMouseReportRequestFlag;
		usbMouseReportRequestFlag = false;
	}
	return ready;
}

// Returns the Report sent status and clears the sent flag
bool usb_mouseReportSent_testAndClear(void)
{
	bool sent;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		sent = usbMouseReportSentFlag;
		usbMouseReportSentFlag = false;
	}
	return sent;
}

//*****************************************************************************
// TCA0 interrupt handler
// Timer is used to delay clearing NAK on the mouse endpoint until the timeout
// interval has been reached.  A tick counter is used to extend the range.
//*****************************************************************************
ISR(TCA0_OVF_vect)
{
	static uint8_t overflowTickCounter = 0;
	
	// Clear interrupt flag
	TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
	
	// Extend TCA0 timer range
	overflowTickCounter++;
	
	if (overflowTickCounter >= NAK_TIMEOUT_SEC)
	{
		// Signal to main loop
		usbMouseReportRequestFlag = true;
		
		// Reset the overflow counter
		overflowTickCounter = 0;
	}
}

//*****************************************************************************
// USB Bus (Device) interrupt handler - INTFLAGSA
//*****************************************************************************
ISR(USB0_BUSEVENT_vect)
{
	// Snapshot only enabled flags
	volatile uint8_t intFlags = USB0.INTFLAGSA & USB0.INTCTRLA;
	
	// Reset
	if (intFlags & USB_RESET_bm)
	{
		USB0.INTFLAGSA = USB_RESET_bm; 
		
		// Reset device address
		usbSetDeviceAddress(0x00);
		usbEnableSuspend();
		
		// Reset the USB states
		usbState = Usb_State_Default;
		usbSuspended = false;
		
		return;
	}

	// Suspend
	if (intFlags & USB_SUSPEND_bm)
	{
		USB0.INTFLAGSA = USB_SUSPEND_bm;

		usbDisableSuspend();
		usbEnableResume();
	
		usbSuspended = true;		
		
		return;
	}

	// Resume
	if (intFlags & USB_RESUME_bm)
	{
		USB0.INTFLAGSA = USB_RESUME_bm;

		usbDisableResume();
		usbEnableSuspend();
		
		usbSuspended = false;		
		
		return;
	}

	// Stall
	if (intFlags & USB_STALLED_bm)
	{
		USB0.INTFLAGSA = USB_STALLED_bm;
		
		usbClearStallRequest();		
		
		return;
	}
}

//*****************************************************************************
// USB Transaction (Endpoint) interrupt handler - INTFLAGSB
//*****************************************************************************
ISR(USB0_TRNCOMPL_vect)
{
	// Handle EP0 Setup Packet
	if (USB0.INTFLAGSB & USB_SETUP_bm)
	{
		USB0.INTFLAGSB = USB_SETUP_bm;
		
		usbClearSetup();
		
		// Process the Setup Packet
		usb_handleSetupRequest();
	}
	
	// Handle EP1.IN Transfer
	if (USB0.INTFLAGSB & USB_TRNCOMPL_bm)
	{
		USB0.INTFLAGSB = USB_TRNCOMPL_bm;
		
		if (endpointTable.EP[EP1].IN.STATUS & USB_TRNCOMPL_bm) 
		{	
			usbClearInTransaction(EP1);
			
			// The EP1.IN report has been successfully received by the Host
			usbMouseReportSentFlag = true;
		}
	}
}

//*****************************************************************************
//	Process the Setup Request
//*****************************************************************************
static void usb_handleSetupRequest(void)
{
	// Debug - print Setup Packet
	//printf("%02x %02x %04x %04x %04x\n", setupBuffer.bmRequestType, setupBuffer.bRequest, setupBuffer.wValue, setupBuffer.wIndex, setupBuffer.wLength);
	
	// Decode Setup request
	switch (setupBuffer.bmRequestType & BM_REQUEST_TYPE_MASK)
	{
		case Device_Standard:
			switch (setupBuffer.bRequest)
			{
				case Standard_GetDescriptor:
					usb_getDescriptor();
					break;
				case Standard_SetAddress:
					usb_setAddress();
					break;
				case Standard_SetConfiguration:
					usb_setConfiguration();
					break;
				default:
					// Stall unsupported Standard_Request (bRequest)
					usbEnableStallRequest();
					break;
			}
			break;
		case Device_Class:
			switch (setupBuffer.bRequest)
			{
				case Hid_GetReport:	// Not used for mouse
					usb_hidGetReport();
					break;
				case Hid_SetReport:	// Not used for mouse
					usb_hidSetReport();
					break;
				case Hid_SetIdle:
					usb_hidSetIdle();
					break;					
				default:
					// Stall unsupported Class_Request (bRequest)
					usbEnableStallRequest();
					break;
			}
			break;					
		default:
			// Stall unsupported Request_Type (bmRequestType)
			usbEnableStallRequest();
			break;
	}
}

//*****************************************************************************
//	Get Descriptor (bRequest = 0x06)
//	Data stage of a Control read - Host specifies the Descriptor request.
//*****************************************************************************
static void usb_getDescriptor(void)
{
	// Decode Descriptor Type
	switch (setupBuffer.wValueH)
	{
		case Descriptor_Device:
			usb_sendDescriptor(&DeviceDescriptor, DeviceDescriptor.bLength);
			break;
		case Descriptor_Configuration:
			usb_sendDescriptor(&ConfigurationDescriptor, ConfigurationDescriptor.Configuration.wTotalLength);
			break;
		case Descriptor_Hid:
			usb_sendDescriptor(&ConfigurationDescriptor.Hid, ConfigurationDescriptor.Hid.bLength);
			break;
		case Descriptor_HidReport:
			usb_sendDescriptor(&HidReportDescriptor, ConfigurationDescriptor.Hid.wDescriptorLength);
			break;
		case Descriptor_String:
			usb_getStringDescriptor();
			break;			
		default:
			// Stall unsupported Descriptor_Type (wValueH)
			usbEnableStallRequest();
			break;
	}
}

//*****************************************************************************
//	Get String Descriptor (bRequest = 0x06, wValueH = 0x03)
//	Host specifies the String Descriptor index in wValueL.
//*****************************************************************************
static void usb_getStringDescriptor(void)
{
	// Decode String Index
	switch (setupBuffer.wValueL)
	{
		case String_Language:
			usb_sendDescriptor(&LanguageString, LanguageString.bLength);
			break;
		case String_Manufacturer:
			usb_sendDescriptor(&ManufacturerString, ManufacturerString.bLength);
			break;
		case String_Product:
			usb_sendDescriptor(&ProductString, ProductString.bLength);
			break;
		case String_SerialNumber:
			usb_sendDescriptor(&SerialNumberString, SerialNumberString.bLength);
			break;
		default:
			// Stall unsupported String Index (wValueL)
			usbEnableStallRequest();
			break;
	}
}

//*****************************************************************************
//	Send Descriptor to the Host (Device -> Host)
//	Send the lesser of the descriptor length and the host's requested wLength, 
//	then complete the status stage.
//*****************************************************************************
static void usb_sendDescriptor(const void *descriptor, uint16_t descriptorLength)
{
	// -- Data Stage (IN) --

	// The host may request fewer bytes than the descriptor length
	uint16_t length = (setupBuffer.wLength < descriptorLength) ? setupBuffer.wLength : descriptorLength;

	// Copy the descriptor to the EP0 buffer
	memcpy(controlBuffer, descriptor, length);

	// Set the endpoint packet length
	endpointTable.EP[EP0].IN.CNT = length;
	endpointTable.EP[EP0].IN.MCNT = 0;

	// Stop NAKing EP0.IN so the buffer is sent on the host's next IN token
	usbClearInNak(EP0);

	// Wait for TRNCOMPL indicating the data was sent and the host has acknowledged it
	usbWaitInTransactionComplete(EP0);
	usbClearInTransaction(EP0);

	// -- Status Stage (OUT) --

	// Stop NAKing EP0.OUT so the host's ZLP is accepted, this device replies with an ACK
	usbClearOutNak(EP0);

	// Wait for TRNCOMPL confirming the host's status ZLP was received
	usbWaitOutTransactionComplete(EP0);
	usbClearOutTransaction(EP0);
}

//*****************************************************************************
//	Set Address (bRequest = 0x05) Host -> Device
//	Host assigns the device a new USB address (wValue). The new address takes
//	effect only after the status stage completes.
//*****************************************************************************
static void usb_setAddress(void)
{
	// Save the new address: it is not applied yet because the status stage must complete using the old address
	uint8_t address = setupBuffer.wValueL;

	// No Data Stage (control transfer with no data)

	// -- Status Stage --

	// Load a ZLP into the EP0.IN buffer
	endpointTable.EP[EP0].IN.CNT = 0;
	endpointTable.EP[EP0].IN.MCNT = 0;

	// Stop NAKing EP0.IN so the ZLP is sent on the host's next IN token
	usbClearInNak(EP0);

	// Wait for TRNCOMPL confirming the ZLP was sent and the host has acknowledged it
	usbWaitInTransactionComplete(EP0);
	usbClearInTransaction(EP0);
	
	// -- End of Status Stage --

	// Status stage is done, switch to the new address
	usbSetDeviceAddress(address);
	
	// Device is now in the Addressed state
	usbState = Usb_State_Address;
}

//*****************************************************************************
//	Set Configuration (bRequest = 0x09) Host -> Device
//	Host selects the active configuration. This completes enumeration for this 
//	single configuration device.
//*****************************************************************************
static void usb_setConfiguration(void)
{
	// No Data Stage (control transfer with no data)

	// -- Status Stage --

	// Load a ZLP into the EP0 IN buffer
	endpointTable.EP[EP0].IN.CNT = 0;
	endpointTable.EP[EP0].IN.MCNT = 0;

	// Stop NAKing EP0.IN so the ZLP is sent on the host's next IN token
	usbClearInNak(EP0);

	// Wait for TRNCOMPL confirming the ZLP was sent and the host has acknowledged it
	usbWaitInTransactionComplete(EP0);
	usbClearInTransaction(EP0);
	
	// Status stage done: device is now in the Configured state
	usbState = Usb_State_Configured;	
}

//*****************************************************************************
//	Set Idle (bRequest = 0x0A) Host -> Device
//	Host indicates the expected EP1.IN endpoint reporting interval
//	wValueL = 0 -> indefinite (i.e., report data change only) 
//*****************************************************************************
static void usb_hidSetIdle(void)
{
	// No Data Stage (control transfer with no data)
	
	// -- Status Stage --

	// Respond with ZLP
	endpointTable.EP[EP0].IN.CNT = 0;
	endpointTable.EP[EP0].IN.MCNT = 0;
		
	// Stop NAKing EP0.IN so the ZLP is sent on the host's next IN token
	usbClearInNak(EP0);
	
	// Wait for TRNCOMPL confirming the ZLP was sent and the host has acknowledged it
	usbWaitInTransactionComplete(EP0);
	usbClearInTransaction(EP0);
}

//*****************************************************************************
//	Hid Get Report (bRequest = 0x01) Device -> Host
//	Not used for mouse
//*****************************************************************************
static void usb_hidGetReport(void)
{
	// -- Data Stage (IN) --
	
	// Respond with ZLP
	endpointTable.EP[EP0].IN.CNT = 0;
	endpointTable.EP[EP0].IN.MCNT = 0;

	// Stop NAKing EP0.IN so the ZLP is sent on the host's next IN token
	usbClearInNak(EP0);
	
	// Wait for TRNCOMPL confirming the ZLP was sent and the host has acknowledged it
	usbWaitInTransactionComplete(EP0);
	usbClearInTransaction(EP0);
	
	// -- Status Stage --

	// Stop NAKing EP0.OUT so the host's ZLP is accepted (this device sends an ACK)
	usbClearOutNak(EP0);
	
	// Wait for TRNCOMP confirming the host's status ZLP was received and this device has acknowledged it
	usbWaitOutTransactionComplete(EP0);
	usbClearOutTransaction(EP0);
}

//*****************************************************************************
//	Hid Set Report (bRequest = 0x09) Host -> Device
//	Not used for mouse
//*****************************************************************************
static void usb_hidSetReport(void)
{
	// -- Data Stage (OUT) --
	
	// Stop NAKing EP0.OUT so the host's data is accepted (this device sends an ACK)
	usbClearOutNak(EP0);
	
	// Wait for TRNCOMP confirming the host's status data was received	
	usbWaitOutTransactionComplete(EP0);
	usbClearOutTransaction(EP0);
		
	// -- Status Stage --

	// Respond with ZLP
	endpointTable.EP[EP0].IN.CNT = 0;
	endpointTable.EP[EP0].IN.MCNT = 0;

	// Stop NAKing EP0.IN so the ZLP is sent on the host's next IN token
	usbClearInNak(EP0);
	
	// Wait for TRNCOMPL confirming the ZLP was sent and the host has acknowledged it
	usbWaitInTransactionComplete(EP0);
	usbClearInTransaction(EP0);
}
