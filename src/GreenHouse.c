#include <GreenHouse.h>

int RTC;
unsigned char interval = 0b0000001;	//1s default output interval

long lastInterruptTime = 0;			//for btn debounce
int alarm = 0;						//default false
int monitoring = 1;					//default true

int sysHour, sysMin, sysSec;		//Time holding variables
int rtcHour, rtcMin, rtcSec;

double humidity = 0;
int temp=0;
int light=0;

int main()
{
	initGPIO();
	
	resetSysTime();
	
	setupThread();
    
    while (1)
    {
    	delay(1000);
    	printf("%f\t%d\t%d\n",humidity, temp, light);
    }
    
	return 0;
}

void setupThread()
{
	//black magic from Keegan to set up thread
	pthread_attr_t tattr;
    pthread_t thread_id;
    int newprio = 80;
    sched_param param;
    pthread_attr_init (&tattr);
    pthread_attr_getschedparam (&tattr, &param);
    param.sched_priority = newprio;
    pthread_attr_setschedparam (&tattr, &param);
    pthread_create(&thread_id, &tattr, adcThread, (void *)1);
}

void *adcThread(void *threadargs)
{
	unsigned char buffer[3];					//Declare buffer array to RW to the ADC
	
	while (1)
	{
		buffer[0] = 1;
		buffer[1] = 0b10000000;								//read from chan0 & modify humidity's variable
		wiringPiSPIDataRW(SPI_CHAN, buffer, 3);
		humidity = (((buffer[1]&3)<<8)+buffer[2])/2023*3.3;
		
		buffer[0] = 1;
		buffer[1] = 0b10010000;								//read from chan1 & modify humidity's variable
		wiringPiSPIDataRW(SPI_CHAN, buffer, 3);
		temp = (((buffer[1]&3)<<8)+buffer[2]);
		
		buffer[0] = 1;
		buffer[1] = 0b10100000;								//read from chan1 & modify humidity's variable
		wiringPiSPIDataRW(SPI_CHAN, buffer, 3);
		light = (((buffer[1]&3)<<8)+buffer[2]);
	}
	
}

void initGPIO()
{
	//set default pin scheme to wiringPi
	wiringPiSetup();
	
	//setup SPI
	wiringPiSPISetup(SPI_CHAN, SPI_SPEED);
	//comented out for the moment
	/*
	//Setup RTC
	int rtcSetup;
	RTC = wiringPiI2CSetup(RTCAddr);
	
	syncTime();
		//Setup alarm on RTC for timing
		pinMode(RTCAlarm, INPUT);
		pullUpDnControl(RTCAlarm, PUD_UP);
		
		//Activate Alarm 0 on the RTC
		rtcSetup = wiringPiI2CReadReg8(RTC, 0x0D);
		rtcSetup &= 0b00000111;						//TODO check that MFP supposed to go low
		//rtcSetup |= 0b10000000;
		wiringPiI2CWriteReg8(RTC, 0x0D, rtcSetup);
		
		//Set RTC to use Alarm with MFP
		wiringPiI2CWriteReg8(RTC, 0x0A, interval);
		rtcSetup = wiringPiI2CReadReg8(RTC, 0x07);
		rtcSetup |= 0b00010000;
		rtcSetup &= 0b10011111;
		wiringPiI2CWriteReg8(RTC, 0x07, rtcSetup);
	*/
	//setup PWW for alarm
	pinMode(PWMpin, PWM_OUTPUT);
	
	//setup buttons b[0]-Interval change, b[1]-Reset Sys Time, b[2]-Dismiss alarm, b[3]-Toggle monitoring
	for(int j=0; j < sizeof(BTNS)/sizeof(BTNS[0]); j++)
	{
		pinMode(BTNS[j], INPUT);
		pullUpDnControl(BTNS[j], PUD_UP);
	}
	
	//setup interupts
	wiringPiISR (BTNS[0], INT_EDGE_FALLING, intervalChange);
	wiringPiISR (BTNS[1], INT_EDGE_FALLING, resetSysTime);
	wiringPiISR (BTNS[2], INT_EDGE_FALLING, dismissAlarm);
	wiringPiISR (BTNS[3], INT_EDGE_FALLING, toggleMonitoring);
	wiringPiISR (RTCAlarm, INT_EDGE_FALLING, outputValues);
	
	
	
}

void syncTime()//sync the RTC with internet time
{
	int HH,MM,SS;
	HH = getHours();
	MM = getMins();
	SS = getSecs();

	HH = decCompensation(HH);
	wiringPiI2CWriteReg8(RTC, HOUR, HH);
	
	MM = decCompensation(MM);
	wiringPiI2CWriteReg8(RTC, MIN, MM);

	SS = decCompensation(SS);
	wiringPiI2CWriteReg8(RTC, SEC, 0b10000000+SS);
}

int decCompensation(int units)
{
	int unitsU = units%10;
	if (units >= 50){units = 0x50 + unitsU;}
	else if (units >= 40){units = 0x40 + unitsU;}
	else if (units >= 30){units = 0x30 + unitsU;}
	else if (units >= 20){units = 0x20 + unitsU;}
	else if (units >= 10){units = 0x10 + unitsU;}
	return units;
}

int hexCompensation(int units)
{
	int unitsU = units%0x10;
	if (units >= 0x50){units = 50 + unitsU;}
	else if (units >= 0x40){units = 40 + unitsU;}
	else if (units >= 0x30){units = 30 + unitsU;}
	else if (units >= 0x20){units = 20 + unitsU;}
	else if (units >= 0x10){units = 10 + unitsU;}
	return units;
}

void intervalChange()//change update interval between 1s, 2s &5s
{
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>debounceTime)
	{
		switch(interval)
		{
			case 1: interval = 0b0000002;
				wiringPiI2CWriteReg8(RTC, 0x0A, interval);
				break;
			case 2: interval = 0b0000101;
				wiringPiI2CWriteReg8(RTC, 0x0A, interval);
				break
			default: interval = 0b0000001;
				wiringPiI2CWriteReg8(RTC, 0x0A, interval);
		}
	}
	lastInterruptTime = interruptTime;
}

void resetSysTime()
{
	long interruptTime = millis();
	if (interruptTime - lastInterruptTime>debounceTime)
	{
		sysHour = 0;
		sysMin = 0;
		sysSec = 0;
	}
	lastInterruptTime = interruptTime;
}

void updateSysTime()
{
	//ToDo
}

void dismissAlarm()
{
	long interruptTime = millis();
	if (interruptTime - lastInterruptTime>debounceTime)
	{
		alarm = 0;
	}
	lastInterruptTime = interruptTime;
}

toggleMonitoring()
{
	long interruptTime = millis();
	if (interruptTime - lastInterruptTime>debounceTime)
	{
		monitoring = ~monitoring;
	}
	lastInterruptTime = interruptTime;
}


void outputValues()
{
	//Reset alarm flag
	int reset = wiringPiI2CReadReg8(RTC, 0x0D);	
	reset &= 0b11110111;					
	wiringPiI2CWriteReg8(RTC, 0x0D, reset);
	
	//Set alarm to trigger after next interval
	
	
	rtcHour = wiringPiI2CReadReg8(RTC, HOUR);
	rtcMin = wiringPiI2CReadReg8(RTC, MIN);
	rtcSec = wiringPiI2CReadReg8(RTC, SEC);
	rtcSec &= 0b01111111;
	
	
}
