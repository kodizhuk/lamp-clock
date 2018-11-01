/*
 * display.c
 *
 * Created: 26.06.2017 21:57:44
 *  Author: Petro
 */ 
#include "display.h"
#include <avr/interrupt.h>
#include "ledLight.h"

#define BRIGHT_NUM_LEVEL	25

uint8_t ledAnimationRequest = 0;

const uint8_t digit[] = {
		0b00000000,		//0
		0b00000100,
		0b00001000,
		0b00001100,
		0b00010000,
		0b00010100,
		0b00011000,
		0b00011100,
		0b00100000,
		0b00100100,		//9
		0b001111100		//off all
};
//0 - max brightness, 255 min brightness(off digit)	
const uint8_t brightConstLevel[BRIGHT_NUM_LEVEL]={1,40,70,90,120,150,170,190,210,230,240,250,250,250,240,230,210,190,170,150,120,90,70,40,1};  
uint8_t brightLevel[BRIGHT_NUM_LEVEL][NUM_DIGIT];
static uint8_t blinkDigit[NUM_DIGIT];		//1-blink, 0 - static light

struct {
	uint8_t newData;
	uint8_t outData;
	uint8_t dataBrightness;
	uint8_t brightLevelDigit;	//brightness level from mas
	uint8_t delay;				//speed change brightness
}data[NUM_DIGIT];

	
animation currentAnimation = SHORT_BLINC;		//SHORT_BLINC or LONG_BLINC
uint8_t currentNum = 0;							//current digit on display (0..5)

ISR(TIMER2_COMP_vect)
{
	PORTB = digit[data[currentNum].outData];		//send data to decoder
	PORTC = (1<<(currentNum));						//next digit
	
	if(++currentNum == NUM_DIGIT)
		currentNum = 0;
}

ISR(TIMER2_OVF_vect)		//every 2ms
{	
		PORTC = 0x00;

		if(currentAnimation == SHORT_BLINC)
		{
			OCR2 =  data[currentNum].dataBrightness;

			if(blinkDigit[currentNum] || (data[currentNum].brightLevelDigit != 0 ))
			{
				if(++data[currentNum].delay == SPEED_BRIGHTNESS)
				{
					data[currentNum].delay = 0;
				
					data[currentNum].brightLevelDigit++;
					if(data[currentNum].brightLevelDigit == BRIGHT_NUM_LEVEL)
					data[currentNum].brightLevelDigit = 0;
				
					if((data[currentNum].brightLevelDigit) > BRIGHT_NUM_LEVEL/2)
					data[currentNum].outData = data[currentNum].newData;		//display new digit
				
					data[currentNum].dataBrightness = brightLevel[data[currentNum].brightLevelDigit][currentNum];	//set brightness to digit
				}
			}else if((data[currentNum].outData != data[currentNum].newData))
			{
				data[currentNum].outData = data[currentNum].newData;
			}
		}
		else if(currentAnimation == LONG_BLINC)
		{			
			OCR2 = data[currentNum].dataBrightness ;

			/*new digit on display*/
			if((data[currentNum].outData != data[currentNum].newData) || (data[currentNum].brightLevelDigit != 0 ))
			{
				if(++data[currentNum].delay == SPEED_BRIGHTNESS)
				{
					data[currentNum].delay = 0;
			
					data[currentNum].brightLevelDigit++;
					if(data[currentNum].brightLevelDigit == BRIGHT_NUM_LEVEL)
						data[currentNum].brightLevelDigit = 0;
					
					if((data[currentNum].brightLevelDigit) > BRIGHT_NUM_LEVEL/2)
						data[currentNum].outData = data[currentNum].newData;		//display new digit 
				
					data[currentNum].dataBrightness = brightLevel[data[currentNum].brightLevelDigit][currentNum];	//set brightness to digit
				}
			}

		}
}

void DisplayInit(void)
{
	DisplaySetBrightness100(100);
	
	/*init pins*/
	DDRB |= (1<<PB2) | (1<<PB3) | (1<<PB4) | (1<<PB5);
	DDRC |= (1<<PB0) |(1<<PB1) |(1<<PB2) |(1<<PB3) |(1<<PB4) |(1<<PB5);
	
	/*init timer*/
	TCCR2 |= (1 << CS22);		//preskaler clk/64 (8bit timer_2)
	TIMSK |= (1<<OCIE2) | (1<<TOIE2);		//timer_2 overflow and compare match enable, period 
}

void DisplaySetData6Num(uint8_t number[] )
{
	uint8_t i;
	for(i=0; i<NUM_DIGIT;i++)
	{
		if(number[i] >= OFF_NUMB)
			data[i].newData = 10;
		else
			data[i].newData = number[i];					//new information
	}	
}

void DisplaySetData3Num(uint8_t number[] )
{
	uint8_t i,j;
	for(i=0,j=0; i<NUM_DIGIT;i+=2,j++)
	{		
		/*new information*/
		if(number[j] >= OFF_NUMB)
		{
			data[i].newData = 10;
			data[i+1].newData = 10;
		}
		else
		{
			data[i].newData = number[j]/10;
			data[i+1].newData = number[j]%10;
		}
	}
}

void DisplaySetAnimation(animation anim)
{
	currentAnimation = anim;
}

animation DisplayReadAnimation()
{
	return currentAnimation;
}

/*
*	brightness = 255 - max brightness
*	brightness = 0 - min brightness (off)
*/
void DisplaySetBrightness(uint8_t brightness)
{
	int i, numDigit;
	
	if(brightness < MIN_BRIGHTNESS_DIGIT)
		brightness = MIN_BRIGHTNESS_DIGIT;

	for (numDigit=0;numDigit<NUM_DIGIT;numDigit++)
	{
		//zoom to new max value
		for(i=0 ;i<BRIGHT_NUM_LEVEL; i++)
			brightLevel[i][numDigit] = ((uint32_t)(brightConstLevel[i]*brightness))/255+(255-brightness);
		
 		data[numDigit].dataBrightness = 255-brightness;		//update
	}		
}

/*
*	brightness = 100 - max brightness
*	brightness = 0 - min brightness (off)
*/
void DisplaySetBrightness100(uint8_t brightness)
{
	uint8_t tmp = brightness;
	if(tmp >= 100)
		tmp = 99;
	tmp = (uint32_t)(tmp*256)/100;
	DisplaySetBrightness(tmp);
}

/*
*	brightness = 255 - max brightness
*	brightness = 0 - min brightness (off)
*/
void DisplaySetBrightnessEachNumber(uint8_t brightness[])
{
	uint8_t i, numDigit;

	for (numDigit=0;numDigit<NUM_DIGIT;numDigit++)
	{
		if(brightness[numDigit] < MIN_BRIGHTNESS_DIGIT)
			brightness[numDigit] = MIN_BRIGHTNESS_DIGIT;
			
		for(i=0 ;i<BRIGHT_NUM_LEVEL; i++)
			brightLevel[i][numDigit] = ((uint32_t)(brightConstLevel[i]*brightness[numDigit]))/255+(255-brightness[numDigit]);

		data[numDigit].dataBrightness = 255- brightness[numDigit];
	}		
}

//brightness: 0-99
void DisplaySetBrightnessEachNumber100(uint8_t brightness[])
{
	uint8_t i, tmp[NUM_DIGIT];
	for(i=0;i<NUM_DIGIT;i++)
	{
		tmp[i] = brightness[i];
		if(tmp[i] >= 100)
		tmp[i] = 99;
		tmp[i] = (uint32_t)(tmp[i]*256)/100;
	}
	DisplaySetBrightnessEachNumber(tmp);
}

/*brightness: 0-255*/
void DisplayReadBrightnessEachNumber(uint8_t brightness[])
{
	uint8_t numDigit;

	for (numDigit=0;numDigit<NUM_DIGIT;numDigit++)
		brightness[numDigit] = 255-brightLevel[0][NUM_DIGIT-numDigit-1];
}

/*1-blink, 0 - static light
* only short_blink mode
*/
void DisplaySetBlinkDigit(uint8_t digitToBlink[])
{
	uint8_t i;
	for(i=0;i<NUM_DIGIT;i++)
		blinkDigit[i] = digitToBlink[i];
}

void DisplayClear(void)
{
	uint8_t i;

	for(i=0; i<NUM_DIGIT;i++)
		data[i].newData = 10;
}

void DisplayRequestUpdateLed(void)
{
	ledAnimationRequest = 1;
}