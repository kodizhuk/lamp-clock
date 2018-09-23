#include <avr/io.h>
#include "I2c.h"
#include <avr/io.h>
#include <avr/sfr_defs.h>

#define SQW_PORT	PORTD
#define SQW_INPUT	PIND
#define SQW_DDR		DDRD
#define SQW_PIN		PD3
#define bit_is_set(sfr,bit) \
(_SFR_BYTE(sfr) & _BV(bit))


volatile unsigned char binar;
uint8_t sqw_state=0;

unsigned char bcd (unsigned char data)
{
	unsigned char bc;

	bc=((((data&(1<<6))|(data&(1<<5))|(data&(1<<4)))*0x0A)>>4)+((data&(1<<3))|(data&(1<<2))|(data&(1<<1))|(data&0x01));

	return bc;
}

unsigned char bin(unsigned char dec){

	char bcd;

	char n, dig, num, count;

	num = dec;
	count = 0;
	bcd = 0;
	for (n=0; n<4; n++) {
		dig = num%10;
		num = num/10;
		bcd = (dig<<count)|bcd;
		count += 4;
	}

	return bcd;
}


void rtc_init(unsigned char rs, unsigned char sqwe, unsigned char out)
{
	rs&=18;
	if (sqwe) rs|=0x40;
	if (out) rs|=0x80;
	i2c_start();
	i2c_send_byte(0xd0);
	i2c_send_byte(0x0e);
	i2c_send_byte(rs);
	i2c_stop();
}

void rtc_get_time(unsigned char *hour, unsigned char *min, unsigned char *sec)
{
	i2c_start(); 
	i2c_send_byte(0xD0); 
	i2c_send_byte(0x00); 
			
	i2c_restart(); 
	i2c_send_byte(0xD1); 
	*sec = bcd(i2c_read_byte(ACK)); 
	*min = bcd(i2c_read_byte(ACK));
	*hour = bcd(i2c_read_byte(NACK));
	i2c_stop(); 
}

void rtc_set_time(unsigned char hour,unsigned char min,unsigned char sec)
{
i2c_start();
i2c_send_byte(0xd0);
i2c_send_byte(0);
i2c_send_byte(bin(sec));
i2c_send_byte(bin(min));
i2c_send_byte(bin(hour));
i2c_stop();
}

void rtc_get_date(unsigned char *date,unsigned char *month,unsigned char *year)
{
i2c_start();
i2c_send_byte(0xd0);
i2c_send_byte(4);

i2c_restart();
i2c_send_byte(0xd1);
*date=bcd(i2c_read_byte(ACK));
*month=bcd(i2c_read_byte(ACK));
*year=bcd(i2c_read_byte(NACK));
i2c_stop();
}

void rtc_set_date(unsigned char date,unsigned char month,unsigned char year)
{
i2c_start();
i2c_send_byte(0xd0);
i2c_send_byte(4);
i2c_send_byte(bin(date));
i2c_send_byte(bin(month));
i2c_send_byte(bin(year));
i2c_stop();
}

unsigned char rtc_read(unsigned char address)
{
unsigned char data;
i2c_start();
i2c_send_byte(0xd0);
i2c_send_byte(address);
i2c_restart();
i2c_send_byte(0xd1);
data=bcd(i2c_read_byte(NACK));
i2c_stop();
return data;
}

void rtc_write(unsigned char address,unsigned char data)
{
i2c_start();
i2c_send_byte(0xd0);
i2c_send_byte(address);
i2c_send_byte(bin(data));
i2c_stop();
}

void rtc_init_sqw(void)
{			
	SQW_DDR &= (~(1<<SQW_PIN));		//input
	//SQW_DDR |= ((1<<SQW_PIN));		//output
	SQW_PORT |= ((1<<SQW_PIN));		//pull up
	//SQW_PORT &= (~(1<<SQW_PIN));		//pull down
	sqw_state = 0;
}

uint8_t rtc_check_sqw(void)
{
	static uint8_t last_sqw_state;
	last_sqw_state = sqw_state;

	sqw_state = (SQW_INPUT & (1<<SQW_PIN));
	
	if((last_sqw_state == 0) && (sqw_state != 0))	//rising edge
		return 1;
	else 
		return 0;
}