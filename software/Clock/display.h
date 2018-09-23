/*
 * display.h
 *
 * Created: 26.06.2017 21:58:01
 *  Author: Petro
 */ 


#ifndef DISPLAY_H_
#define DISPLAY_H_
#include <stdint.h>

#define SPEED_BRIGHTNESS	3		//value 1..3 (1 fast, 3 slow)
#define NUM_DIGIT			6
#define OFF_NUMB			100
typedef enum {SHORT_BLINC,LONG_BLINC}animation;
	
#define MIN_BRIGHTNESS_DIGIT	10		//0-255

void DisplayInit(void);
void DisplaySetData6Num(uint8_t number[] );
void DisplaySetData3Num(uint8_t number[] );
void DisplaySetAnimation(animation anim);
animation DisplayReadAnimation(void);
void DisplaySetBrightness(uint8_t brightness);
void DisplaySetBrightness100(uint8_t brightness);
//void DisplaySetBrightnessEachNumber(uint8_t brightness[]);
void DisplaySetBrightnessEachNumber100(uint8_t brightness[]);
void DisplayReadBrightnessEachNumber(uint8_t brightness[]);

void DisplaySetBlinkDigit(uint8_t digitToBlink[]);

void DisplayClear(void);

void DisplayRequestUpdateLed(void);




#endif /* DISPLAY_H_ */