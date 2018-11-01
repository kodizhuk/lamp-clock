/*
 * clock.c
 *
 * Created: 26.06.2017 21:49:57
 * Author : kodizhuk
 */ 

#define F_CPU 8000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <stdint.h>

#include "display.h"
#include "ds1307.h"
#include "I2c.h"
#include "ledLight.h"
#include "menu.h"
#include "brightness_senor.h"
#include "control.h"
//#include "ds18x20.h"

#define bit_is_set(sfr,bit) \
	(_SFR_BYTE(sfr) & _BV(bit))

#define MAX_NUM_OF_TIMERS   5

/*time to display date*/
#define DELAY_TO_DATE		1000		// period 6s 
#define TIME_DISPLAY_DATE	20			//2s

/*task timer*/
uint8_t flag[MAX_NUM_OF_TIMERS];
enum {ACTION1, ACTION2, ACTION3, ACTION4};
volatile static struct
{
	uint8_t Number;
	uint16_t Time;
}SoftTimer[MAX_NUM_OF_TIMERS];

uint8_t dataToDisplay[3];

control_result controlState;

uint8_t numLedAnimation = 0;
uint8_t numDigitAnimation = 0;


uint8_t EEMEM eepIsExist;
uint8_t EEMEM eepBrightnessLedMax;
uint8_t EEMEM eepBrightnessLedMin;
uint8_t EEMEM eepBrightnessDigitMax;
uint8_t EEMEM eepBrightnessDigitMin;
uint8_t EEMEM eepNumLedAnimation;
uint8_t EEMEM eepNumDigitAnimation;
uint8_t EEMEM eepDisplayAnimation;
uint8_t EEMEM eepLedColor[3];

void Init(void);
void Loading(void);
void SetTaskTimer(uint8_t newNumber, uint16_t newTime);

void CheckUpdateTime();
void VerifyControl();
void DisplayOtherInfo();
void GotoMenu();

void RestoreSettingsFromEeprom();


/*task timer*/
ISR(TIMER1_OVF_vect)
{
	uint8_t i;
	for(i = 0; i < MAX_NUM_OF_TIMERS; i++)
	{
		if(SoftTimer[i].Number == 255)
			continue;     /*if timer empty, next timer*/
		
		if(SoftTimer[i].Time > 0)
		{
			SoftTimer[i].Time--;
		}
		else
		{
			/*set flag*/
			flag[SoftTimer[i].Number] = 1;
			
			/*reset timer*/
			SoftTimer[i].Number = 255;
		}
	}
	TCNT1 = 65527;
}

int main(void)
{
	Init();
	Loading();
	RestoreSettingsFromEeprom();

    while (1) 
    {
		if(flag[ACTION1])CheckUpdateTime();		//update time on display
		if(flag[ACTION2])VerifyControl();		//check encoder/button
		if(flag[ACTION3])DisplayOtherInfo();	//display temperature/...
		if(flag[ACTION4])GotoMenu();			//menu flag

		_delay_us(10);
	}
}

void Init(void)
{  
	/*task time initialization*/
	uint8_t i;
	for(i = 0; i < MAX_NUM_OF_TIMERS; i++)
		SoftTimer[i].Number = 255;
	TCCR1A = 0x00;
	TCCR1B |= (1 << CS12) | (1 << CS10);		//divider clk/1024
	TCNT1 = 65527;								//period 1ms
	TIMSK |= (1 << TOIE1);						//interrupt to timer overflow

	SetTaskTimer(ACTION1,10);
 	SetTaskTimer(ACTION2,10);			
 	SetTaskTimer(ACTION3,100);			//every 100ms
 	SetTaskTimer(ACTION4,100);			

	/*display initialization*/
	DisplayInit();
	
	/*RTC initialization*/
 	i2c_init();
 	rtc_init(0, 1, 0);
 	rtc_init_sqw();

 	ControlInit();
 	
 	LedInit();
 	
 	BrightnessInit();

	sei();
}

void Loading(void)
{
	uint8_t i;

	DisplayClear();
	DisplaySetData3Num(dataToDisplay);
	LedOffAll();
	LedUpdate();
	_delay_ms(300);
	for(i = 0; i < 6; i++)
	{
		LedSetColorRGB(i, 255, 20, 0);
		LedUpdate();
		_delay_ms(100);
	}	
}

void SetTaskTimer(uint8_t newNumber, uint16_t newTime)
{
	uint8_t i;
	/*search existing timer*/
	for(i = 0; i < MAX_NUM_OF_TIMERS; i++)
	{
		if(SoftTimer[i].Number == newNumber)
		{
			/*set new time*/
			SoftTimer[i].Time = newTime;
			return;
		}
	}
	
	/*search new empty timer*/
	for(i = 0; i < MAX_NUM_OF_TIMERS; i++)
	{
		if(SoftTimer[i].Number == 255)
		{
			SoftTimer[i].Number = newNumber;
			SoftTimer[i].Time = newTime;
			return;
		}
	}
}

void CheckUpdateTime()
{
	flag[ACTION1] = 0;
	
	static uint16_t countToDateDisplay;
	
	/*display date & update brightness every 1s*/
	if(++countToDateDisplay >= DELAY_TO_DATE)
	{
 		rtc_get_date(&dataToDisplay[0], &dataToDisplay[1], &dataToDisplay[2]);
 			
 		if(countToDateDisplay >= DELAY_TO_DATE + TIME_DISPLAY_DATE)	//delay to display date
 			countToDateDisplay = 0;
	}else if(rtc_check_sqw())
	{
		rtc_get_time(&dataToDisplay[0], &dataToDisplay[1], &dataToDisplay[2]);

		/*change brightness*/
 		uint16_t tmpBright;
 		tmpBright = BrightnessGet();
 		tmpBright *= 2;
 		if(tmpBright > 255)
 			tmpBright = 255;
 		if(tmpBright < 20)
 			tmpBright = 20;
 		LedSetBrigtness(tmpBright * 100 / 255);
// 		DisplaySetBrightness(255);
		
//   		dataToDisplay[0] = OFF_NUMB;
//   		dataToDisplay[1] = tmpBright/100;
//   		dataToDisplay[2] = tmpBright-dataToDisplay[1]*100;
	}

	DisplaySetData3Num(dataToDisplay);

	SetTaskTimer(ACTION1, 10);

}

void VerifyControl()
{
	flag[ACTION2] = 0;

	if(controlState == NO_PRESS)
		controlState = ControlCheck();

	/*ignore turn left/right*/
	if(controlState != PRESS_CENTER)
		controlState = NO_PRESS;
	
	SetTaskTimer(ACTION2, 10);
}

void DisplayOtherInfo()
{
	flag[ACTION3] = 0;

	/*при переході з години в дату можна перелічувати всі цифри, виключаючи їх справа наліво
	*	або просто затухання плавне години, а потім появлення плавне дати 
	*   або просто зсув вправо/вліво години та аналогічно появлення дати
	*/
	/*display animation*/
 	switch (eeprom_read_byte(&eepNumLedAnimation))
 	{
 		case 0:
 			LedOffAll();
			LedUpdate();
 			break;
 		case 1:
 			LedTheaterChaseRainbow();
			LedUpdate();
 			break;
 		case 2:
 			LedRainbowCycle();
			LedUpdate();
 			break;
 		case 3:
 			LedRainbow();
			LedUpdate();
 			break;
 		case 4:
 			LedAllColorAnim(10, UP);
			LedUpdate();
 			break;
		case 5:
			LedSetColorRGBAllLed(eeprom_read_byte(&eepLedColor[0]),
								eeprom_read_byte(&eepLedColor[1]),
								eeprom_read_byte(&eepLedColor[2]));
			LedUpdate();
			break;
 		default:
 			break;
 	} 

	SetTaskTimer(ACTION3, 100);
}

void GotoMenu()
{
	flag[ACTION4] = 0;

	/*go to menu*/
	if(controlState == PRESS_CENTER)
	{
		/*MENU*/
		StartMenu();
		controlState = NO_PRESS;
	}

	SetTaskTimer(ACTION4, 100);
}

void RestoreSettingsFromEeprom()
{
	uint8_t i;

	LedOffAll();
	LedUpdate();

	/*restore all settings from eeprom*/
	//eeprom_write_byte(&eepIsExist,0xFF);		//check default
	if(eeprom_read_byte(&eepIsExist) == 0xFF)
	{
		/*default settings*/
		for(i = 0; i < 3; i++)
			eeprom_write_byte(&eepLedColor[i], 0);
		eeprom_write_byte(&eepBrightnessLedMax, 99);
		eeprom_write_byte(&eepBrightnessLedMin, 5);
		eeprom_write_byte(&eepBrightnessDigitMax, 99);
		eeprom_write_byte(&eepBrightnessDigitMin, 10);
		eeprom_write_byte(&eepNumLedAnimation, 4);
		eeprom_write_byte(&eepNumDigitAnimation, 0);
		eeprom_write_byte(&eepDisplayAnimation, 1);		//0-short, 1 - long animation
		eeprom_write_byte(&eepIsExist, 0x00);
	}

	LedSetBrigtness(eeprom_read_byte(&eepBrightnessLedMax));
	DisplaySetBrightness100(eeprom_read_byte(&eepBrightnessDigitMax));
	numLedAnimation = eeprom_read_byte(&eepNumLedAnimation);
	if(numLedAnimation == 5)
	{
		LedSetColorRGBAllLed(eeprom_read_byte(&eepLedColor[0]), 
							eeprom_read_byte(&eepLedColor[1]),
							eeprom_read_byte(&eepLedColor[2]));
	}
	numDigitAnimation = eeprom_read_byte(&eepNumDigitAnimation);

	if(eeprom_read_byte(&eepDisplayAnimation))
		DisplaySetAnimation(LONG_BLINC);
	else
		DisplaySetAnimation(SHORT_BLINC);
}