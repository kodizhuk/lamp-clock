#include "brightness_senor.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define ADC_CHANEL 6

volatile uint8_t dataADC = 0;

ISR(ADC_vect)
{
	dataADC = ADCH;
}

void BrightnessInit()
{
	ADMUX |= (1 << REFS0)|(1<<ADLAR)|(1<<MUX1)|(1<<MUX2);		//6 chanel adc
	ADCSRA |= (1 << ADEN)|(1 << ADSC)|(1 << ADIE)|(1<<ADFR)|(1<<ADPS2)|(1<<ADPS1)|(1 << ADPS0);	
}

uint8_t BrightnessGet()
{
	return dataADC;
}