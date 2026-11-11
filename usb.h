//*****************************************************************************
//	USB Common elements and public functions
//	
//	Author: Richard
//	Date:	2026-03-10
//
//*****************************************************************************
#ifndef USB_H
#define USB_H

#include "config.h"			// F_CPU
#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>			// printf

#define WORD_ALIGN				__attribute__((aligned(2)))

// ConfigurationDescriptor.bmAttributes options
#define USB_REMOTE_WAKEUP		0x20		// Remote wakeup option
#define USB_BUS_POWERED    		0x80		// 0x80 | USB_REMOTE_WAKEUP
#define USB_SELF_POWERED    	0xC0		// 0xC0 | USB_REMOTE_WAKEUP

// USB Endpoint parameters
#define EP0_SIZE				64			// Endpoint 0 buffer size
#define EP_MAX_ADDR				1			// Maximum endpoint number used
#define EP0						0			// Endpoint 0 (Control)
#define EP1						1			// Endpoint 1 (Mouse)

// HidReport.bEndpointAddress options
#define USB_EP_DIR_OUT			0x00
#define USB_EP_DIR_IN			0x80

#define BM_REQUEST_TYPE_MASK	0b01100000	// setupPacket.bmRequestType mask

// USB states
typedef enum Usb_State {
	Usb_State_Detached			= 0,		// MCU initialized state, not attached to USB bus
	Usb_State_Powered			= 1,		// Attached, but no bus reset received yet
	Usb_State_Default			= 2,		// Reset received; responds on address 0, EP0 only
	Usb_State_Address			= 3,		// Has a unique address, but is not configured
	Usb_State_Configured		= 4,		// SET_CONFIGURATION succeeded, endpoints are live
} Usb_State_t;

// Device Request_Type (requires bmRequestType to be masked with BM_REQUEST_TYPE_MASK)
typedef enum Usb_Device_Request {
	Device_Standard				= 0x00,		// 0b00000000
	Device_Class				= 0x20,		// 0v00100000
	Device_Vendor				= 0x40,		// 0b01000000
	Device_Reserved				= 0x60		// 0b01100000
} Usb_Device_Request_t;

// Standard Device Request (bRequest)
typedef enum Usb_Standard_Request {
	Standard_SetAddress 		= 0x05,
	Standard_GetDescriptor		= 0x06,
	Standard_SetConfiguration	= 0x09,
} Usb_Standard_Request_t;

// HID Device Request (bRequest)
typedef enum Usb_Hid_Request {
	Hid_GetReport				= 0x01,
	Hid_SetReport				= 0x09,
	Hid_SetIdle					= 0x0a
} Usb_Hid_Request_t;

// Descriptor Request Type (wValueH)
typedef enum Usb_Descriptor_Type {
	Descriptor_Device			= 0x01,
	Descriptor_Configuration	= 0x02,
	Descriptor_String			= 0x03,
	Descriptor_DeviceQualifier	= 0x06,
	Descriptor_Hid				= 0x21,
	Descriptor_HidReport		= 0x22
} Usb_Descriptor_Type_t;

// String Request Index (wValueL)
typedef enum Usb_String_Request {
	String_Language				= 0,
	String_Manufacturer			= 1,
	String_Product				= 2,
	String_SerialNumber			= 3
} Usb_String_Request_t;

//*****************************************************************************
//	Setup Packet struct (8 bytes). 
//*****************************************************************************
typedef struct {
	uint8_t						bmRequestType;
	uint8_t						bRequest;
	union {
		uint16_t				wValue;
		struct {
			uint8_t				wValueL;
			uint8_t				wValueH;
		};
	};
	union {
		uint16_t				wIndex;
		struct {
			uint8_t				wIndexL;
			uint8_t				wIndexH;
		};
	};
	uint16_t					wLength;
} Usb_SetupPacket_t;

//*****************************************************************************
//	Endpoint Table struct - word aligned per datasheet 27.5.7
//	FIFO and Frame Number not implemented
//*****************************************************************************
typedef struct WORD_ALIGN {
	//uint8_t					FIFO[(EP_MAX_ADDR + 1) * 2];	// Two FIFO registers per EP
	USB_EP_PAIR_t				EP[EP_MAX_ADDR + 1];			// In/Out Endpoint Register Pair
	//uint16_t	 				FRAMENUM;						// Frame Number
} Usb_Endpoint_Table_t;

//*****************************************************************************
// Public Function Prototypes
//*****************************************************************************
void usb_init(void);
void usb_connect(void);
void usb_disconnect(void);
void usb_mouseReportArm(void);
bool usb_mouseReportReadyToSend_testAndClear(void);
bool usb_mouseReportSent_testAndClear(void);

// -- Public Accessor functions --

// Returns current USB state
Usb_State_t usb_state(void);

// Returns Suspend status.  true=Suspend, false=Resume
bool usb_isSuspended(void);


#endif