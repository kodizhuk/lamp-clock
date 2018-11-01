/*
 * ledLight.c
 *
 * Created: 05.08.2017 6:17:09:PM
 *  Author: Petro
 */ 
 #include <avr/io.h>
 #include <avr/interrupt.h>
 #include "stdint.h"
 #include "ledLight.h"
 #include "display.h"
 
 extern void output_grb(uint8_t * ptr, uint16_t count);
 static void Wheel(int8_t WheelPos,uint8_t *red, uint8_t *green, uint8_t *blue);
 static void allColorSetRGB(uint8_t *col1, uint8_t *col2, uint8_t jump);

 uint8_t buffLeds[NUM_LEDS*3];			//real color, max brigtness
 uint8_t buffLedsBright[NUM_LEDS*3];	//color with current brightness 
 uint8_t brigtness = 99;

 void LedInit(void)
 {
	DDRB |= (1<<PB0);		//output
	PORTB &= (~(1<<PB0));	//0
	//PORTB |= (1<<PB0);		//1
 }


 void LedUpdate(void)
 {
	output_grb(buffLedsBright, sizeof(buffLedsBright));
 }

 void LedSetColorRGB( uint8_t led, uint8_t r, uint8_t g, uint8_t b)
 {
	uint16_t index = 3*led;
	buffLedsBright[index] = (g*brigtness)/LED_MAX_BRIGHTNESS;
	buffLeds[index++] = g;
	buffLedsBright[index] = (r*brigtness)/LED_MAX_BRIGHTNESS;
	buffLeds[index++] = r;
	buffLedsBright[index] = (b*brigtness)/LED_MAX_BRIGHTNESS;
	buffLeds[index] = b;
 }

 void LedSetColor(uint8_t numLed, uint8_t color[])
 {
	LedSetColorRGB(numLed, color[0], color[1], color[2]);
 }

 void LedSetColorRGBAllLed(uint8_t r, uint8_t g, uint8_t b)
 {
	uint8_t numLed;

	for(numLed=0; numLed< NUM_LEDS;numLed++)
		LedSetColorRGB(numLed, r,g,b);
 }

 /*animation effect*/
 void LedOffAll(void)
 {
	int i;
	for(i=0;i<NUM_LEDS;i++)
		LedSetColorRGB( i, 0, 0, 0);
 }

 void LedTheaterChaseRainbow(void)
 {
	static uint16_t j;
	static uint8_t q;
	uint8_t r,g, b;
	int i;
	if (j < 256)
	{     // cycle all 256 colors in the wheel
		if ( q < 3)
		{
			for (i=0; i < NUM_LEDS && i+q<NUM_LEDS; i+=3)
			{
				Wheel(((i+j) % 255),&r,&g,&b);
				LedSetColorRGB( i+q, r, g, b);
			}
			q++;
		}else q = 0;
		j++;
	}else j = 0;
	//LedUpdate();
	DisplayRequestUpdateLed();
 }

 void LedRainbowCycle(void)
 {
	static uint16_t j;
	uint16_t i;
	uint8_t r,g, b;
	
	if(j<256*5)
	{ // 5 cycles of all colors on wheel
		for(i=0; i< NUM_LEDS; i++)
		{
			Wheel((((i * 256/NUM_LEDS)+j)&255),&r,&g,&b);
			LedSetColorRGB(i, r, g, b);
		}
		j++;
	}else j =0;
	//LedUpdate();
	DisplayRequestUpdateLed();
 }

 void LedRainbow(void) 
 {
	static uint16_t j;
	uint16_t i;
	uint8_t r,g, b;
	
	if(j<256)
	{
		for(i=0; i<NUM_LEDS; i++)
		{
			Wheel(((i+j) & 255),&r,&g,&b);
			LedSetColorRGB( i, r, g, b);
		}
		j++;
	}else j = 0;
	//LedUpdate();
	DisplayRequestUpdateLed();
 }

 void LedAllColorAnim(uint8_t jump, direction dir)
 {
	static uint8_t r=255,g,b;
	uint8_t i;
	
	if(dir==UP)
	{
		if(b == 0)	allColorSetRGB(&g,&r,jump);
		if(r == 0)	allColorSetRGB(&b,&g,jump);
		if(g == 0)	allColorSetRGB(&r,&b,jump);
	}else
	{
		if(b == 0)	allColorSetRGB(&r,&g,jump);
		if(r == 0)	allColorSetRGB(&g,&b,jump);
		if(g == 0)	allColorSetRGB(&b,&r,jump);
	}
	
	for(i=0;i<NUM_LEDS;i++)
		LedSetColorRGB( i, r, g, b);
	//LedUpdate();
	DisplayRequestUpdateLed();
 }

 static void allColorSetRGB(uint8_t *col1, uint8_t *col2, uint8_t jump)
 {
	if((*col1+jump)<=255)*col1+=jump;
	else *col1 = 255;
	if(*col1==255)
	{
		if((*col2-jump)>=0)*col2-=jump;
		else *col2 = 0;
	}
 }

 static void Wheel(int8_t WheelPos,uint8_t *red, uint8_t *green, uint8_t *blue)
 {
	 WheelPos = 255 - WheelPos;
	 if(WheelPos < 85)
	 {
		 //strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
		 *red = 255 - WheelPos*3;
		 *blue = 0;
		 *green = WheelPos*3;
	 }
	 else if(WheelPos < 170)
	 {
		 WheelPos -= 85;
		 *red = 0;
		 *blue = WheelPos * 3;
		 *green = WheelPos*3;
		 //strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
	 }
	 else
	 {
		 WheelPos -= 170;
		 *red = WheelPos * 3;
		 *blue = 255 - WheelPos * 3;
		 *green = 0;
		 //strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
	 }
 }

 /*bright 0..99
 * 0 - min brightness
 * 99 - max brightness
 */
 void LedSetBrigtness(uint8_t bright)
 {
	if(bright <= LED_MAX_BRIGHTNESS) brigtness = bright;
	else brigtness = LED_MAX_BRIGHTNESS;
 }

 uint8_t LedReadBrigtness()
 {
	return brigtness;
 }

 void LedReadColor(uint8_t led, uint8_t *r, uint8_t *g, uint8_t *b)
 {
	uint16_t index = 3*led;
	*g = buffLeds[index++];
	*r = buffLeds[index++];
	*b = buffLeds[index];
 }