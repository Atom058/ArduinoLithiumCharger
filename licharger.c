#include <licharger.h>

uint8_t state = 0;

uint16_t batteryVoltage = 0;

int main ( void ) {

	setup();

	while(1){
		//This loop is running constantly

		if((PINB>>PINB3) & 1){
			
			//Turn on circuit power
			PORTB |= (1<<PORTB4);

			if( !((state>>CHARGING) & 1) && (batteryVoltage < VCHARGELVL)){

				// If the battery is not yet charging, AND below the charge limit
				state |= (1<<CHARGING);

			} else {

				//Charging logic
				
				if(batteryVoltage > VLOWLVL){

					//battery is no longer considered empty
					state &= ~(1<<BATTERYDEPLETED);

				}

				if (batteryVoltage < VCHARGELVL){

					PORTB |= (1<<PORTB1); //Turn on charging

				} else if (batteryVoltage >= VHIGHLVL){

					//If VHIGHLVL has been reached, charging should stop
					state &= ~(1<<CHARGING);
					PORTB &= ~(1<<PORTB1);

				}

			}

		} else {

			//If external power is disconnnected, 
			//  Wait for WDT (reset timer) and put processor to sleep

		}

	}

}



void setup(void) {

//Disable interrupts and reset WDT timer to allow setup to finish
	cli();
	
/*Interrupts*/

	/*Watch dog start, firing every 2s */
		MCUSR &= ~(1<<WDRF); //Clear WD interrupt flag
		wdt_reset(); // Reset timer
		WDTCR |= (1<<WDCE) | (1<<WDE);
		WDTCR = (0<<WDE) | (0<<WDTIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);

		startWatchdog();

	/*USB-sense (pin-chage) interupt*/

		//Select PCINT3 as interrupt source (pin 2)
		GIMSK = (1<<PCIE); //Enable pin interrupts
		GIMSK &= ~(1<<INT0); //... but not for INT0
		PCMSK = (1<<PCINT3);

//POWER SETTINGS
	//Disable the Analog Comparator circuit 
	//	Note: this is not the ADEN bit, so the CONVERTER is still active
	ACSR &= ~(1<<ACIE); //Disables the interrupt to avoid disturbance during setup
	ACSR |= (1<<ACD); //Disable Comparator

	//Enable deep sleep, i.e. power-down mode, as default
	MCUCR &= ~(1<<SM0);
	MCUCR |= (1<<SM1);

	PRR = 0; //Initialize this as full-on

/*PORTB Pin settings*/
	//Configure input or output
	PORTB = 0; //Disables all lingering outputs and pull-ups
	DDRB = (1<<DDB0) | (1<<DDB1) | (0<<DDB2) | (0<<DDB3) | (1<<DDB4) | (0<<DDB5);

/*Analog to Digital converter settings*/
	// Set ADC to use 1.1V internal reference and standard right-adjusted results
	// Set and select ADC1 (pin7) as an input channel and disable as digital
	// Set ADC3 (pin2) as an input channel
	ADMUX = (1<<REFS0) | (0<<ADLAR) | (0<<MUX1) | (1<<MUX0);
	// Disable Output on ADC1
	DIDR0 = (1<<ADC1D);
	


	/*Set the clock speed for the ADC. Should be between 100 and 200 kHz the 
		System clock is initially 9.6MHz, with a divider of 8, making the
		actual CPU freuqency 1.2Mz. A prescaler of 8 gives a frequency of
		150 kHz, which is acceptable.

		Also: disable ADC: 
			- ADATE: Auto triggering conversions
			- ADIE: interupts on completion

		//TODO update this if CPU speed is adjusted

	*/
	ADCSRA = (0<<ADIE) | (0<<ADATE) | (0<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);

//Enable interrupts again
	sei();

}

void startWatchdog(){

	//Start watchdog interrupts, firing every 2 seconds
	WDTCR |= (1<<WDCE) | (1<<WDE);
	WDTCR |= (1<<WDTIE);

}

void stopWatchdog(){

	//Reset MCU status register
	MCUSR &= ~(1<<WDRF);
	wdt_reset();

	WDTCR |= (1<<WDCE) | (1<<WDE); //Enable change mode
	WDTCR &= ~((1<<WDE) | (1<<WDTIE)); //unset WD

}


/*
	Reads voltage from the battery-voltage port
		Please disable interrupts before this function is run
*/
uint16_t readVoltage(){

	//Enable power to sense circuit
	PORTB |= (1<<PORTB0);
	
	//Make sure that the ADC is powered and enabled
	PRR &= ~(1<<PRADC);
	ADCSRA |= (1<<ADEN);

	//Make sure internal bandgap has stabilized
	_delay_us(BANDGAPDELAY);

	//Start voltage conversion
	ADCSRA |= (1<<ADSC);

	while((ADCSRA>>ADSC) & 1){
		//Wait for conversion to finish
	}

	//Reset conversion-done flag
	state &= ~(1<<ADCDONE);

	//Save reading to memory
	uint16_t reading = (uint16_t) ADCL;
	reading |= (ADCH<<sizeof(ADCH));

	//Disable power to sense pin
	PORTB &= ~(1<<PORTB0);

	return reading;

}



void sleep(void){

	cli(); //Disable interrupts 

	if(((state>>BATTERYDEPLETED) & 1) && !((PINB>>PINB3) & 1)){

		/*If the battery is depleted and USB is not connected,
		 	The watchdog (with its voltage checking) should be disabled.
			This means that only a pin change can wake the device (i.e. USB Connected)
		*/
		stopWatchdog();


	} 

	//ADC off
	ADCSRA &= ~(1<<ADEN); //Must be disabled before PRADC
	PRR |= (1<<PRADC);
	//Turn off Timer0
	PRR |=  (1<<PRTIM0);

	//Put device to sleep
	MCUCR |= (1<<SE); //Sleep enabled
	sei(); //Enable interrupts again
	asm( "sleep" ); //Sleep command

}



/*


	Interrupt catching


*/

/*	[--WatchDog interrupts--]
	
	Watchdog is used for different funcitons depending on state. When the watchdog 
		timer runs out, there are the following options:

To reduce power consumption, the processor is put to sleep most of the time. 

	- If USB is connected: start charging logic - Does nothing

	- If 3.2 < V(bat) < 4.2: allow circuit power.

	- If V(bat) < 3.2: disallow circuit power, and turn off the ATtiny (this chip).
		Turn on interrupt on connected USB. This will reduce power as much as possible.

*/
ISR ( WDT_vect ) {

	//Disable sleep
	MCUCR &= ~(1<<SE);

	cli();
	batteryVoltage = readVoltage();
	sei();

	if((PINB>>PINB3) & 1){

		//Fool-proofing: check USB-connected pin to make sure that interrupt wasn't faulty
		//	This is done to make sure power conservation is activated properly

	} else {

		//Turn of charging stuff
		state &= ~(1<<CHARGING);
		PORTB &= ~(1<<PORTB0);

		if(batteryVoltage < VLOWLVL){

			state |= (1<<BATTERYDEPLETED);
			//Disable circuit power
			PORTB &= ~(1<<PORTB4);

		} else {

			//Enable circuit power
			PORTB |= (1<<PORTB4);

		}

		sleep();

	}

}

ISR ( PCINT0_vect ){

	//Disable sleep
	MCUCR &= ~(1<<SE);

	if((WDTCR>>WDTIE) ^ 1){

		//Start watchdog if it is disabled
		startWatchdog();

	}

	if(!((PINB>>PINB3) & 1)){

		//Reset all charging logic, USB was removed
		state &= ~(1<<CHARGING);
		PORTB &= ~(1<<PORTB1);

	} 

}