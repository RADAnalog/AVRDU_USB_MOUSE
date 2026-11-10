<img width="647" height="863" alt="image" src="https://github.com/user-attachments/assets/df815d88-afc7-4808-b8c4-9355c1ac3ea3" />

<img width="647" height="863" alt="image" src="https://github.com/user-attachments/assets/ca86377a-f66c-47fc-a986-1a95dfda9bc9" />

A compact version of the Mouse Shaker.  My prototype PCB assumed I could use PF6 to drive the "Activity" LED since the DU is not programmed (by default) to use this pin as /Reset.  However, PF6 can only be used as an input! I had to jumper PA0 to PF6 to drive the LED.  Always read the fine print.

A basic USB HID Mouse library for the Microchip AVRxxDU series of microcontrollers

The prototype was developed on the AVR16DU14 using Microchip Studio 7 (see notes below) and the Cynthion USB packet analyzer (from Great Scott Gadgets).

The purpose of developing this library was to learn how to implement a USB peripheral on the newer Series 2 AVR MCUs.  This implementation is for a "mouse shaker" - a device that enumerates as a mouse and keeps a Windows session from timing out (some organizations lock down Windows settings, so this is a hardware workaround :).

NOTE: 
-----
When the Host performs the first Get_Descriptor request for the 18 byte Device_Descriptor, the Host requests only the first 8 bytes of the Device_Descriptor and then issues a Bus Reset.  After receiving the 8 bytes, the Host does not request further packets and the device should consider this transaction complete.  However, a situation will occur if the USB interface is configured in "multipacket" mode AND the setup packet BUFSIZE = 8 or 16.  In this configuration, the USB interface will not assert TRNCOMPL (in multipacket mode, the USB interface waits until all bytes have been received by the host before asserting TRNCOMPL).  The simple fix is to ensure to use a BUFSIZE >= Device Descriptor size.  Just something to be aware of.

NOTE: Microchip Studio 7 and Atmel ICE - realizing that Microchip Studio 7 has been deprecated by Microchip!
-----
a) To use the newer devices with the ATMEL-ICE, the ICE firmware needs to be updated to at least Version 1.2e.  Microchip Studio 7 support ended some time (years) ago but I prefer using it over MPLABX.  However, Studio no longer keeps the firmware packs up to date through the Device Pack Manager.  I found I had to install MPLABX from Microchip, create an empty main.c and load the program to force MPLABX to update the firmware (there might be a way to manually load the firmware pack directly into Studio 7).

b) An annoying feature (to me at least) of Microchip Studio is that Studio continually inspects String Descriptors for HID devices after enumeration has completed.  You can disable this from happening while Studio is running if you set all of the descriptor String indices to 0.
