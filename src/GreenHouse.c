#include <GreenHouse.h>

int RTC;
int interval = 1000;//1s default
long lastInterruptTime = 0;//for btn debounce
int alarm = 0;//default false
int monitoring = 1;//default true
int sysHour, sysMin, sysSec;
int rtcHour, rtcMin, rtcSec;

int main()
{
	initGPIO();
	
	resetSysTime();
	
	//black magic from Keegan to set up thread
	pthread_attr_t tattr;
    pthread_t thread_id;
    int newprio = 99;
    sched_param param;
    pthread_attr_init (&tattr);
    pthread_attr_getschedparam (&tattr, &param);
    param.sched_priority = newprio;
    pthread_attr_setschedparam (&tattr, &param);
    pthread_create(&thread_id, &tattr, adcThread, (void *)1);
    
    
	return 0;
}

void *adcThread(void *threadargs)//ToDo initialise
{
	//code to read from adc
}

void initGPIO()
{
	//set default pin scheme to wiringPi
	wiringPiSetup();
	
	//setup RTC
	int rtcSetup;
	RTC = wiringPiI2CSetup(RTCAddr);
	syncTime();
		//setup alarm on RTC for timing
		pinMode(RTCAlarm, INPUT);
		pullUpDnControl(RTCAlarm, PUD_UP);
		
		rtcSetup = ReadReg8(RTC, 0x0D);
		rtcSetup &= 0b10000111;
		rtcSetup |= 0b10000000;
		wiringPiI2CWriteReg8(RTC, 0x0D, rtcSetup);
		wiringPiI2CWriteReg8(RTC, 0x0A, 0bxxxxxx1);//no idea if this will work
		rtcSetup = ReadReg8(RTC, 0x07);
		rtcSetup |= 0b00010000;
		rtcSetup &= 0b10111111;
		wiringPiI2CWriteReg8(RTC, 0x07, rtcSetup);
		//This will probably have a lot of issues
	
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
	wiringPiISR (RTCAlarm, INT_EDGE_FALLING, //TODO);
	
	//setup SPI
	wiringPiSPISetup(SPI_CHAN, SPI_SPEED);
	
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
			case 1000: interval = 2000;
			break;
			case 2000: interval = 5000;
			break:
			default: interval = 1000;
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
