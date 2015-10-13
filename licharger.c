#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

int main ( void ) {

	//Disable interrupts and reset WDT timer
	cli();
	wdt_reset();

	//Reset MCU status register
	MCUSR &= ~(1<<WDRF);	

	//Disable watchdog
	WDTCR |= (1<<WDCE) | (1<<WDE);
	WDTCR = 0;

	//Enable LED
	DDRB = 1<<DDB3;
	PORTB = 1<<PORTB3;

	//Start watchdog
	WDTCR |= (1<<WDCE) | (1<<WDE);
	WDTCR =  (1<<WDTIE) | (1<<WDP2) | (1<<WDP1);

	//Enable interrupts
	sei();

	while(1);

}

//Catching WatchDog interrupts
ISR ( WDT_vect ) {

	PORTB ^= 1<<PORTB3;

}