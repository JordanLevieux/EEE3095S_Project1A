//Include statements
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h>
#include <stdlib.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <iostream>

//Method declarations
void initGPIO();
void syncTime();
int hexCompensation(int units);
int decCompensation(int units);
void intervalChange();
void resetSysTime();
void getSysTime(int rHour, int rMin, int rSec);
void dismissAlarm();
void toggleMonitoring();
void setupThread();
void *adcThread(void *threadargs);
void outputValues();
void triggerAlarm();
void setSysTime();

//Constants
const char RTCAddr = 0x6f;
const char SEC = 0x00;
const char MIN = 0x01;
const char HOUR = 0x02;
const int debounceTime = 400;
#define SPI_CHAN 0
#define SPI_SPEED 409600
//Pins
const int BTNS[] = {22,23,24,25};
const int PWMpin = 1;
const int RTCAlarm =7;

