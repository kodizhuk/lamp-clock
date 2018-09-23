/*
 * encoder.h
 *
 * Created: 27.06.2017 17:56:54
 *  Author: Petro
 */ 


#ifndef ENCODER_H_
#define ENCODER_H_
#include <stdint.h>

typedef enum {NO_PRESS, PRESS_LEFT, PRESS_RIGHT, PRESS_CENTER}control_result;

void EncoderInit();
control_result EncoderCheck();



#endif /* ENCODER_H_ */