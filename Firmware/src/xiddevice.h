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
#include <avr/wdt.h>
#include <avr/power.h>
#include <LUFAConfig.h>
#include <LUFA/Drivers/USB/USB.h>
#include "dukecontroller.h"
#include "steelbattalion.h"


#define DUKE_CONTROLLER 0
#define STEELBATTALION 1

/* Function Prototypes: */
#ifdef __cplusplus
extern "C" {
	#endif

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
	extern USB_ClassInfo_HID_Device_t DukeController_HID_Interface;
	extern USB_ClassInfo_HID_Device_t SteelBattalion_HID_Interface;
	extern USB_XboxGamepad_Data_t XboxOGDuke[4];
	#ifdef SUPPORTBATTALION
	extern USB_XboxSteelBattalion_Data_t XboxOGSteelBattalion;
	extern USB_XboxSteelBattalion_Feedback_t XboxOGSteelBattalionFeedback;
	#endif
	extern bool enumerationComplete;
	extern uint8_t playerID;
	#ifdef __cplusplus
}
#endif
#endif

