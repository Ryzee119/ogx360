/*
LUFA Library
Copyright (C) Dean Camera, 2013.

dean [at] fourwalledcubicle [dot] com
www.lufa-lib.org
*/

/*
Copyright 2013  Dean Camera (dean [at] fourwalledcubicle [dot] com)

Permission to use, copy, modify, distribute, and sell this
software and its documentation for any purpose is hereby granted
without fee, provided that the above copyright notice appear in
all copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of the author not be used in
advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

The author disclaims all warranties with regard to this
software, including all implied warranties of merchantability
and fitness.  In no event shall the author be liable for any
special, indirect or consequential damages or any damages
whatsoever resulting from loss of use, data or profits, whether
in an action of contract, negligence or other tortious action,
arising out of or in connection with the use or performance of
this software.
*/

/** \file
*
*  Header file for xboxcontroller.c.
*/

#ifndef _DUAL_VIRTUALSERIAL_H_
#define _DUAL_VIRTUALSERIAL_H_

/* Includes: */
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "LUFAConfig.h"

#include <LUFA/LUFA/Drivers/USB/USB.h>
#include <LUFA/LUFA/Platform/Platform.h>


#define DUP (1<<0)
#define DDOWN (1<<1)
#define DLEFT (1<<2)
#define DRIGHT (1<<3)
#define START_BTN (1<<4)
#define BACK_BTN (1<<5)
#define LS_BTN (1<<6)
#define RS_BTN (1<<7)

#define USB_HOST_RESET_PIN 9
#define PLAYER_LED_PIN 4
#define ARDUINO_LED_PIN 17
#define PLAYER_ID1_PIN 19
#define PLAYER_ID2_PIN 20
#define PLAYER1 0
#define PLAYER2 1
#define PLAYER3 2
#define PLAYER4 3




/* Function Prototypes: */
#ifdef __cplusplus
extern "C" {
	#endif

	typedef struct
	{
		uint8_t startByte; //Always 0x00
		uint8_t bLength;
		uint8_t digButtons;
		uint8_t reserved;
		uint8_t A;
		uint8_t B;
		uint8_t X;
		uint8_t Y;
		uint8_t BLACK;
		uint8_t WHITE;
		uint8_t L;
		uint8_t R;
		int16_t leftStickX;
		int16_t leftStickY;
		int16_t rightStickX;
		int16_t rightStickY;
		//These last few values aren't part of the xbox controller HID report, but are added here by me to store extra stuff.
		uint8_t left_actuator;
		uint8_t right_actuator;
		uint8_t rumbleUpdate;
		uint32_t rumbleTimer;
		uint32_t xbox_holdtime;
		uint32_t commandTimer;
	} USB_XboxGamepad_Data_t;

	uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
	const uint16_t wIndex,
	const void** const DescriptorAddress)
	ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);
	void SetupHardware(void);
	void EVENT_USB_Device_Connect(void);
	void EVENT_USB_Device_Disconnect(void);
	void EVENT_USB_Device_ConfigurationChanged(void);
	void EVENT_USB_Device_ControlRequest(void);
	void EVENT_USB_Device_StartOfFrame(void);
	bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
	uint8_t* const ReportID,
	const uint8_t ReportType,
	void* ReportData,
	uint16_t* const ReportSize);
	void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
	const uint8_t ReportID,
	const uint8_t ReportType,
	const void* ReportData,
	const uint16_t ReportSize);

	

	/* Data Types: */
	extern USB_ClassInfo_HID_Device_t Xbox_HID_Interface;
	extern USB_XboxGamepad_Data_t XboxOG[4];
	extern bool enumerationComplete;
	extern uint8_t playerID;
	#ifdef __cplusplus
}
#endif
#endif

