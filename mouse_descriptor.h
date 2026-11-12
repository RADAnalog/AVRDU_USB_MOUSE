//*****************************************************************************
//	USB HID Descriptor - Mouse
//
//	Author: Richard
//	Date:	2026-03-10
//
//*****************************************************************************
#ifndef MOUSE_DESCRIPTOR_H
#define MOUSE_DESCRIPTOR_H

#include <stddef.h>		// wchar_t

#define USB_VID					0x1209		// Open Source VID
#define USB_PID					0x0FFF		// Mouse

// Unicode strings, no commas permitted
#define MANUFACTURER_STR 		L"Black and White"
#define PRODUCT_STR 			L"KeepAlive Mouse"// Displays in Windows setup
#define SERIAL_NUMBER_STR 		L"0001"

// Device Descriptor (USB 2.0 Spec Table 9.8)
typedef struct {
	uint8_t     bLength;
	uint8_t     bDescriptorType;
	uint16_t    bcdUSB;
	uint8_t     bDeviceClass;
	uint8_t     bDeviceSubClass;
	uint8_t     bDeviceProtocol;
	uint8_t     bMaxPacketSize0;
	uint16_t    idVendor;
	uint16_t    idProduct;
	uint16_t    bcdDevice;
	uint8_t     iManufacturer;
	uint8_t     iProduct;
	uint8_t     iSerialNumber;
	uint8_t     bNumConfigurations;
} Usb_DeviceDescriptor_t;

// Configuration Descriptor Header (USB 2.0 Spec Table 9.10)
typedef struct {
	uint8_t     bLength;
	uint8_t     bDescriptorType;
	uint16_t    wTotalLength;
	uint8_t     bNumInterfaces;
	uint8_t     bConfigurationValue;
	uint8_t     iConfiguration;
	uint8_t     bmAttributes;
	uint8_t     bMaxPower;
} Usb_ConfigurationDescriptorHeader_t;

// Interface Descriptor (USB 2.0 Spec Table 9.12)
typedef struct {
	uint8_t     bLength;
	uint8_t     bDescriptorType;
	uint8_t     bInterfaceNumber;
	uint8_t     bAlternateSetting;
	uint8_t     bNumEndpoints;
	uint8_t     bInterfaceClass;
	uint8_t     bInterfaceSubClass;
	uint8_t     bInterfaceProtocol;
	uint8_t     iInterface;
} Usb_InterfaceDescriptor_t;

// HID Descriptor (HID 1.11 Spec, Section 6.2.1)
typedef struct {
	uint8_t     bLength;
	uint8_t     bDescriptorType;
	uint16_t    bcdHID;
	uint8_t     bCountryCode;
	uint8_t     bNumDescriptors;
	uint8_t     bReportDescriptorType;
	uint16_t    wDescriptorLength;
} Usb_HidDescriptor_t;

// Endpoint Descriptor (USB 2.0 Spec Table 9.13)
typedef struct {
	uint8_t     bLength;
	uint8_t     bDescriptorType;
	uint8_t     bEndpointAddress;
	uint8_t     bmAttributes;
	uint16_t    wMaxPacketSize;
	uint8_t     bInterval;
} Usb_EndpointDescriptor_t;

// String Descriptor (USB 2.0 Spec Table 9.15)
typedef struct {
	uint8_t     bLength;
	uint8_t     bDescriptorType;
	wchar_t     bString[];
} Usb_StringDescriptor_t;

// HID Mouse Report (EP1.IN): 3 bytes to match HidReportDescriptor
typedef struct {
	uint8_t buttons;
	int8_t  x;
	int8_t  y;
} Usb_HidMouseReport_t;


//*****************************************************************************
//	HID Report - Mouse (Total report size = 3 bytes)
//	Buttons: bit0 = left, bit1 = right, bit2 = middle, bit3..bit7 = 0
//	Relative X delta, -127..127.  0 = no X movement
//	Relative Y delta, -127..127.  0 = no Y movement
//*****************************************************************************
const uint8_t HidReportDescriptor[] = {
	0x05, 0x01,       // Usage Page (Generic Desktop)
	0x09, 0x02,       // Usage (Mouse)
	0xA1, 0x01,       // Collection (Application)
	0x09, 0x01,       // Usage (Pointer)
		0xA1, 0x00,       // Collection (Physical)
			// Buttons (3 bits)
			0x05, 0x09,       //     Usage Page (Buttons)
			0x19, 0x01,       //     Usage Minimum (1)
			0x29, 0x03,       //     Usage Maximum (3)
			0x15, 0x00,       //     Logical Minimum (0)
			0x25, 0x01,       //     Logical Maximum (1)
			0x95, 0x03,       //     Report Count (3 fields)
			0x75, 0x01,       //     Report Size (1 bit)
			0x81, 0x02,       //     Input (Data, Var, Abs)
			// Padding (5 bits)
			0x95, 0x01,       //     Report Count (1 field)
			0x75, 0x05,       //     Report Size (5 bits)
			0x81, 0x03,       //     Input (Const, Var, Abs)
			// X/Y movement - 2 bytes
			0x05, 0x01,       //     Usage Page (Generic Desktop)
			0x09, 0x30,       //     Usage (X)
			0x09, 0x31,       //     Usage (Y)
			0x15, 0x81,       //     Logical Minimum (-127)
			0x25, 0x7F,       //     Logical Maximum (127)
			0x95, 0x02,       //     Report Count (2 fields)		
			0x75, 0x08,       //     Report Size (8 bits)
			0x81, 0x06,       //     Input (Data, Var, Rel)
		0xC0,             // End Collection
	0xC0              // End Collection
};


//*****************************************************************************
//	Configuration Descriptor Collection for the HID device
//	This is what the host receives in response to GET_DESCRIPTOR.
//*****************************************************************************
typedef struct {
	Usb_ConfigurationDescriptorHeader_t	Configuration;
	Usb_InterfaceDescriptor_t			Interface;
	Usb_HidDescriptor_t					Hid;
	Usb_EndpointDescriptor_t			HidReportInEndpoint;
} Usb_ConfigurationDescriptor_t;


//*****************************************************************************
//  Descriptor Initializers
//*****************************************************************************
// Device Descriptor
const Usb_DeviceDescriptor_t DeviceDescriptor = {
    .bLength            = sizeof(Usb_DeviceDescriptor_t),
    .bDescriptorType    = 0x01,         // Device Descriptor
    .bcdUSB             = 0x0200,       // USB 2.00
    .bDeviceClass       = 0x00,         // Defined in Interface
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = 64,           // Set to match USB_BUFSIZE_DEFAULT_BUF64
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0001,       // User defined
    .iManufacturer      = 1,			// Manufacturer String Descriptor
    .iProduct           = 2,			// Product String Descriptor
    .iSerialNumber      = 3,			// Serial Number String Descriptor
    .bNumConfigurations = 1
};

// Configuration Descriptor
const Usb_ConfigurationDescriptor_t ConfigurationDescriptor = {
    .Configuration = {
        .bLength            = sizeof(Usb_ConfigurationDescriptorHeader_t),
        .bDescriptorType    = 0x02,					// Configuration Descriptor
        .wTotalLength       = sizeof(Usb_ConfigurationDescriptor_t),
        .bNumInterfaces     = 1,					// Total number of Interfaces
        .bConfigurationValue= 1,					// The value the host sends in SET_CONFIGURATION
        .iConfiguration     = 0,					// No String Descriptor
        .bmAttributes       = USB_BUS_POWERED,
        .bMaxPower          = (100 / 2)				// 100mA in 2mA units
    },

    .Interface = {
        .bLength            = sizeof(Usb_InterfaceDescriptor_t),
        .bDescriptorType    = 0x04,					// Interface Descriptor
        .bInterfaceNumber   = 0,					// Interface number (zero based)
        .bAlternateSetting  = 0,					// Default
        .bNumEndpoints      = 1,					// EP1 IN only (EP0 not counted)
        .bInterfaceClass    = 0x03,					// HID
        .bInterfaceSubClass = 0x00,
        .bInterfaceProtocol = 0x00,
        .iInterface         = 0						// No String Descriptor
    },

    .Hid = {
        .bLength            = sizeof(Usb_HidDescriptor_t),
        .bDescriptorType    = 0x21,					// HID Descriptor
        .bcdHID             = 0x0110,				// HID 1.10
        .bCountryCode       = 0,
        .bNumDescriptors    = 1,
        .bReportDescriptorType = 0x22,				// Report
        .wDescriptorLength  = sizeof(HidReportDescriptor)
    },

    .HidReportInEndpoint = {
        .bLength            = sizeof(Usb_EndpointDescriptor_t),
        .bDescriptorType    = 0x05,					// Endpoint Descriptor
        .bEndpointAddress   = (USB_EP_DIR_IN | 1),	// EP1.IN
        .bmAttributes       = 0x03,					// Interrupt type
        .wMaxPacketSize     = 8,					// Set to match EP1.IN buffer size
        .bInterval          = 32					// 32ms (Windows maximum value)
    }
};

// String Index 0 Descriptor - Language
const Usb_StringDescriptor_t LanguageString = {
	.bLength = 4,
	.bDescriptorType = 0x03,						// String Descriptor
	.bString = { 0x0409 }							// US English
};

// String Index 1 Descriptor - Manufacturer
const Usb_StringDescriptor_t ManufacturerString = {
	.bLength = 2 + (sizeof(MANUFACTURER_STR) - 2),	// Subtract 2 string termination bytes
	.bDescriptorType	= 0x03,						// String Descriptor
	.bString			= MANUFACTURER_STR
};

// String Index 2 Descriptor - Product
const Usb_StringDescriptor_t ProductString = {
	.bLength = 2 + (sizeof(PRODUCT_STR) - 2),		// Subtract 2 string termination bytes
	.bDescriptorType	= 0x03,						// String Descriptor
	.bString			= PRODUCT_STR	
};

// String Index 3 Descriptor - Serial Number
const Usb_StringDescriptor_t SerialNumberString = {
	.bLength = 2 + (sizeof(SERIAL_NUMBER_STR) - 2),	// Subtract 2 string termination bytes
	.bDescriptorType	= 0x03,						// String Descriptor
	.bString			= SERIAL_NUMBER_STR
};

#endif
