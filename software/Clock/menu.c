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
 #include "i2c.h"
 #include "control.h"
 #include "ledLight.h"
 
 #define TIME_TO_AUTOEXIT	10000	//ms

 void MenuTime();
 void MenuLedStaticColor();
 void MenuLedAnimation();
 void MenuDigitAnimation();
 void MenuTubeTraning();

 extern void RestoreSettingsFromEeprom(void);


 extern uint8_t EEMEM eepIsExist;
 extern uint8_t EEMEM eepBrightnessLedMax;
 extern uint8_t EEMEM eepBrightnessLedMin;
 extern uint8_t EEMEM eepBrightnessDigitMax;
 extern uint8_t EEMEM eepBrightnessDigitMin;
 extern uint8_t EEMEM eepNumLedAnimation;
 extern uint8_t EEMEM eepNumDigitAnimation;
 extern uint8_t EEMEM eepDisplayAnimation;
 extern uint8_t EEMEM eepLedColor[3];
 extern uint8_t EEMEM eepTubeRecoveryOn;

 enum {DAY_MODE, NIGHT_MODE};
static uint8_t brigthessMode = DAY_MODE;

static control_result controlState;		//far save check control state
uint8_t dataToDisplay[3];				//buffer for display info 

uint8_t colorBacklightChoice[] = {255,0,0};
uint8_t digitBrightness[6], digitBlink[6];
uint8_t highDigitBrigtness, lowDigitBrightness;
uint8_t color[3];
uint16_t autoExitMs;

uint8_t StartMenu(void)
{
	uint8_t mainMenu[] = {1, 2, 3, 4, 5, 6};
	uint8_t selectMenu = 0;
	autoExitMs = 0;

	/*save led color, led brightness, digit brightness, number led animation, digit animation, display animation*/
	uint8_t i;
	LedReadColor(0, &color[0], &color[1], &color[2]);
	for(i = 0; i < 3; i++)
		eeprom_write_byte(&eepLedColor[i], color[i]);

	while(selectMenu != 5)		//select_menu = 6 - menu exit
	{
		selectMenu = 0;

		highDigitBrigtness = eeprom_read_byte(&eepBrightnessDigitMax);		//read current display brightness 
		LedSetBrigtness(highDigitBrigtness);
	
		lowDigitBrightness = highDigitBrigtness/5;
		digitBrightness[0] = highDigitBrigtness;
		digitBlink[0] = 1;
		for(i = 1; i < NUM_DIGIT; i++)														//to lower other digit	
		{																		
			digitBrightness[i] = lowDigitBrightness;
			digitBlink[i] = 0;
		}
		DisplaySetBrightnessEachNumber100(digitBrightness);

		DisplaySetAnimation(SHORT_BLINC);
		DisplaySetData6Num(mainMenu);
		_delay_ms(100);
		DisplaySetBlinkDigit(digitBlink);

		LedOffAll();
		//LedUpdate();																			
		LedSetColor(selectMenu, colorBacklightChoice);	//enable led only "select menu" digit
		DisplayRequestUpdateLed();

		while(((controlState = ControlCheck()) != PRESS_CENTER) && autoExitMs <= TIME_TO_AUTOEXIT)
		{
			if(controlState == PRESS_R || controlState == PRESSED_R)
			{
				autoExitMs = 0;
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
				_delay_ms(100);
			}
			if (controlState == PRESS_L || controlState == PRESSED_L)
			{
				autoExitMs = 0;
				digitBrightness[selectMenu] = lowDigitBrightness;
				digitBlink[selectMenu] = 0;
				if (--selectMenu > NUM_DIGIT)
					selectMenu = NUM_DIGIT - 1;
				digitBrightness[selectMenu] = highDigitBrigtness;
				digitBlink[selectMenu] = 1;
				DisplaySetBrightnessEachNumber100(digitBrightness);
				DisplaySetBlinkDigit(digitBlink);

				LedOffAll();
				LedSetColor(selectMenu, colorBacklightChoice);
				DisplayRequestUpdateLed();
				_delay_ms(100);
			}
			
			_delay_ms(1);
			autoExitMs++;
		}
		
		digitBlink[selectMenu] = 0;			// off digit blink
		DisplaySetBlinkDigit(digitBlink);	
		
		if(autoExitMs >= TIME_TO_AUTOEXIT)
			selectMenu = 5;

		/*choice  menu*/
		switch(selectMenu)
		{
			case 0:		//hour, min, date, month, year
				MenuTime();
				selectMenu = 5;
				controlState = PRESS_CENTER;
				break;
			case 1:		//choice led animation ,digit animation
				MenuLedAnimation();
				selectMenu = 5;
				controlState = PRESS_CENTER;
				break;
			case 2:		//brightness digit/led (day and night mode)	
				MenuLedStaticColor();
				selectMenu = 5;
				controlState = PRESS_CENTER;
				break;
			case 3:	
				MenuDigitAnimation();
				selectMenu = 5;
				controlState = PRESS_CENTER;
				break;
			case 4:
				MenuTubeTraning();
				selectMenu = 5;
				controlState = PRESS_CENTER;
				break;
			case 5:		//exit
				DisplayClear();
				break;
			default:
				break;
		}
	}

	RestoreSettingsFromEeprom();

	return 1;
}

void MenuTime()
{
	uint8_t i;
	uint8_t null;
	uint8_t pressedDelay = 0;
	uint8_t SPEED_CHANGE_IN_PRESSED = 150;
	autoExitMs = 0;

	enum {HOUR, MIN, DATE, MONTH, YEAR, EXIT};
	const uint8_t maxDataValue[5] = {23, 59, 31, 12, 99};		//HOUR, MIN, DATE, MONTH, YEAR
	const uint8_t minDataValue[5] = {0, 0, 1, 1, 0};			//HOUR, MIN, DATE, MONTH, YEAR
	uint8_t currentSettings = HOUR;

	DisplayClear();

	if(brigthessMode == DAY_MODE)
		highDigitBrigtness = eeprom_read_byte(&eepBrightnessDigitMax);		//read current display brightness
	else
		highDigitBrigtness = eeprom_read_byte(&eepBrightnessDigitMin);

	lowDigitBrightness = highDigitBrigtness / 8;

	digitBrightness[0] = highDigitBrigtness;
	digitBrightness[1] = highDigitBrigtness;
	digitBlink[0] = 0;
	digitBlink[1] = 0;
	for(i = 2; i < NUM_DIGIT; i++)
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
	//LedUpdate();
	for(i = 0; i < 2; i++)
		LedSetColor(i, colorBacklightChoice);
	DisplayRequestUpdateLed();

	while(currentSettings != EXIT && autoExitMs <= TIME_TO_AUTOEXIT)
	{
		controlState = ControlCheck();

		/*update time*/
		if(rtc_check_sqw() && (currentSettings == HOUR || currentSettings == MIN))
		{
			rtc_get_time(&null, &null, &dataToDisplay[2]);
			DisplaySetData3Num(dataToDisplay);
		}

		if(controlState == PRESS_R || controlState == PRESSED_R)
		{
			autoExitMs = 0;
			if(controlState == PRESS_R || pressedDelay > SPEED_CHANGE_IN_PRESSED)
			{
				if(currentSettings!=YEAR)
				{
					if((++dataToDisplay[currentSettings%2]) > maxDataValue[currentSettings])
						dataToDisplay[currentSettings%2] = minDataValue[currentSettings];
				}
				else{
					if((++dataToDisplay[2]) > maxDataValue[currentSettings])
						dataToDisplay[2] = minDataValue[currentSettings];
				}
				DisplaySetData3Num(dataToDisplay);
				pressedDelay = 0;
			}else
				pressedDelay++;
		}else 
		if(controlState == PRESS_L || controlState == PRESSED_L)
		{
			autoExitMs = 0;
			if(controlState == PRESS_L || pressedDelay > SPEED_CHANGE_IN_PRESSED)
			{
				if(currentSettings!=YEAR){
					if((--dataToDisplay[currentSettings%2]) > maxDataValue[currentSettings])
						dataToDisplay[currentSettings%2] = maxDataValue[currentSettings];
					if(currentSettings == DATE || currentSettings == MONTH)
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
			autoExitMs = 0;
			currentSettings++;

			/*save time*/
			if (currentSettings == DATE)
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
			if(currentSettings >= DATE)
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
			//LedUpdate();
			DisplayRequestUpdateLed();
			for(i = 0; i < NUM_DIGIT; i++)						//set minimum brightness for all digit
			{
				digitBrightness[i] = lowDigitBrightness;
			}
			if(currentSettings != YEAR)
			{
				LedSetColor((currentSettings%2)*2, colorBacklightChoice);
				LedSetColor((currentSettings%2)*2+1, colorBacklightChoice);
				digitBrightness[(currentSettings%2)*2] = highDigitBrigtness;
				digitBrightness[(currentSettings%2)*2+1] = highDigitBrigtness;
			}
			else
			{
				for (i = 2; i < NUM_DIGIT; i++)
				{
					LedSetColor(i, colorBacklightChoice);
					digitBrightness[i] = highDigitBrigtness;
				}
			}
			DisplayRequestUpdateLed();
			DisplaySetBrightnessEachNumber100(digitBrightness);
			DisplaySetBlinkDigit(digitBlink);
		}
		
		_delay_ms(1);
		autoExitMs++;
	}
}

 void MenuLedStaticColor()
 {
	autoExitMs = 0;
	controlState = NO_PRESS;
	uint8_t r, g, b;
	DisplayClear();

	LedSetBrigtness(LED_MAX_BRIGHTNESS);

	LedOffAll();
	//LedUpdate();
	dataToDisplay[0] = 0;
	dataToDisplay[1] = 0;
	dataToDisplay[2] = 0;
	DisplaySetData3Num(dataToDisplay);
	DisplaySetBrightness100(100);
	
	//set first color
	LedAllColorAnim(20, DOWN);
	//LedUpdate();
	DisplayRequestUpdateLed();

	while(controlState != PRESS_CENTER && autoExitMs <= TIME_TO_AUTOEXIT)
	{
		controlState = ControlCheck();

		/*display number on display*/
		if (controlState == PRESS_R || controlState == PRESSED_R)
		{
			autoExitMs = 0;
			LedAllColorAnim(20, UP);
			//LedUpdate();
			DisplayRequestUpdateLed();
		}
		else if(controlState == PRESS_L || controlState == PRESSED_L)
		{
			autoExitMs = 0;
			LedAllColorAnim(20, DOWN);
			//LedUpdate();
			DisplayRequestUpdateLed();
		}
		else if (controlState == PRESS_CENTER)
		{
			LedReadColor(0, &r, &g, &b);
			eeprom_write_byte(&eepNumLedAnimation, 5);	//static color led animation
			eeprom_write_byte(&eepLedColor[0], r);
			eeprom_write_byte(&eepLedColor[1], g);
			eeprom_write_byte(&eepLedColor[2], b);
		}

		_delay_ms(100);
		autoExitMs += 100;
	}
 }

void MenuLedAnimation()
{
	autoExitMs = 0;
	uint8_t i;
	uint8_t animationUpdateCounter = 0;

	enum {INPUT, EXIT};

	uint8_t tmpAnimation = 4;			//default animation
	uint8_t currentSettings = INPUT;

	LedOffAll();
	//LedUpdate();
	DisplayRequestUpdateLed();
	LedSetBrigtness(LED_MAX_BRIGHTNESS);

	digitBlink[5] = 1;
	for(i = 0; i < NUM_DIGIT-1; i++)														
		digitBlink[i] = 0;
	DisplaySetBlinkDigit(digitBlink);

	dataToDisplay[0] = OFF_NUMB;
	dataToDisplay[1] = OFF_NUMB;
	dataToDisplay[2] = tmpAnimation;
	DisplaySetBrightness100(100);
	DisplaySetData3Num(dataToDisplay);		//display current animation

	while(currentSettings != EXIT && autoExitMs <= TIME_TO_AUTOEXIT)
	{
		controlState = ControlCheck();

		/*display animation*/
		if(++animationUpdateCounter >= 50)	//every 10 ms
		{
			switch(tmpAnimation)
			{
				case 0:
					LedOffAll();
					//LedUpdate();
					DisplayRequestUpdateLed();
					break;
				case 1:
					LedTheaterChaseRainbow();
					//LedUpdate();
					DisplayRequestUpdateLed();
					break;
				case 2:
					LedRainbowCycle();
					//LedUpdate();
					DisplayRequestUpdateLed();
					break;
				case 3:
					LedRainbow();
					//LedUpdate();
					DisplayRequestUpdateLed();
					break;
				case 4:
					LedAllColorAnim(10, UP);
					//LedUpdate();
					DisplayRequestUpdateLed();
					break;
				default:
					break;
			}
			animationUpdateCounter = 0;
		}

		/*display number on display*/
		switch(controlState)
		{
			case PRESS_R:
				autoExitMs = 0;
				tmpAnimation++;

				if(tmpAnimation == 5)
					tmpAnimation = 0;

				dataToDisplay[2] = tmpAnimation;
				DisplaySetData3Num(dataToDisplay);
				break;
			case PRESS_L:
				autoExitMs = 0;
				tmpAnimation--;

				if(tmpAnimation >= 5)
					tmpAnimation = 4;

				dataToDisplay[2] = tmpAnimation;
				DisplaySetData3Num(dataToDisplay);
				break;
			case PRESS_CENTER:
				currentSettings = EXIT;		//exit

				/*save settings*/
				eeprom_write_byte(&eepNumLedAnimation, tmpAnimation);
				break;
			default:
				break;
		}
		_delay_ms(1);
		autoExitMs++;
	}
}

void MenuDigitAnimation()
{
	autoExitMs = 0;
	uint8_t secondCounter = 0;
	controlState = NO_PRESS;
	DisplayClear();
	LedOffAll();
	//LedUpdate();
	DisplayRequestUpdateLed();
	 
	dataToDisplay[0] = 0;
	dataToDisplay[1] = 0;
	dataToDisplay[2] = 0;
	DisplaySetData3Num(dataToDisplay);
	DisplaySetBrightness100(100);
	 
	uint8_t digitAnumation = eeprom_read_byte(&eepDisplayAnimation) ? 0 : 1;
	 
	DisplaySetAnimation(digitAnumation);
	 
	while(controlState != PRESS_CENTER && autoExitMs <= TIME_TO_AUTOEXIT)
	{
		controlState = ControlCheck();

		/*display number on display*/
		if (controlState == PRESS_R || controlState == PRESSED_R || controlState == PRESS_L || controlState == PRESSED_L)
		{
			digitAnumation = digitAnumation ? 0 : 1;
			DisplaySetAnimation(digitAnumation);
			autoExitMs = 0;
		}else if (controlState == PRESS_CENTER)
		{
			eeprom_write_byte(&eepDisplayAnimation, digitAnumation);	//static color led animation
		}

		if(++secondCounter >= 10)
		{
			secondCounter = 0;
			if(++dataToDisplay[2] >= 60)
				dataToDisplay[2] = 0;
			DisplaySetData3Num(dataToDisplay);
			
		}
		autoExitMs+=100;
		_delay_ms(100);
	}
}

void MenuTubeTraning()
{
	uint8_t secondCounter = 0;
	controlState = NO_PRESS;
	DisplayClear();
	DisplaySetBrightness100(100);
	dataToDisplay[0] = 0;
	dataToDisplay[1] = 0;
	dataToDisplay[2] = 0;
	DisplaySetData3Num(dataToDisplay);
	DisplaySetAnimation(eeprom_read_byte(&eepDisplayAnimation));
	uint8_t tubeTraningOn = eeprom_read_byte(&eepTubeRecoveryOn);
	if(tubeTraningOn)
		LedSetColorRGBAllLed(0,255,0);
	else
		LedSetColorRGBAllLed(255,0,0);
	DisplayRequestUpdateLed();
	

	while(controlState != PRESS_CENTER && autoExitMs <= TIME_TO_AUTOEXIT)
	{
 		controlState = ControlCheck();

	  	if (controlState == PRESS_R || controlState == PRESSED_R || controlState == PRESS_L || controlState == PRESSED_L)
	  	{
	 	 	tubeTraningOn = tubeTraningOn ? 0 : 1;		//invert data
			if(tubeTraningOn)
				LedSetColorRGBAllLed(0,255,0);
			else
				LedSetColorRGBAllLed(255,0,0);			
			DisplayRequestUpdateLed();	
			autoExitMs = 0;	
	  	}
	 	else if (controlState == PRESS_CENTER)
	 	{
	 		eeprom_write_byte(&eepTubeRecoveryOn, tubeTraningOn);		//save data
	 	}

 		if(++secondCounter >= 10)
 		{
	 		secondCounter = 0;
	 		if(++dataToDisplay[2] >= 5)
	 		{
 				if(tubeTraningOn)
 				{
 		 			DisplaySetAnimation(0);
 		 			uint8_t i = 110;
 		 			do
 		 			{
 			 			i-=11;
 			 			dataToDisplay[0] = i;
 			 			dataToDisplay[1] = i;
 			 			dataToDisplay[2] = i;
 			 			DisplaySetData3Num(dataToDisplay);
 			 			_delay_ms(20);
 		 			} while (i>0);
 		 			DisplaySetAnimation(1);
				}else{
					dataToDisplay[2] = 0;
				}	
			}
			DisplaySetData3Num(dataToDisplay);
 		}

 		_delay_ms(100);
		 autoExitMs+=100;
	}	 
}