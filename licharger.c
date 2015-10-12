#include <avr/io.h>

int main (void) {

	//Set pin 3 as output to source current?
	PORTB = 1<<PORTB3;
	DDRB = 1<<DDB3;

}