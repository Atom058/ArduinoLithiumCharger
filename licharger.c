#include <licharger.h>

//Definitions
#define VLOWLIMIT = 500; //Voltage where battery is considered depleted - 3.2V
#define VHIGHLIMIT = 1000; //Charge to this voltage - 4.15V
#define VCVLIMIT = 850; //Voltage limit where charging switches from constant current to constant voltage.
#define VCHARGELIMIT = 850; //Voltage above which charging will not initiate - 4.0V

//Booleans
uint8_t state = 0;
#define HASRUNSETUP = 0;
#define USBCONNECTED = 1;
#define CHARGING = 2;
#define CIRCUTRUNNING = 3;
#define BATTERYDEPLETED = 4;

uint16_t batteryVoltage = 0;

int main ( void ) {

	if((state>>HASRUNSETUP) & 1){

		setup();

	}

	while(1){

		batteryVoltage = readVoltage();

	}

	if(readVoltage() < VLOWLIMIT){

		powerdown();

	}

	if((state>>USBCONNECTED) & 1){

		charge();

	}

	//Enable LED
	DDRB = 1<<DDB3;
	PORTB = 1<<PORTB3;


}



void setup(void) {

	//Record that setup has been run
	state |= (1<<HASRUNSETUP);

/*Watch dog settings*/
	//Disable interrupts and reset WDT timer
	cli();
	wdt_reset();

	//Reset MCU status register
	MCUSR &= ~(1<<WDRF);	

	//Disable watchdog
	WDTCR |= (1<<WDCE) | (1<<WDE);
	WDTCR = 0;

	//Start watchdog, firing every 2 seconds
	WDTCR |= (1<<WDCE) | (1<<WDE);
	WDTCR =  (1<<WDTIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);

	//Enable interrupts
	sei();


/*Analog to Digital converter settings*/
	//Set ADC to use 1.1V internal reference and 
	//	select ADC1 (pin7) as input channel and disable it as a digital pin
	ADMUX = (0<<REFS0) | (0<<MUX1) | (1<<MUX0) | (0<<ADLAR);
	DIDR0 = (1<<ADC1D);

	/*Set the clock speed for the ADC. Shoul be between 100 and 200 kHz the 
		System clock is initially 9.6MHz, with a divider of 8, making the
		actual CPU freuqency 1.2Mz. A prescaler of 8 gives a frequency of
		150 kHz, which is acceptable.

		Also: disable ADC: 
			- ADATE: Auto triggering conversions
			- ADIE: interupts on completion

	*/
	ADCSRA = (0<<ADATE) | (0<<ADIE) | (0<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);

	//Read voltage - first one should be discarded
	readVoltage();

}



uint16_t readVoltage(void){
	
	//Make sure that the ADC is enabled
	ADCSRA |= (1<<ADEN);
	//Start voltage conversion
	ADCSRA |= (1<<ADSC);

	//Wait for conversion to finish
	while((ADCSRA>>ADSC) & 1){}

	uint16_t reading = ADCL | (ADCH<<8);

	return reading;

}



/*
	Interrupt catching
*/

//Catching WatchDog interrupts
ISR ( WDT_vect ) {

	PORTB ^= 1<<PORTB3;

}