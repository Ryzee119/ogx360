/*
This sketch is used for the ogx360 PCB (See https://github.com/Ryzee119/ogx360).
This program incorporates the USB Host Shield Library for the MAX3421 IC  See https://github.com/felis/USB_Host_Shield_2.0.
The USB Host Shield Library is an Arduino library, consequently I have imported to necessary Arduino libs into this project.

This program also incorporates the AVR LUFA USB Library, See http://www.fourwalledcubicle.com/LUFA.php for the USB HID support.

It supports up to four Xbox 360 controllers using the Xbox 360 Wireless Receiver including Rumble.

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

//#define HOST //Comment this line out to compile for Player 2, 3 and 4 slave boards.
//Uncomment to compile for Player 1 master board.

#ifdef HOST
#include <XBOXRECV.h>
#include <SPI.h>
#endif

#include "xboxcontroller.h"
#include "Wire.h"

extern uint8_t two_timeout_error;


#ifdef HOST
USB UsbHost;						//USB Host Controller USB class.
XBOXRECV Xbox360(&UsbHost);			//USB Host Controller Xbox 360 Receiver Class
uint32_t loopCount =0;				//Monitors the current loop
#else
uint8_t input_buffer[100];			//Input buffer used by slave devices
uint32_t wire_timeout=0;			//I2C comms timeout counter used for slave devices
uint8_t no_i2c = 0;
#endif
uint32_t startTime = 0;				//Stores a millsecond timestamp  at the start of each loop.
uint8_t  playerID;					//playerID is set in the main program based on the slot the Arduino is installed.


/*** Slave I2C Requests ***/
#ifndef HOST
//This function executes whenever a data request is sent from the I2C Master.
//The master only requests the actuator values from the slave.
void sendRumble(){
	Wire.write(&XboxOG[0].left_actuator,1); //when compiled for the slave devices the current state is stored in XboxOG[0]
	Wire.write(&XboxOG[0].right_actuator,1); //when compiled for the slave devices the current state is stored in XboxOG[0]	
}

//This function executes whenever data is sent from the I2C Master. The master only sends the button and
//axis state of the Xbox360 controllers
void getControllerData(int len){
	for (int i=0;i<len;i++){
		input_buffer[i]=Wire.read();
	}
	
	//If data is being received from the master,
	//it means that a wireless controller is synced. so set attach flag.
	USB_Attach(); 
	wire_timeout=millis(); //Update the timeout value
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
	
	//Init Serial Port
	Serial1.begin(500000);
	
	//Determine what player this board is. Used for the slave devices mainly.
	//There is 2 ID pins on the PCB which are read in.
	playerID = digitalRead(PLAYER_ID1_PIN)<<1 | digitalRead(PLAYER_ID2_PIN);
	Serial1.print(F("\r\nThis device is in player slot "));
	Serial1.print(playerID+1);
	
	
	
	
	#ifdef HOST
	//Init the XboxOG data arrays to zero.
	memset(&XboxOG,0x00,sizeof(USB_XboxGamepad_Data_t)*4);

	//Init Usb Host Controller
	digitalWrite(USB_HOST_RESET_PIN, LOW);
	delay(20);//wait 20ms to reset the IC. Reseting at startup improves reliability in my experience.
	digitalWrite(USB_HOST_RESET_PIN, HIGH);
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
		UsbHost.Task();
		if (Xbox360.XboxReceiverConnected) {
			for (uint8_t i = 0; i < 4; i++) {
				if (Xbox360.Xbox360Connected[i]) {
					
					//If Player 1 Wireless Controller is Synced, Enable the USB Peripheral for this Player
					//Only applicable for Player 1. i.e when i==0.
					//Slave devices will attach USB when they receive I2C commands. See getControllerData() function.
					if (i==0) {
						USB_Attach();
						if(enumerationComplete){
							digitalWrite(ARDUINO_LED_PIN, LOW);
							digitalWrite(PLAYER_LED_PIN, HIGH);  //This LED was included in the prototypes only. N/A anymore
						}
					}
					
					//Read Digital Buttons
					Xbox360.getButtonPress(UP, i)    ? XboxOG[i].digButtons |= DUP       : XboxOG[i].digButtons &= ~DUP;
					Xbox360.getButtonPress(DOWN, i)  ? XboxOG[i].digButtons |= DDOWN     : XboxOG[i].digButtons &= ~DDOWN;
					Xbox360.getButtonPress(LEFT, i)  ? XboxOG[i].digButtons |= DLEFT     : XboxOG[i].digButtons &= ~DLEFT;
					Xbox360.getButtonPress(RIGHT, i) ? XboxOG[i].digButtons |= DRIGHT    : XboxOG[i].digButtons &= ~DRIGHT;
					Xbox360.getButtonPress(START, i) ? XboxOG[i].digButtons |= START_BTN : XboxOG[i].digButtons &= ~START_BTN;
					Xbox360.getButtonPress(BACK, i)  ? XboxOG[i].digButtons |= BACK_BTN  : XboxOG[i].digButtons &= ~BACK_BTN;
					Xbox360.getButtonPress(L3, i)    ? XboxOG[i].digButtons |= LS_BTN    : XboxOG[i].digButtons &= ~LS_BTN;
					Xbox360.getButtonPress(R3, i)    ? XboxOG[i].digButtons |= RS_BTN    : XboxOG[i].digButtons &= ~RS_BTN;

					//Read Analog Buttons - have to be converted to digital because x360 controllers don't have analog buttons
					Xbox360.getButtonPress(A, i)     ? XboxOG[i].A = 0xFF              : XboxOG[i].A = 0x00;
					Xbox360.getButtonPress(B, i)     ? XboxOG[i].B = 0xFF              : XboxOG[i].B = 0x00;
					Xbox360.getButtonPress(X, i)     ? XboxOG[i].X = 0xFF              : XboxOG[i].X = 0x00;
					Xbox360.getButtonPress(Y, i)     ? XboxOG[i].Y = 0xFF              : XboxOG[i].Y = 0x00;
					Xbox360.getButtonPress(L1, i)    ? XboxOG[i].BLACK = 0xFF          : XboxOG[i].BLACK = 0x00;
					Xbox360.getButtonPress(R1, i)    ? XboxOG[i].WHITE = 0xFF          : XboxOG[i].WHITE = 0x00;

					//Read Analog triggers
					XboxOG[i].L = Xbox360.getButtonPress(L2, i); //0x00 to 0xFF
					XboxOG[i].R = Xbox360.getButtonPress(R2, i); //0x00 to 0xFF

					//Read Control Sticks
					XboxOG[i].leftStickX  = Xbox360.getAnalogHat(LeftHatX, i);
					XboxOG[i].leftStickY  = Xbox360.getAnalogHat(LeftHatY, i);
					XboxOG[i].rightStickX = Xbox360.getAnalogHat(RightHatX, i);
					XboxOG[i].rightStickY = Xbox360.getAnalogHat(RightHatY, i);
					
					
					//Send data to slave devices, and retrieve actuator/rumble values from slave devices.
					//Applicable to player 2, 3 and 4 only. i.e when i>0.
					if(i>0){
						
						//Send the button press data to the slave device. The last 11 bytes are not
						//used as part of the HID report so we dont need to send them
						Wire.beginTransmission(i);
						Wire.write((char*)&XboxOG[i],sizeof(USB_XboxGamepad_Data_t)-11); 
						Wire.endTransmission(true);

						if(Wire.requestFrom(i, (uint8_t)2)==2){
							int temp = Wire.read(); //read first 8 bytes - this is left actuator, returns -1 or 0xFF on error.
							if(temp!=-1 && XboxOG[i].left_actuator!=temp && temp!=0xFF){
								XboxOG[i].left_actuator=(uint8_t)temp;
								XboxOG[i].rumbleUpdate=1;
							}
							temp = Wire.read();    //read second 8 bytes - this is right actuator, returns -1 or 0xFF on error.
							if(temp!=-1 && XboxOG[i].right_actuator!=temp && temp!=0xFF){
								XboxOG[i].right_actuator=(uint8_t)temp;
								XboxOG[i].rumbleUpdate=1;
							}
							temp=0;
						} else {
							//just clear the buffer, must've been an error.
							Wire.flush();
						}
					}
					
					
					//Anything that sends a command to the Xbox 360 controllers happens here. (i.e rumble, LED changes, controller off command)
					//The Xbox 360 controller OUT endpoints have a pole rate of 8ms (8 USB Frames), and each loop is 4-5ms.
					//Consequently, loopCount%2 only enters every second loop so it should be about 8ms. 
					//If you send commands faster than 8 USB frames apart to the OUT endpoints all sorts of strange things happen.
					//All this code is in else if chains to ensure only one command is ever sent per loop per OUT endpoint.
					if(loopCount%2){
						//If you hold the XBOX button for more than ~1second, turn off controller
						if (Xbox360.getButtonPress(XBOX, i)) {
							if(XboxOG[i].xbox_holdtime==0){
								XboxOG[i].xbox_holdtime=millis();
							}
							if((millis()-XboxOG[i].xbox_holdtime)>1000 && (millis()-XboxOG[i].xbox_holdtime)<1100){
								XboxOG[i].digButtons = 0x00;
								XboxOG[i].left_actuator = 0x00;
								XboxOG[i].right_actuator = 0x00;
								XboxOG[i].xbox_holdtime=0;
								Xbox360.setRumbleOff(i);
								delay(10);
								Xbox360.disconnect(i);
							} else if ((millis()-XboxOG[i].xbox_holdtime)>1100){
								XboxOG[i].xbox_holdtime=0;
							}
							
						//If Xbox button isnt held down, perform the normal commands to the controller (rumbles, led changes:
						} else {
							XboxOG[i].xbox_holdtime=0; //Reset the XBOX button hold time counter.
							
							//Send actuator levels to the Xbox 360 controllers if new rumble values have been received.
							//If not, check the time since last rumble update and set rumbles back to zero if no updates have been
							//received after 250ms or so. Prevents the rumble getting locked on.
							//Else, finally just poll the controllers for status, battery and set the LEDs.
							if(XboxOG[i].rumbleUpdate==1){
								Xbox360.setRumbleOn(XboxOG[i].left_actuator, XboxOG[i].right_actuator, i);
								XboxOG[i].rumbleUpdate=0;
								XboxOG[i].rumbleTimer=millis();
							} else if (millis()-XboxOG[i].rumbleTimer>250 && (XboxOG[i].left_actuator!=0x00 || XboxOG[i].right_actuator!=0x00)){
								XboxOG[i].right_actuator=0x00;
								XboxOG[i].left_actuator=0x00;
								Xbox360.setRumbleOn(XboxOG[i].left_actuator, XboxOG[i].right_actuator, i);
							} else {
								static uint8_t statusToggle = 0;
								switch (statusToggle){
									//Happen every 100 loops or so.. ~ half a second and only needs to happen every so often.
									case 100:
									case 101:
									Xbox360.checkStatus(statusToggle);	
									statusToggle++; //Toggle between checking controller status and battery status. Both need to be checked regularly.
									break;
									case 102:
									Xbox360.setLedOn((LEDEnum)(i+1),i); //If nothing else needed to be sent, just make sure the right LED quadrant is on.
									statusToggle=0;
									break;
									default:
									statusToggle++;
									break;
								}
							}
						}
					}
				} else { //If Controller is not connected (Only applicable for Player 1)
					if (i==0) {
						digitalWrite(ARDUINO_LED_PIN, HIGH);
						digitalWrite(PLAYER_LED_PIN, LOW); //This LED was included in the prototypes only. N/A anymore
						USB_Detach();
					}
				}
			}
		}
		
		loopCount++;
		/***END USB HOST TASKS ***/
		#endif
		
		
		#ifndef HOST
		/*** TASKS SPECIFIC TO THE SLAVE DEVICES ***/
		//This area is specific to Player 2 and 3 and 4 when compiled as a slave device.
		//Input buffer is set by the getControllerData i2c event
		//Slave devices always use XboxOG[0] for their current controller state.
		//If no i2c command received after 1000ms, the msater isn't sending them likely because the x360 controller isn't synced.
		//If the x360 controller isn't synced we should use USB_Detach.
		//This makes the game pause etc. if you turn off the wireless controller.
		if(millis()-wire_timeout>1000){
			USB_Detach(); //Will detach the usb device from the Xbox console. The is the same as unplugging an original controller from the console.
			digitalWrite(PLAYER_LED_PIN, LOW);  //This LED was included in the prototypes only. N/A anymore
			if(!digitalRead(ARDUINO_LED_PIN)){
				Serial1.print(F("\r\nI2C timed out after 1000ms, disconnecting USB controller from OG Xbox"));	
			}
			digitalWrite(ARDUINO_LED_PIN, HIGH);
			Wire.flush();
			
		} else {
			//Copy the i2c input_buffer into the XboxOG array.
			memcpy(&XboxOG[0],input_buffer,sizeof(USB_XboxGamepad_Data_t)-11); //Last 11 bytes aren't part of the HID report.
			if(enumerationComplete){
				digitalWrite(ARDUINO_LED_PIN, LOW);
				digitalWrite(PLAYER_LED_PIN, HIGH);  //This LED was included in the prototypes only. N/A anymore
			}
		}
		
		/*** END TASKS SPECIFIC TO THE SLAVE DEVICES  ***/
		#endif
		
		
		/*** USB DEVICE TASKS ***/
		//HID Reports should be limited to every 4 USB Frames. I made it 5 to account for differences in timing accuracies.
		while (millis() - startTime < 5){
			USB_USBTask(); //Process any control transfers that happen to come through while we wait though.
		}
		startTime +=5;
		HID_Device_USBTask(&Xbox_HID_Interface); //Send OG Xbox HID Report
		
		/*** END USB DEVICE TASKS ***/
		
		
	}
}



