//*****************************************************************************
//	Configuration header - AVR16DU14
//
//	Author: Richard
//	Date:	2026-03-10
//
//*****************************************************************************
#ifndef CONFIG_H
#define CONFIG_H

// Note: AVR64DU errata: F_CPU Max = 20MHz
// Must also set the correct CLKCTRL_FRQSEL_xxM_gc in main.c
#define F_CPU					24000000UL

#endif // CONFIG_H