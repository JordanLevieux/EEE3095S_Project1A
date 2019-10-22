
#include "GreenHouse.h"

int RTC;
int interval = 1;
int counter =0;

long alarmInterruptTime = 0;		//for alarm debounce
long lastInterruptTime = 0;			//for btn debounce
int dacAlarm = 1;						//default false
int monitoring = 1;					//default true

int sysHour, sysMin, sysSec;		//Time holding variables
int rtcHour, rtcMin, rtcSec;
int sHour, sMin, sSec;

float humidity = 0;
int temp=0;
int light=0;
float dacOut =0;
int main()
{
	initGPIO();
	
	setSysTime();
	
	setupThread();
    
	printf("RTC Time\tSys Time\tHumidity Temp\tLight\tDACout\tAlarm\n");
    while (1)
    {
		if(monitoring)
		{
    		delay(interval*1000);
			//outputValues();
		}
		
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
    pthread_attr_getschedparam (&tattr, &param); /* safe to get existing scheduling param */
    param.sched_priority = newprio; /* set the priority; others are unchanged */
    pthread_attr_setschedparam (&tattr, &param); /* setting the new scheduling param */
    pthread_create(&thread_id, &tattr, adcThread, (void *)1);
}

void *adcThread(void *threadargs)
{
	unsigned char buffer[3];					//Declare buffer array to RW to the ADC
	
	while (1)
	{
		buffer[0] = 1;
		buffer[1] = 0b10110000;								//read from chan0 & modify humidity's variable
		wiringPiSPIDataRW(SPI_CHAN, buffer, 3);
  		humidity = (((buffer[1]&3)<<8)+buffer[2]);
		humidity = (humidity/1023)*3.3;

		buffer[0] = 1;
		buffer[1] = 0b10010000;								//read from chan1 & modify humidity's variable
		wiringPiSPIDataRW(SPI_CHAN, buffer, 3);
		temp = (((buffer[1]&3)<<8)+buffer[2]);
		
		buffer[0] = 1;
		buffer[1] = 0b10100000;								//read from chan1 & modify humidity's variable
		wiringPiSPIDataRW(SPI_CHAN, buffer, 3);
		light = (((buffer[1]&3)<<8)+buffer[2]);

		dacOut = (light/1023.0)*humidity;
		
	}
	
}

void initGPIO()
{
	//set default pin scheme to wiringPi
	wiringPiSetup();
	
	//setup SPI
	wiringPiSPISetup(SPI_CHAN, SPI_SPEED);
	
	//setup RTC
	RTC = wiringPiI2CSetup(RTCAddr);
	wiringPiI2CWriteReg8(RTC, HOUR, 0x13);
    wiringPiI2CWriteReg8(RTC, MIN, 0x54);
    wiringPiI2CWriteReg8(RTC, SEC, 0x00);
    wiringPiI2CWriteReg8(RTC, SEC, 0b10000000);
	int setup = wiringPiI2CReadReg8(RTC, 0x07);
	setup = wiringPiI2CReadReg8(RTC, 0x07);
	setup |= 0b01000000;
	setup &= 0b11000000;
	wiringPiI2CWriteReg8(RTC, 0x07, setup);
	resetSysTime();
	
	//setup PWW for alarm
	pinMode(PWMpin, PWM_OUTPUT);
	
	//setup buttons b[0]-Interval change, b[1]-Reset Sys Time, b[2]-Dismiss alarm, b[3]-Toggle monitoring
	for(uint j=0; j < sizeof(BTNS)/sizeof(BTNS[0]); j++)
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
	printf("Change Interval:\n");

	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>debounceTime)
	{
		switch(interval)
		{
			case 1: interval = 1;
				interval = 2;
				break;
			case 2: interval = 2;
				interval = 5;
				break;
			default: interval = 1;
		}
	}
	lastInterruptTime = interruptTime;
}

void resetSysTime()
{
	long interruptTime = millis();
	if (interruptTime - lastInterruptTime>debounceTime)
	{
		printf("Reset System Time:\n");
		sysHour = hexCompensation(wiringPiI2CReadReg8(RTC,HOUR));
		sysMin = hexCompensation(wiringPiI2CReadReg8(RTC,MIN));
		sysSec = hexCompensation((wiringPiI2CReadReg8(RTC,SEC)&0b01111111));
	}
	lastInterruptTime = interruptTime;
}

void setSysTime()
{
   
        printf("Reset System Time:\n");
        sysHour = hexCompensation(wiringPiI2CReadReg8(RTC,HOUR));
        sysMin = hexCompensation(wiringPiI2CReadReg8(RTC,MIN));
        sysSec = hexCompensation((wiringPiI2CReadReg8(RTC,SEC)&0b01111111));
   
   
}

void getSysTime(int rHour, int rMin, int rSec)
{
	sHour = hexCompensation(rHour)-sysHour;
	sMin = hexCompensation(rMin)-sysMin;
	sSec = hexCompensation(rSec)-sysSec;
	if (sHour<0){sHour+=24;}
	if (sMin<0){sMin+=60;}
	if (sSec<0){sSec+=60;}
}

void dismissAlarm()
{
	long interruptTime = millis();
	if (interruptTime - lastInterruptTime>debounceTime)
	{
		dacAlarm = 0;
		printf("Dismiss Alarm:\n");
	}
	lastInterruptTime = interruptTime;
}

void toggleMonitoring()
{
	long interruptTime = millis();
	if (interruptTime - lastInterruptTime>debounceTime)
	{
		printf("Toggle Monitoring:\n");
		if (monitoring ==1){monitoring =0;}
		else {monitoring = 1;}
	}
	lastInterruptTime = interruptTime;
}

void triggerAlarm()
{
	long interruptTime = millis();
	if (interruptTime-alarmInterruptTime > 180000){dacAlarm = 1;}
}

void outputValues()
{
	if(monitoring)
	{
		if(counter<interval){counter++;}
		else
		{
			counter = 1;
			rtcHour = wiringPiI2CReadReg8(RTC, HOUR);
			rtcMin = wiringPiI2CReadReg8(RTC, MIN);
			rtcSec = wiringPiI2CReadReg8(RTC, SEC);
			rtcSec &= 0b01111111;
			getSysTime(rtcHour,rtcMin,rtcSec);
			if(dacOut<0.65||dacOut>2.65){triggerAlarm();}
			printf("%02x:%02x:%02x\t%02d:%02d:%02d\t%.1f\t%d\t%d\t%.1f\t%d\n",rtcHour,rtcMin,rtcSec,sHour,sMin,sSec,humidity, temp, light,dacOut,dacAlarm);
		}
	}
}

