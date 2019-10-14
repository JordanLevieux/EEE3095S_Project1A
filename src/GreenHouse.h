//Include statements
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h>
#include <stdlib.h>

//Method declarations
void initGPIO();
void syncTime();
int hexCompensation(int units);
int decCompensation(int units);
void intervalChange();
void resetSysTime();
void updateSysTime();
void dismissAlarm();
void toggleMonitoring();
void setupThread();

//Constants
const char RTCAddr = 0x6f;
const char SEC = 0x00;
const char MIN = 0x01;
const char HOUR = 0x02;
const int debounceTime = 200;
//Pins
const int BTNS[] = {,,,};
const int PWMpin = 1;
const int RTCAlarm =;//TODO

