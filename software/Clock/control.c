#include "control.h"
#include <avr/io.h>
#include <avr/sfr_defs.h>


#define BTN0 PD0
#define BTN2 PD1
#define BTN1 PD2

control_result result = NO_PRESS;

uint8_t permissNextClick[3] = {1,1,1};


void ControlInit()
{
	DDRD &=(~((1<<BTN0)|(1<BTN1)|(1<<BTN2)));		//  input
	PORTD |= ((1<<BTN0)|(1<<BTN1)|(1<<BTN2));		// pull up
}

control_result ControlCheck()
{
	result = NO_PRESS;
	//first
	if(bit_is_clear(PIND, BTN1))
	{
		if(permissNextClick[0])
		{
			result = PRESS_L;
			permissNextClick[0] = 0;
		}else
		result = PRESSED_L;
	}else
	{
		permissNextClick[0] = 1;
	}
	//second
	if(bit_is_clear(PIND, BTN2))
	{
		if(permissNextClick[1])
		{
			result = PRESS_CENTER;
			permissNextClick[1] = 0;
		}else
		result = PRESSED_CENTER;
	}else
	{
		permissNextClick[1] = 1;
	}
	//third
	if(bit_is_clear(PIND, BTN0))
	{
		if(permissNextClick[2])
		{
			result = PRESS_R;
			permissNextClick[2] = 0;
		}else
		result = PRESSED_R;
	}else
	{
		permissNextClick[2] = 1;
	}
	
	return result;
}