#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdlib.h>
#include <util/delay.h>
#include "usb_serial.h"
#include "uart.h"

#define BAUD_RATE 115200
#define LED_CONFIG	(DDRD |= (1<<6))
#define LED_ON		(PORTD |= (1<<6))
#define LED_OFF		(PORTD &= ~(1<<6))
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

//Global Variables
char post;
char buffer0 = 0; //, buffer1 = 0;

void setup(void)
{
	//CPU_PRESCALE(0x01);  // run at 8 MHz //needed for uart
	CPU_PRESCALE(0x0);  // run at 16 MHz
	LED_CONFIG;
	
	//Turn LED on for setup
	LED_ON;
	DDRD = 0x00; //Set Port D as Input
	PORTD = 0xFF; //Activate all pullups on Port D
	uart_init(BAUD_RATE); //needed for uart
	usb_init();
	
	while (!usb_configured()) /* wait */ ;
	//_delay_ms(1000); //delay for driver setup
	
	// wait for the user to run their terminal emulator program
	// which sets DTR to indicate it is ready to receive.
	while (!(usb_serial_get_control() & USB_SERIAL_DTR)) /* wait */ ;
	
	// discard anything that was received prior.  Sometimes the
	// operating system or other software will send a modem
	// "AT command", which can still be buffered.
	usb_serial_flush_input();
	
	//Turn LED off when setup complete
	LED_OFF;
}

void int2hex(int value, char *hex) 
{
	char low = value & 0x0F;
	char high = (value & 0xF0) >> 4;
	
	//ASCII 0 is 0x30, ASCII 'A' is 0x41
	if (high <=9) 
	{
		hex[0] = (0x30 + high);
	} else
	{
		high -= 10;
		hex[0] = (0x41 + high);
	}
	
	if (low <= 9)
	{
		hex[1] = (0x30 + low);
	} else 
	{
		low -= 10;
		hex[1] = (0x41 + low);
	}
}

int main(void)
{
	//Setup
	setup();
	
	for(;;) 
	{
		//Local Variables
		post = uart_getchar();
		if (post == -1) usb_send_str(PSTR("failed to get character \r\n"));
		if(post == buffer0) continue;
		//LED_ON;
		char *hex = malloc(2);
		int2hex(post, hex);
		
		//usb serial design
		usb_serial_putchar(hex[0]);
		usb_serial_putchar(hex[1]);
		usb_serial_putchar((char)13); // carriage return
		usb_serial_putchar((char)10); // newline

		free(hex);
		//LED_OFF;
		//buffer1 = buffer0;
		buffer0 = post;
	}
	
	return 0;
}
