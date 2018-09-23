/*
 * menu.c
 *
 * Created: 02.01.2018 2:37:14:PM
 *  Author: Petro
 */ 
 #define F_CPU 8000000
 #include <stdint.h>
 #include <avr/io.h>
 #include <util/delay.h>
 #include <avr/eeprom.h>
 #include "display.h"
 #include "ds1307.h"
 #include "I2c.h"
 #include "control.h"
 #include "ledLight.h"

  void MenuTime();
 //static void MenuBrigtness();
  void MenuLedAnimation();

 extern void RestoreSettingsFromEeprom(void);


 extern uint8_t EEMEM eepIsExist;
 extern uint8_t EEMEM eepBrightnessLedMax;
 extern uint8_t EEMEM eepBrightnessLedMin;
 extern uint8_t EEMEM eepBrightnessDigitMax;
 extern uint8_t EEMEM eepBrightnessDigitMin;
 extern uint8_t EEMEM eepNumLedAnimation;
 extern uint8_t EEMEM eepNumDigitAnimation;
 extern uint8_t EEMEM eepDisplayAnimation;
 extern uint8_t EEMEM eepColor[3];


 enum {DAY_MODE, NIGHT_MODE};
static uint8_t brigthessMode = DAY_MODE;

static control_result controlState;		//far save check control state
uint8_t dataToDisplay[3];				//buffer for display info 

uint8_t colorBacklightChoice[] = {255,0,0};
uint8_t digitBrightness[6], digitBlink[6];
uint8_t highDigitBrigtness, lowDigitBrightness;
uint8_t color[3];

uint8_t StartMenu(void)
{
	uint8_t mainMenu[] = {1,2,3,4,5,6};
	uint8_t selectMenu = 0;
	uint8_t pressedDelay = 0;
	const uint8_t SPEED_CHANGE_IN_PRESSED = 20;

	/*save led color, led brightness, digit brightness, number led animation, digit animation, display animation*/
	uint8_t i;
	LedReadColor(0, &color[0], &color[1], &color[2]);
	for(i=0;i<3;i++)
		eeprom_write_byte(&eepColor[i],color[i]);

	while(selectMenu != 5)		//select_menu = 6 - menu exit
	{
		selectMenu = 0;

		highDigitBrigtness = eeprom_read_byte(&eepBrightnessDigitMax);		//read current display brightness 
		LedSetBrigtness(highDigitBrigtness);
	
		lowDigitBrightness = highDigitBrigtness/5;
		digitBrightness[0] = highDigitBrigtness;
		digitBlink[0] = 1;
		for(i=1;i<NUM_DIGIT;i++)														//to lower other digit	
		{																		
			digitBrightness[i] = lowDigitBrightness;
			digitBlink[i] = 0;
		}
		DisplaySetBrightnessEachNumber100(digitBrightness);

		DisplaySetAnimation(SHORT_BLINC);
		DisplaySetData6Num(mainMenu);
		_delay_ms(100);
		DisplaySetBlinkDigit(digitBlink);

		LedOffAll();																	//enable led only "select menu" digit
		LedSetColor(selectMenu, colorBacklightChoice);
		for(i=4; i<6; i++)
			DisplayRequestUpdateLed();

		while((controlState=ControlCheck()) != PRESS_CENTER)
		{
			if(controlState == PRESS_LEFT || controlState == PRESSED_LEFT)
			{
				if(controlState == PRESS_LEFT && pressedDelay >SPEED_CHANGE_IN_PRESSED)
				{
					digitBrightness[selectMenu] = lowDigitBrightness;
					digitBlink[selectMenu] = 0;

					if(++selectMenu == NUM_DIGIT)
						selectMenu = 0;
					digitBrightness[selectMenu] = highDigitBrigtness;
					digitBlink[selectMenu] = 1;
					DisplaySetBrightnessEachNumber100(digitBrightness);
					DisplaySetBlinkDigit(digitBlink);

					LedOffAll();
					LedSetColor(selectMenu, colorBacklightChoice);
					DisplayRequestUpdateLed();
					pressedDelay = 0;
				}else
					pressedDelay++;
			}
			if (controlState == PRESS_RIGHT || controlState == PRESSED_RIGHT)
			{
				if(controlState == PRESS_RIGHT && pressedDelay >SPEED_CHANGE_IN_PRESSED)
				{
					digitBrightness[selectMenu]=lowDigitBrightness;
					digitBlink[selectMenu] = 0;
					if (--selectMenu>NUM_DIGIT)
						selectMenu = NUM_DIGIT-1;
					digitBrightness[selectMenu] = highDigitBrigtness;
					digitBlink[selectMenu] = 1;
					DisplaySetBrightnessEachNumber100(digitBrightness);
					DisplaySetBlinkDigit(digitBlink);

					LedOffAll();
					LedSetColor(selectMenu, colorBacklightChoice);
					DisplayRequestUpdateLed();
					pressedDelay = 0;
				}else
					pressedDelay ++;
			}

			_delay_ms(1);
		}

		digitBlink[selectMenu] = 0;			// off digit blink
		DisplaySetBlinkDigit(digitBlink);	//

		/*choice  menu*/
		switch(selectMenu)
		{
			case 0:		//hour, min, date, month, year
				MenuTime();
				break;
			case 2:		//brightness digit/led (day and night mode)	
				//MenuBrigtness();
				break;
			case 1:		//choice led animation ,digit animation
				MenuLedAnimation();
				break;
			case 3:	
				break;
			case 4:
				break;
			case 5:		//exit
				DisplayClear();
				break;
			default:
				break;
		}
	}

	/*restore all setting from eeprom*/
	/*restore led color, led brightness, digit brightness, number led animation, digit animation, display animation*/

	RestoreSettingsFromEeprom();
	//set brightness led and digit


	return 1;
}

void MenuTime()
{
	uint8_t i;
	uint8_t null;
	uint8_t pressedDelay = 0;
	uint8_t SPEED_CHANGE_IN_PRESSED = 150;

	enum {HOUR, MIN, DATE, MONTH, YEAR, EXIT};
	const uint8_t maxDataValue[5] = {23,59,31,12,99};		//HOUR, MIN, DATE, MONTH, YEAR
	const uint8_t minDataValue[5] = {0,0,1,1,0};			//HOUR, MIN, DATE, MONTH, YEAR
	uint8_t currentSettings = HOUR;

	DisplayClear();

	if(brigthessMode == DAY_MODE)
		highDigitBrigtness = eeprom_read_byte(&eepBrightnessDigitMax);		//read current display brightness
	else
		highDigitBrigtness = eeprom_read_byte(&eepBrightnessDigitMin);

	lowDigitBrightness = highDigitBrigtness/8;

	digitBrightness[0] = highDigitBrigtness;
	digitBrightness[1] = highDigitBrigtness;
	digitBlink[0] = 0;
	digitBlink[1] = 0;
	for(i=2;i<NUM_DIGIT;i++)
	{
		digitBrightness[i] = lowDigitBrightness;
		digitBlink[i] = 0;
	}
	DisplaySetAnimation(SHORT_BLINC);
	rtc_get_time(&dataToDisplay[0], &dataToDisplay[1], &dataToDisplay[2]);
	DisplaySetData3Num(dataToDisplay);
	DisplaySetBrightnessEachNumber100(digitBrightness);
	_delay_ms(100);
	DisplaySetBlinkDigit(digitBlink);

	LedOffAll();
	for(i=0; i<2; i++)
		LedSetColor(i, colorBacklightChoice);
	DisplayRequestUpdateLed();

	while(currentSettings != EXIT)
	{
		controlState = ControlCheck();

		/*update time*/
		if(rtc_check_sqw() && (currentSettings==HOUR || currentSettings==MIN))
		{
			rtc_get_time(&null, &null, &dataToDisplay[2]);
			DisplaySetData3Num(dataToDisplay);
		}

		if(controlState == PRESS_LEFT || controlState == PRESSED_LEFT)
		{
			if(controlState == PRESS_LEFT || pressedDelay >SPEED_CHANGE_IN_PRESSED)
			{
				if(currentSettings!=YEAR){
					if((++dataToDisplay[currentSettings%2]) > maxDataValue[currentSettings])
						dataToDisplay[currentSettings%2] = minDataValue[currentSettings];
				}
				else{
					if((++dataToDisplay[2])>maxDataValue[currentSettings])
						dataToDisplay[2] = minDataValue[currentSettings];
				}
				DisplaySetData3Num(dataToDisplay);
				pressedDelay = 0;
			}else
				pressedDelay++;
		}else 
		if(controlState == PRESS_RIGHT || controlState == PRESSED_RIGHT)
		{
			if(controlState == PRESS_RIGHT || pressedDelay >SPEED_CHANGE_IN_PRESSED)
			{
				if(currentSettings!=YEAR){
					if((--dataToDisplay[currentSettings%2]) > maxDataValue[currentSettings])
						dataToDisplay[currentSettings%2] = maxDataValue[currentSettings];
					if(currentSettings==DATE || currentSettings==MONTH)
						if(dataToDisplay[currentSettings%2]<minDataValue[currentSettings])
							dataToDisplay[currentSettings%2] = maxDataValue[currentSettings];
				}
				else{
					if((--dataToDisplay[2]) > maxDataValue[currentSettings])
						dataToDisplay[2] = maxDataValue[currentSettings];
				}
				DisplaySetData3Num(dataToDisplay);
				pressedDelay = 0;
			}else
				pressedDelay ++;
		}else
		if(controlState == PRESS_CENTER)
		{
				currentSettings++;

				/*save time*/
				if (currentSettings==DATE)
					rtc_set_time(dataToDisplay[0], dataToDisplay[1], dataToDisplay[2]);
				/*save date*/
				if(currentSettings == YEAR)
					rtc_set_date(dataToDisplay[0], dataToDisplay[1], dataToDisplay[2]);
				if(currentSettings == EXIT)
				{
					uint8_t tmpYear = dataToDisplay[2];
					rtc_get_date(&dataToDisplay[0], &dataToDisplay[1], &dataToDisplay[2]);
					rtc_set_date(dataToDisplay[0], dataToDisplay[1], tmpYear);
				}

				/*off time and display data*/
				if(currentSettings >=DATE)
				{
					if(currentSettings == DATE)
					rtc_get_date(&dataToDisplay[0], &dataToDisplay[1], &dataToDisplay[2]);
					if(currentSettings == YEAR)		//year in format __ 20 18
					{
						dataToDisplay[0] = OFF_NUMB;
						dataToDisplay[1] = 20;
					}
					DisplaySetData3Num(dataToDisplay);
				}

				/*set led backlighting and digit brightness*/
				LedOffAll();
				for(i=0;i<NUM_DIGIT;i++)						//set minimum brightness for all digit
				{
					digitBrightness[i] = lowDigitBrightness;
					//digitBlink[i] = 0;
				}
				if(currentSettings != YEAR)
				{
					LedSetColor((currentSettings%2)*2, colorBacklightChoice);
					LedSetColor((currentSettings%2)*2+1, colorBacklightChoice);
					digitBrightness[(currentSettings%2)*2] = highDigitBrigtness;
					digitBrightness[(currentSettings%2)*2+1] = highDigitBrigtness;
					//digitBlink[(currentSettings%2)*2] = 1;
					//digitBlink[(currentSettings%2)*2+1] = 1;
				}
				else
				{
					for (i=2;i<NUM_DIGIT;i++)
					{
						LedSetColor(i, colorBacklightChoice);
						digitBrightness[i] = highDigitBrigtness;
						//digitBlink[i] = 1;
					}
				}
				DisplayRequestUpdateLed();
				DisplaySetBrightnessEachNumber100(digitBrightness);
				DisplaySetBlinkDigit(digitBlink);
		}
		
		_delay_ms(1);
	}
}


// static void MenuBrigtness()
// {
// 	/*додати плавне моргання для узера, до першого прокручування енкодера*/
// 	uint8_t i;
// 	uint8_t pressedDelay = 0;
// 	uint8_t SPEED_CHANGE_IN_PRESSED = 100;
// 
// 	enum {BRIGHTNESS_DIGIT, BRIGHTNESS_LED, BRIGHTNESS_DIGIT_NIGHT, BRIGHTNESS_LED_NIGHT, EXIT};
// 	uint8_t currentSettings = BRIGHTNESS_DIGIT;
// 
// 	uint8_t tmpBrightness = eeprom_read_byte(&eepBrightnessDigitMax);
// 
// 	LedOffAll();
// 	LedSetBrigtness(eeprom_read_byte(&eepBrightnessLedMax));
// 	for(i=4; i<6; i++)
// 		LedSetColor(i, colorBacklightChoice);
// 	DisplayRequestUpdateLed();
// 
// 	dataToDisplay[0] = OFF_NUMB;
// 	dataToDisplay[1] = OFF_NUMB;
// 	dataToDisplay[2] = tmpBrightness;
// 	DisplaySetBrightness100(99);
// 	DisplaySetData3Num(dataToDisplay);
// 	
// 	while(currentSettings != EXIT)
// 	{
// 		controlState = ControlCheck();
// 		
// 		if(controlState == PRESS_LEFT || controlState == PRESSED_LEFT)
// 		{
// 			if(controlState == PRESS_LEFT || pressedDelay >SPEED_CHANGE_IN_PRESSED)
// 			{
// 				tmpBrightness++;
// 
// 				if(tmpBrightness >= 100)
// 					tmpBrightness = 0;
// 				dataToDisplay[2] = tmpBrightness;
// 				DisplaySetData3Num(dataToDisplay);
// 
// 				if(currentSettings == BRIGHTNESS_DIGIT || currentSettings == BRIGHTNESS_DIGIT_NIGHT)
// 				{
// 					DisplaySetBrightness100(tmpBrightness);
// 				}
// 				else if(currentSettings == BRIGHTNESS_LED || currentSettings == BRIGHTNESS_LED_NIGHT)
// 				{
// 					LedSetBrigtness(tmpBrightness);
// 					for(i=4; i<6; i++)
// 						LedSetColor(i, colorBacklightChoice);
// 					DisplayRequestUpdateLed();
// 				}
// 				pressedDelay = 0;
// 			}else
// 				pressedDelay ++;
// 		}else
// 		if(controlState == PRESS_RIGHT || controlState == PRESSED_RIGHT)
// 		{
// 			if(controlState == PRESS_RIGHT || pressedDelay >SPEED_CHANGE_IN_PRESSED)
// 			{
// 				tmpBrightness--;
// 				if(tmpBrightness >= 100)
// 					tmpBrightness = 99;
// 				dataToDisplay[2] = tmpBrightness;
// 				DisplaySetData3Num(dataToDisplay);
// 
// 				if(currentSettings == BRIGHTNESS_DIGIT || currentSettings == BRIGHTNESS_DIGIT_NIGHT)
// 					DisplaySetBrightness100(tmpBrightness);
// 				else if(currentSettings == BRIGHTNESS_LED || currentSettings == BRIGHTNESS_LED_NIGHT)
// 				{
// 					LedSetBrigtness(tmpBrightness);
// 					for(i=4; i<6; i++)
// 						LedSetColor(i, colorBacklightChoice);
// 					DisplayRequestUpdateLed();
// 				}
// 				pressedDelay = 0;
// 			}else
// 				pressedDelay ++;
// 		}else
// 		if(controlState == PRESS_CENTER)
// 		{
// 				if(currentSettings == BRIGHTNESS_DIGIT)
// 				{
// 					eeprom_write_byte(&eepBrightnessDigitMax, tmpBrightness);	//save brightness digit
// 					/*next setting - led brightness*/
// 					tmpBrightness = eeprom_read_byte(&eepBrightnessLedMax);
// 					//LedOffAll();
// 					for(i=4; i<6; i++)
// 						LedSetColor(i, colorBacklightChoice);
// 					DisplayRequestUpdateLed();
// 					
// 				}else if(currentSettings == BRIGHTNESS_LED)
// 				{
// 					eeprom_write_byte(&eepBrightnessLedMax, tmpBrightness);		//save brightness led
// 					/*next setting - digit brightness night*/
// 					tmpBrightness = eeprom_read_byte(&eepBrightnessDigitMin);
// 					DisplaySetBrightness100(tmpBrightness);
// 					LedSetBrigtness(eeprom_read_byte(&eepBrightnessLedMin));
// 					for(i=4; i<6; i++)
// 						LedSetColor(i, colorBacklightChoice);
// 					DisplayRequestUpdateLed();
// 				}
// 				else if(currentSettings == BRIGHTNESS_DIGIT_NIGHT)
// 				{
// 					eeprom_write_byte(&eepBrightnessDigitMin, tmpBrightness);		//save brightness digit night
// 					/*next setting - led brightness night*/
// 					tmpBrightness = eeprom_read_byte(&eepBrightnessLedMin);		//0-99 diapason
// 					//LedOffAll();
// 					LedSetBrigtness(tmpBrightness);
// 					for(i=4; i<6; i++)
// 						LedSetColor(i, colorBacklightChoice);
// 					DisplayRequestUpdateLed();
// 				}else if(currentSettings == BRIGHTNESS_LED_NIGHT)
// 				{
// 					eeprom_write_byte(&eepBrightnessLedMin, tmpBrightness);		//save brightness led night
// 				}
// 
// 				/*display new digit*/
// 				dataToDisplay[2] = tmpBrightness;
// 				DisplaySetData3Num(dataToDisplay);
// 
// 				currentSettings++;
// 		}
// 		_delay_ms(1);
// 	}
// }

void MenuLedAnimation()
{
	/**/
	uint8_t i;
	uint8_t animationUpdateCounter = 0;

	enum {INPUT, EXIT};

	uint8_t tmpAnimation = eeprom_read_byte(&eepNumLedAnimation);
	uint8_t currentSettings = 0;

	LedOffAll();
	LedSetBrigtness(eeprom_read_byte(&eepBrightnessLedMax));
	for(i=0; i<6; i++)
		LedSetColor(i, colorBacklightChoice);
	DisplayRequestUpdateLed();

	digitBlink[5] = 1;
	for(i=0;i<NUM_DIGIT-1;i++)														
		digitBlink[i] = 0;
	DisplaySetBlinkDigit(digitBlink);

	uint8_t numberToDisplay[6] = {OFF_NUMB,OFF_NUMB,OFF_NUMB,OFF_NUMB,OFF_NUMB,OFF_NUMB};
	/*денний чи нічний режим*/
	DisplaySetBrightness100(eeprom_read_byte(&eepBrightnessDigitMax));

	numberToDisplay[5] = tmpAnimation;
	DisplaySetData6Num(numberToDisplay);		//display current animation

	while(currentSettings != EXIT)
	{
		controlState = ControlCheck();

		/*display animation*/
		if(++animationUpdateCounter >= 50)	//every 10 ms
		{
			switch(tmpAnimation)
			{
				case 0:
					LedOffAll();
					break;
				case 1:
					LedTheaterChaseRainbow();
					break;
				case 2:
					LedRainbowCycle();
					break;
				case 3:
					LedRainbow();
					break;
				case 4:
					LedAllColorAnim(10, UP);
					break;
				default:
					break;
			}
			animationUpdateCounter = 0;
		}

		/*display number on display*/
		switch(controlState)
		{
			case PRESS_LEFT:
				tmpAnimation++;

				if(tmpAnimation == 5)
					tmpAnimation = 0;

				numberToDisplay[5] = tmpAnimation;
				DisplaySetData6Num(numberToDisplay);
				break;
			case PRESS_RIGHT:
				tmpAnimation--;

				if(tmpAnimation >= 5)
					tmpAnimation = 4;

				numberToDisplay[5] = tmpAnimation;
				DisplaySetData6Num(numberToDisplay);
				break;
			case PRESS_CENTER:
				currentSettings++;		//exit

				/*save settings*/
				eeprom_write_byte(&eepNumLedAnimation, tmpAnimation);
				break;
			default:
				/*if delay xx min - exit from menu*/
				break;
		}
		_delay_ms(1);
	}
}