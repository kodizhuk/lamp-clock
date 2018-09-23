#ifndef CONTROL_H_
#define CONTROL_H_

typedef enum {NO_PRESS, PRESS_LEFT, PRESS_CENTER, PRESS_RIGHT, 
			PRESSED_LEFT, PRESSED_CENTER, PRESSED_RIGHT}control_result;

void ControlInit();
control_result ControlCheck();

#endif /* CONTROL_H_ */