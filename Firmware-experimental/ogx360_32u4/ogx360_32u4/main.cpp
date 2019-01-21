/*
This sketch is used for the ogx360 PCB (See https://github.com/Ryzee119/ogx360).
This program incorporates the USB Host Shield Library for the MAX3421 IC  See https://github.com/felis/USB_Host_Shield_2.0.
The USB Host Shield Library is an Arduino library, consequently I have imported to necessary Arduino libs into this project.

This program also incorporates the AVR LUFA USB Library, See http://www.fourwalledcubicle.com/LUFA.php for the USB HID support.

It supports up to four Xbox 360 controllers using the Xbox 360 Wireless Receiver including Rumble. This experimental build also
supports Xbox 360 Wired Controllers and Xbox One Wired Controllers via a USB HUB. Third party wired controllers for the Xbox is unknown
and untested.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

The code can be compiled for a SLAVE device or MASTER device by commenting out the #define HOST below.

The MASTER will communicate with the Xbox 360 Wireless Receiver, gather data for all four Xbox 360 controllers,
collate and then send the controller states to player 2,3 and 4 slave devices.
Player 1 controller data is converted by the MASTER itself and sent to the Original Xbox controller port.
The MASTER will also receive all the actuator/rumble values from the slave devices so that the values
can be sent to the Xbox 360 controllers accordingly.

The SLAVE will receive the Xbox 360 Controller data from the MASTER and also send the rumble values it
receives from the OG Xbox console to the MASTER. The data it receives from the MASTER is converted to
the correct USB HID report and sent to the OG Xbox Console.

Each device handles it's own USB comms. Respond the USB control transfers and generate the controller
HID reports to send to the OG Xbox via the controller port.

*/

#define HOST //Comment this line out to compile for Player 2, 3 and 4 slave boards.

#ifdef HOST
#include <XBOXRECV.h>
#include <XBOXUSB.h>
#include <XBOXONE.h>
#include <usbhub.h>
#endif

#include "xboxcontroller.h"
#include "Wire.h"


#ifdef HOST
//Setup all the USB supported classes.
USB UsbHost;										//USB Host Controller USB class.
USBHub  Hub1(&UsbHost);							//USB Hub support
XBOXRECV Xbox360Wireless(&UsbHost);			//USB Host Controller Xbox 360 Receiver Class

XBOXUSB Xbox360Wired1(&UsbHost);
XBOXUSB Xbox360Wired2(&UsbHost);
XBOXUSB Xbox360Wired3(&UsbHost);
XBOXUSB Xbox360Wired4(&UsbHost);
XBOXONE XboxOneWired1(&UsbHost);
XBOXONE XboxOneWired2(&UsbHost);
XBOXONE XboxOneWired3(&UsbHost);
XBOXONE XboxOneWired4(&UsbHost);
XBOXUSB *Xbox360Wired[4] = {&Xbox360Wired1, &Xbox360Wired2, &Xbox360Wired3, &Xbox360Wired4}; //An array of pointers to the Xbox 360 Wired controllers
XBOXONE *XboxOneWired[4] = {&XboxOneWired1, &XboxOneWired2, &XboxOneWired3, &XboxOneWired4}; //An array of pointers to the Xbox ONE Wired controllers


//Each type of controller must parse requests slightly differently. This is handled in these functions:

//Parse button presses for each type of controller
uint8_t getButtonPress(ButtonEnum b, uint8_t controller){
	if(Xbox360Wireless.Xbox360Connected[controller]){
		return Xbox360Wireless.getButtonPress(b, controller);
	} else if (Xbox360Wired[controller]->Xbox360Connected){
		return Xbox360Wired[controller]->getButtonPress(b);
	} else if (XboxOneWired[controller]->XboxOneConnected){
		if(b==L2 || b==R2){
			return (uint8_t)(XboxOneWired[controller]->getButtonPress(b)>>2); //Xbone one triggers are 10-bit, remove 2LSBs so its 8bit like OG Xbox
		} else {
			return (uint8_t)XboxOneWired[controller]->getButtonPress(b);
		}
	}
	return 0;
}

//Parse analog stick requests for each type of controller.
int16_t getAnalogHat(AnalogHatEnum a, uint8_t controller){
	if(Xbox360Wireless.Xbox360Connected[controller]){
		return Xbox360Wireless.getAnalogHat(a, controller);
	} else if (Xbox360Wired[controller]->Xbox360Connected){
		return Xbox360Wired[controller]->getAnalogHat(a);
	} else if (XboxOneWired[controller]->XboxOneConnected){
		return XboxOneWired[controller]->getAnalogHat(a);
	}
	return 0;
}

//Parse rumble activation requests for each type of controller.
void setRumbleOn(uint8_t lValue, uint8_t rValue, uint8_t controller){
	if(Xbox360Wireless.Xbox360Connected[controller]){
		Xbox360Wireless.setRumbleOn(lValue, rValue, controller);
	} else if (Xbox360Wired[controller]->Xbox360Connected){
		//As a safety measure I have limited rumble support to player 1  only when using wired.
		//This prevents excessive current being drawn from the single controller port on the console.
		//If you use an externally powered USB2.0 HUB to connect multiple wired controllers you can
		//re enable all four controller rumbles safely.
		if(controller==0){
			Xbox360Wired[controller]->setRumbleOn(lValue, rValue);	
		}
	} else if (XboxOneWired[controller]->XboxOneConnected){
		if(controller==0){
			XboxOneWired[controller]->setRumbleOn(lValue/8, rValue/8, lValue/2, rValue/2);
		}
	}
}

//Parse LED activation requests for each type of controller.
void setLedOn(LEDEnum led, uint8_t controller){
	if(Xbox360Wireless.Xbox360Connected[controller]){
		Xbox360Wireless.setLedOn(led,controller);
	} else if (Xbox360Wired[controller]->Xbox360Connected){
		 Xbox360Wired[controller]->setLedOn(led);
	} else if (XboxOneWired[controller]->XboxOneConnected){
		//no LEDs on Xbox One Controller. I think it is possible to adjust brightness but this is not implemented.
	}
}
#endif

#ifndef HOST
uint8_t input_buffer[100];			//Input buffer used by slave devices
#endif
uint8_t playerID;					//playerID is set in the main program based on the slot the Arduino is installed.


/*** Slave I2C Requests ***/
#ifndef HOST
//This function executes whenever a data request is sent from the I2C Master.
//The master only requests the actuator values from the slave.
void sendRumble(){
	Wire.write(&XboxOG[0].left_actuator,1);
	Wire.write(&XboxOG[0].right_actuator,1);
}

//This function executes whenever data is sent from the I2C Master.
//The master sends either the controller state if a wireless controller
//is synced or a disable packet {0xF0} if a controller is not synced.
void getControllerData(int len){
	for (int i=0;i<len;i++){
		input_buffer[i]=Wire.read();
	}
	//0xF0 is a packet sent from the master if the respective wireless controller isn't synced.
	if(input_buffer[0]==0xF0){
		USB_Detach();
		digitalWrite(ARDUINO_LED_PIN, HIGH);
	} else {
		USB_Attach();
		if(enumerationComplete)	digitalWrite(ARDUINO_LED_PIN, LOW);
		memcpy(&XboxOG[0],input_buffer,sizeof(USB_XboxGamepad_Data_t)-15); //Last 15 bytes aren't part of the HID report.
	}
}
#endif





int main(void)
{
	//Init the Arduino Library
	init();
	
	//Init IO
	pinMode(PLAYER_LED_PIN, OUTPUT);
	pinMode(USB_HOST_RESET_PIN, OUTPUT);
	pinMode(ARDUINO_LED_PIN, OUTPUT);
	pinMode(PLAYER_ID1_PIN, INPUT_PULLUP);
	pinMode(PLAYER_ID2_PIN, INPUT_PULLUP);
	digitalWrite(USB_HOST_RESET_PIN, LOW);
	digitalWrite(PLAYER_LED_PIN, LOW);
	digitalWrite(ARDUINO_LED_PIN, HIGH);
	
	//Init the LUFA USB Device Library
	SetupHardware();
	GlobalInterruptEnable();
	
	//Init Serial Port - Used for Debugging.
	Serial1.begin(500000);
	
	//Determine what player this board is. Used for the slave devices mainly.
	//There is 2 ID pins on the PCB which are read in.
	//00 = Player 1
	//01 = Player 2
	//10 = Player 3
	//11 = Player 4
	playerID = digitalRead(PLAYER_ID1_PIN)<<1 | digitalRead(PLAYER_ID2_PIN);
	Serial1.print(F("\r\nThis device is in player slot "));
	Serial1.print(playerID+1);
	
	
	//Init the XboxOG data arrays to zero.
	memset(&XboxOG,0x00,sizeof(USB_XboxGamepad_Data_t)*4);
	
	
	#ifdef HOST
	//Init Usb Host Controller
	delay(500); //Let everything settle.
	digitalWrite(USB_HOST_RESET_PIN, LOW);
	delay(20);//wait 20ms to reset the IC. Reseting at startup improves reliability in my experience.
	digitalWrite(USB_HOST_RESET_PIN, HIGH);
	delay(20); //Settle
	if (UsbHost.Init() == -1) {
		Serial1.print(F("\r\nOSC did not start"));
		while (1); //Something has gone wrong
	}
	Serial1.print(F("\r\nUSB Host Controller Initialised"));
	
	//Init I2C Master
	Wire.begin();
	Wire.setClock(400000);
	Serial1.print(F("\r\nThis is the host controller"));
	#endif
	
	#ifndef HOST
	//Init I2C Slave
	Wire.begin(playerID); //I2C Address is 0x01,0x02,0x03 for Player 2,3 and 4 respectively.
	Wire.setClock(400000);
	Wire.onRequest(sendRumble); //Register event for sendRumble. The host only requests actuator values.
	Wire.onReceive(getControllerData); //Register receive event for getting Xbox360 controller state data.
	Serial1.print(F("\r\nThis is a slave device"));
	#endif
	

	while (1){
		#ifdef HOST
		/*** USB HOST TASKS ***/
		UsbHost.busprobe();
		UsbHost.Task();
		
		for (uint8_t i = 0; i < 4; i++) {
			if (Xbox360Wireless.Xbox360Connected[i] || Xbox360Wired[i]->Xbox360Connected || XboxOneWired[i]->XboxOneConnected) {
				
				//Read Digital Buttons
				getButtonPress(UP, i)    ? XboxOG[i].digButtons |= DUP       : XboxOG[i].digButtons &= ~DUP;
				getButtonPress(DOWN, i)  ? XboxOG[i].digButtons |= DDOWN     : XboxOG[i].digButtons &= ~DDOWN;
				getButtonPress(LEFT, i)  ? XboxOG[i].digButtons |= DLEFT     : XboxOG[i].digButtons &= ~DLEFT;
				getButtonPress(RIGHT, i) ? XboxOG[i].digButtons |= DRIGHT    : XboxOG[i].digButtons &= ~DRIGHT;
				getButtonPress(START, i) ? XboxOG[i].digButtons |= START_BTN : XboxOG[i].digButtons &= ~START_BTN;
				getButtonPress(BACK, i)  ? XboxOG[i].digButtons |= BACK_BTN  : XboxOG[i].digButtons &= ~BACK_BTN;
				getButtonPress(L3, i)    ? XboxOG[i].digButtons |= LS_BTN    : XboxOG[i].digButtons &= ~LS_BTN;
				getButtonPress(R3, i)    ? XboxOG[i].digButtons |= RS_BTN    : XboxOG[i].digButtons &= ~RS_BTN;

				//Read Analog Buttons - have to be converted to digital because x360 controllers don't have analog buttons
				getButtonPress(A, i)     ? XboxOG[i].A = 0xFF                : XboxOG[i].A = 0x00;
				getButtonPress(B, i)     ? XboxOG[i].B = 0xFF                : XboxOG[i].B = 0x00;
				getButtonPress(X, i)     ? XboxOG[i].X = 0xFF                : XboxOG[i].X = 0x00;
				getButtonPress(Y, i)     ? XboxOG[i].Y = 0xFF                : XboxOG[i].Y = 0x00;
				getButtonPress(L1, i)    ? XboxOG[i].BLACK = 0xFF            : XboxOG[i].BLACK = 0x00;
				getButtonPress(R1, i)    ? XboxOG[i].WHITE = 0xFF            : XboxOG[i].WHITE = 0x00;

				//Read Analog triggers
				XboxOG[i].L = getButtonPress(L2, i); //0x00 to 0xFF
				XboxOG[i].R = getButtonPress(R2, i); //0x00 to 0xFF

				//Read Control Sticks
				XboxOG[i].leftStickX  = getAnalogHat(LeftHatX, i);
				XboxOG[i].leftStickY  = getAnalogHat(LeftHatY, i);
				XboxOG[i].rightStickX = getAnalogHat(RightHatX, i);
				XboxOG[i].rightStickY = getAnalogHat(RightHatY, i);

				//Anything that sends a command to the Xbox 360 controllers happens here. (i.e rumble, LED changes, controller off command)
				//The Xbox 360 controller OUT endpoints have a pole rate of 8ms (8 USB Frames), so should be limited to that.
				//If you send commands faster than 8 USB frames apart to the OUT endpoints all sorts of strange things happen.
				//All this code is in else if chains to ensure only one command is ever sent per loop per OUT endpoint.
				if(millis()-XboxOG[i].commandTimer>8){
					//If you hold the XBOX button for more than ~1second, turn off controller
					if (getButtonPress(XBOX, i)) {
						if(XboxOG[i].xbox_holdtime==0){
							XboxOG[i].xbox_holdtime=millis();
						}
						if((millis()-XboxOG[i].xbox_holdtime)>1000 && (millis()-XboxOG[i].xbox_holdtime)<1100){
							XboxOG[i].digButtons = 0x00;
							setRumbleOn(0, 0, i);
							delay(10);
							if(Xbox360Wireless.Xbox360Connected[i]) Xbox360Wireless.disconnect(i);
							XboxOG[i].commandTimer=millis();
						}
					//START+BACK TRIGGERS is a standard soft reset command. We turn off the rumble motors here to prevent them getting locked on
					//if you happen to press this reset combo mid rumble.
					} else if (getButtonPress(START, i) && getButtonPress(BACK, i) && XboxOG[i].L>0x00 && XboxOG[i].R>0x00) {
						setRumbleOn(0, 0, i);
						XboxOG[i].commandTimer=millis();
						XboxOG[i].rumbleUpdate=0;
					//If Xbox button isnt held down, perform the normal commands to the controller (rumbles, led changes etc)
					} else {
						XboxOG[i].xbox_holdtime=0; //Reset the XBOX button hold time counter.
						//Send actuator levels to the Xbox 360 controllers if new rumble values have been received.
						if(XboxOG[i].rumbleUpdate==1){
							setRumbleOn(XboxOG[i].left_actuator, XboxOG[i].right_actuator, i);
							XboxOG[i].rumbleUpdate=0;
							XboxOG[i].commandTimer=millis();
						} else {
							//These are low priority commands that only happen if nothing else was required. commandToggle increases once every 8 ms or so
							//so we can roughly time things here without just spamming unneeded commands to the controllers.
							static uint8_t commandToggle[4] = {0,0,0,0};
							switch (commandToggle[i]){
								//Happen every 250 loops ~ 2 seconds. A windows PC does this every 2.5 seconds or so.
								case 250:
								if(Xbox360Wireless.Xbox360Connected[i]) {
									Xbox360Wireless.checkStatus1(i); //Only applicable for Xbox360 Wireless
								}
								if(XboxOneWired[i]->XboxOneConnected) XboxOneWired[i]->enableInput();
								XboxOG[i].commandTimer=millis();
								commandToggle[i]++;
								break;
								case 251:
								if(Xbox360Wireless.Xbox360Connected[i]) {
									Xbox360Wireless.checkStatus2(i); //Only applicable for Xbox360 Wireless
								}
								XboxOG[i].commandTimer=millis();
								commandToggle[i]++;
								break;
								case 252:
								setLedOn((LEDEnum)(i+1),i); //If nothing else needed to be sent, just make sure the right LED quadrant is on.
								XboxOG[i].commandTimer=millis();
								commandToggle[i]=0;
								break;
								default:
								commandToggle[i]++;
								break;
							}
						}
					}
				} //end output commands
				
				
				//Send data to slave devices, and retrieve actuator/rumble values from slave devices.
				//Applicable to player 2, 3 and 4 only. i.e when i>0.
				if(i>0){
					//Send the button press data to the slave device. The last 15 bytes are not
					//used as part of the HID report so we dont need to send them
					Wire.beginTransmission(i);
					Wire.write((char*)&XboxOG[i],sizeof(USB_XboxGamepad_Data_t)-15);
					Wire.endTransmission(true);

					if(Wire.requestFrom(i, (uint8_t)2)==2){
						int temp = Wire.read(); //read first 8 bytes - this is left actuator, returns -1 or 0xFF on error.
						if(temp!=-1 && (uint8_t)temp!=0xFF && XboxOG[i].left_actuator!=(uint8_t)temp){
							XboxOG[i].left_actuator=(uint8_t)temp;
							XboxOG[i].rumbleUpdate=1;
						}
						temp = Wire.read();    //read second 8 bytes - this is right actuator, returns -1 or 0xFF on error.
						if(temp!=-1 && (uint8_t)temp!=0xFF && XboxOG[i].right_actuator!=(uint8_t)temp){
							XboxOG[i].right_actuator=(uint8_t)temp;
							XboxOG[i].rumbleUpdate=1;
						}
						temp=0;
					} else {
						//just clear the buffer, must've been an error.
						Wire.flush();
					}
				}
			} else {
				//If the respective controller isn't synced, we instead send a disablePacket over the i2c bus
				//so that the slave device knows to disable its USB. I've arbitrarily made this 0xF0.
				if(i>0){
					static uint8_t disablePacket[2] = {0xF0,0xF0};
					Wire.beginTransmission(i);
					Wire.write((char*)disablePacket,2);
					Wire.endTransmission(true);
				}
			}
			if(USB_Device_GetFrameNumber()-Xbox_HID_Interface.State.PrevFrameNum>=4){
				HID_Device_USBTask(&Xbox_HID_Interface); //Send OG Xbox HID Report
				USB_USBTask();
			}
		} //End for loop
		
		
		//Handle Player 1 controller connect/disconnect events.
		if (Xbox360Wireless.Xbox360Connected[0] || Xbox360Wired[0]->Xbox360Connected || XboxOneWired[0]->XboxOneConnected){
			USB_Attach();
			if(enumerationComplete){
				digitalWrite(ARDUINO_LED_PIN, LOW);
			}
		} else {
			digitalWrite(ARDUINO_LED_PIN, HIGH);
			USB_Detach();  //Disconnect from the OG Xbox port.
		}
		
		/***END USB HOST TASKS ***/
		#endif
		
		if(USB_Device_GetFrameNumber()-Xbox_HID_Interface.State.PrevFrameNum>=4){
			HID_Device_USBTask(&Xbox_HID_Interface); //Send OG Xbox HID Report
		}
		USB_USBTask();
		
	}
}


