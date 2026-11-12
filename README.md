<img width="647" height="863" alt="image" src="https://github.com/user-attachments/assets/df815d88-afc7-4808-b8c4-9355c1ac3ea3" />

<img width="1280" height="678" alt="image" src="https://github.com/user-attachments/assets/235f8cf0-70cc-48ad-9982-cd88effe45bc" />

A compact version of the Mouse Shaker.  My prototype PCB assumed I could use PF6 to drive the "Activity" LED since the DU is not programmed (by default) to use this pin as /Reset.  However, PF6 can only be used as an input! I had to jumper PA0 to PF6 to drive the LED.  Always read the fine print.

Project Description
----
A basic USB HID Mouse library for the Microchip AVR_DU series of microcontrollers

Purpose
----
This library was developed to learn how to implement the USB peripheral on the newer AVR Series 2 MCUs (AVR_DU). The example application is a mouse shaker: a device that enumerates as a USB HID mouse and keeps a Windows session from timing out. Some organizations lock down Windows power and lock-screen settings, so this is a hardware workaround. 🙂

What It Does
----
The DU enumerates as a standard HID mouse and periodically reports “activity” without actually moving the cursor.

- At initialization, the DU starts a 20-second timer (TCA0).
- Every 20 seconds, the timer ISR sets a flag that is used to clear EP[1].STATUS.BUSNACK, which clears the NAK condition and arms the IN endpoint.
- On the next IN token, the host receives a report with the button byte and both (x, y) deltas set to zero, and the host acknowledges it.
- When the IN transaction completes, an activity LED flashes briefly.

Because the relative coordinates are always zero, the cursor never moves. On the Windows machines tested, a completed mouse IN transaction is enough to reset the Windows session idle timer.

NOTE: 
-----
When the Host performs the first Get_Descriptor request for the 18 byte Device_Descriptor, the Host requests only the first 8 bytes of the Device_Descriptor and then issues a Bus Reset.  After receiving the 8 bytes, the Host does not request further packets and the device should consider this transaction complete.  However, a situation will occur if the USB interface is configured in "multipacket" mode AND the setup packet BUFSIZE = 8 or 16.  In this configuration, the USB interface will not assert TRNCOMPL (in multipacket mode, the USB interface waits until all bytes have been received by the host before asserting TRNCOMPL).  The simple fix is to ensure to use a BUFSIZE >= Device Descriptor size.  Just something to be aware of.

NOTE: Microchip Studio 7 and Atmel ICE - realizing that Microchip Studio 7 has been deprecated by Microchip!
-----
a) To use the newer devices with the ATMEL-ICE, the ICE firmware needs to be updated to at least Version 1.2e.  Microchip Studio 7 support ended some time (years) ago but I prefer using it over MPLABX.  However, Studio no longer keeps the firmware packs up to date through the Device Pack Manager.  I found I had to install MPLABX from Microchip, create an empty main.c and load the program to force MPLABX to update the firmware (there might be a way to manually load the firmware pack directly into Studio 7).

b) An annoying feature (to me at least) of Microchip Studio is that Studio continually inspects String Descriptors for HID devices after enumeration has completed.  You can disable this from happening while Studio is running if you set all of the descriptor String indices to 0.
