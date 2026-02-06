// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _ESP8266MODBathroomLED_H_
#define _ESP8266MODBathroomLED_H_
#include "Arduino.h"
//add your includes for the project ESP8266MODBathroomLED here
#include "WiFiHelper.h"
#include "ServerHelper.h"
#include "Logger.h"
#include "secrets.h"

//end of add your includes here


//add your function definitions for the project ESP8266MODBathroomLED here
void setupGPIO();
void indexPage();
void handleNotFound();
void setupHTTPActions();
void setupAfterWiFiConnected();

void getRelayState();
void setRelayState();

void health();
void writeOk();
void writeJson(int httpStatus, const JsonDocument &doc);
void relayLoop(unsigned long);




//Do not add code below this line
#endif /* _ESP8266MODBathroomLED_H_ */
