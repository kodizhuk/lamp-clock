/*
 * encoder.c
 *
 * Created: 27.06.2017 17:57:10
 *  Author: Petro
 */ 
#include "encoder.h"
#include <avr/io.h>
#include <avr/sfr_defs.h>


#define CLK PD0
#define DT	PD1
#define BTN PD2

control_result encoderTurn = NO_PRESS;
control_result btnClick = NO_PRESS;
uint8_t lastState, newState;

control_result EncoderBtnCheck();

void EncoderInit()
{
	DDRD &=(~((1<<CLK)|(1<DT)|(1<<BTN)));			//  input
	PORTD |= ((1<<CLK)|(1<<DT)|(1<<BTN));		// pull up
	
	lastState = PIND & ((1<<CLK)|(1<<DT));
}

control_result EncoderCheck()
{
	encoderTurn = NO_PRESS;
	newState = PIND & ((1<<CLK)|(1<<DT));
	switch(lastState)
	{
		//case 2:
			//if(newState == 3)encoderTurn = PRESS_RIGHT;
			//if(newState == 0)encoderTurn = PRESS_LEFT;
			//break;
		//case 0:
			//if(newState == 2)encoderTurn = PRESS_RIGHT;
			//if(newState == 1)encoderTurn = PRESS_LEFT;
			//break;
		//case 1:
			//if(newState == 0)encoderTurn = PRESS_RIGHT;
			//if(newState == 3)encoderTurn = PRESS_LEFT;
			//break;
		case 3:
			if(newState == 1)encoderTurn = PRESS_RIGHT;
			if(newState == 2)encoderTurn = PRESS_LEFT;
			break;
		default:
			break;
	}
	lastState = newState;

	if(EncoderBtnCheck() == PRESS_CENTER)
		encoderTurn = PRESS_CENTER;
	
	return encoderTurn;
}

control_result EncoderBtnCheck()
{
	static uint8_t permissNextClick = 1;

	btnClick = NO_PRESS;
	if(bit_is_clear(PIND,BTN))
	{
		if(permissNextClick)
		{
			btnClick = PRESS_CENTER;
			permissNextClick = 0;
		}
		else
			btnClick = NO_PRESS;
	}
	else 
	{
		btnClick = NO_PRESS;
		permissNextClick = 1;
	}
	
	return btnClick;
}