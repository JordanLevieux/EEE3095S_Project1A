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

//Constants
Define BTNS[] = {,,,};
Define PWMpin 1

