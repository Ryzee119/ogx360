/*
 * main.h
 *
 * Created: 9/02/2019 4:15:11 PM
 *  Author: Ryan
 */ 

#ifndef MAIN_H_
#define MAIN_H_
#include <inttypes.h>

#define USB_HOST_RESET_PIN 9
#define ARDUINO_LED_PIN 17
#define PLAYER_ID1_PIN 19
#define PLAYER_ID2_PIN 20


//Settings - Note the Atmega 32U4 has only 32KB of flash! The Arduino bootloader takes up about 15% of this.
//if the Program Memory Usage is >85% or so it may fail programming the device. You cannot enable all the below settings at once :(


/* Comment this line out to compile for Player 2, 3 and 4 slave boards. i.e '//#define MASTER' *///
#define MASTER

/* Define this to apply extra deadzone correction. Generally not required, but if needed will increase latency slightly and code size.
 * Heavily used, third-party/knock off controllers may need this. */
//#define APPLYDEADZONECORRECTION
#ifdef APPLYDEADZONECORRECTION
#define DEADZONE_INNER 0.1f //These are decimal percentages of the total range. i.e. 0.1=10%
#define DEADZONE_OUTER 0.05f
void applyDeadzone(float* pOutX, float* pOutY, float x, float y, const float deadZoneLow, const float deadZoneHigh);
#endif

#ifdef MASTER
/* Define this to add support for Steel Battalion Controller emulation with an Xbox 360 Wireless Controller Chatpad. (It wont work with any wired controllers) */
#define SUPPORTBATTALION

/* Define this to add support for Wired Xbox One Controllers. */
//#define SUPPORTWIREDXBOXONE

/* Define this to add support for Wired Xbox 360 Controllers. */
#define SUPPORTWIREDXBOX360

#endif


/* prototypes */
void sendControllerHIDReport();


#endif /* MAIN_H_ */