/*
 * ledLight.h
 *
 * Created: 05.08.2017 6:17:37:PM
 *  Author: Petro
 */ 


#ifndef LEDLIGHT_H_
#define LEDLIGHT_H_

#define NUM_LEDS	6
#define LED_MAX_BRIGHTNESS	99
#define LED_MIN_BRIGHTNESS 0
typedef enum {UP,DOWN}direction;

void LedInit(void);
void LedUpdate(void);

void LedSetColorRGBAllLed(uint8_t r, uint8_t g, uint8_t b);
void LedSetColorRGB( uint8_t led, uint8_t r, uint8_t g, uint8_t b);			//set color for one led
void LedSetColor(uint8_t numLed, uint8_t color[]);
void LedReadColor(uint8_t numLed, uint8_t *r, uint8_t *g, uint8_t *b);		//read color one led

/*animation effect*/
void LedOffAll(void);

void LedTheaterChaseRainbow(void) ;
void LedRainbowCycle(void);
void LedRainbow(void) ;

void LedAllColorAnim(uint8_t jump, direction dir);

void LedSetBrigtness(uint8_t brigtness);
uint8_t LedReadBrigtness(void);



#endif /* LEDLIGHT_H_ */