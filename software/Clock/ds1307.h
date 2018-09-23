/*
 * ds1307.h
 *
 *  Created on: 28.10.2011
 *      Author: bars
 */

#ifndef DS1307_H_
#define DS1307_H_

#include <stdint.h>



void rtc_init(unsigned char rs, unsigned char sqwe, unsigned char out);

void rtc_get_time(unsigned char *hour, unsigned char *min, unsigned char *sec);

void rtc_get_date(unsigned char *date, unsigned char *month, unsigned char *year);

void rtc_set_time(unsigned char hour, unsigned char min, unsigned char sec);

void rtc_set_date(unsigned char date, unsigned char month, unsigned char year);

unsigned char rtc_read(unsigned char address);

void rtc_write(unsigned char address,unsigned char data);

unsigned char bcd (unsigned char data);

void rtc_init_sqw(void);

uint8_t rtc_check_sqw(void);

#endif /* DS1307_H_ */