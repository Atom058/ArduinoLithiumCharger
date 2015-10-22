#include <licharger.h>

//Definitions
#define VLOWLIMIT 500 //Voltage where battery is considered depleted - 3.2V
#define VHIGHLIMIT 1000 //Charge to this voltage - 4.15V
#define VCVLIMIT 850 //Voltage limit where charging switches from constant current to constant voltage.
#define VCHARGELIMIT 850 //Voltage above which charging will not initiate - 4.0V
#define BANDGAPDELAY 50 //Calculated as follows: Takes three CPU cycles per loop, and 100us needed

//Booleans
uint8_t state = 0;
#define USBCONNECTED 1
#define CHARGING 2
#define BATTERYDEPLETED 3

uint16_t batteryVoltage = 0;

int main ( void ) {

	setup();

	while(1){
		//This loop is running constantly

		if((state>>USBCONNECTED) & 1){
			
			batteryVoltage = readVoltage();
			//Turn on circuit power
			PORTB |= (1<<PORTB4);

			if( ((state>>CHARGING) ^ 1) && (batteryVoltage < VCHARGELIMIT)){

				// If the battery is not yet charging, AND below the charge limit
				state |= (1<<CHARGING);
				continue; //Start the next loop

			} else {

				//Charging logic
				
				if(batteryVoltage < VCVLIMIT){

					//Charge with constant current below VCVLIMIT
					PORTB &= ~(1<<PORTB1); //Turn off constant voltage pin
					PORTB |= (1<<PORTB0); //Turn on constant current pin

					if(batteryVoltage > VLOWLIMIT){

						//battery is no longer completely empty
						state &= ~(1<<BATTERYDEPLETED);

					}

				} else if (batteryVoltage < VHIGHLIMIT){

					PORTB &= ~(1<<PORTB0); //Turn off constant current pin
					PORTB |= (1<<PORTB1); //Turn on constant voltage pin

				} else {

					//If VHIGHLIMIT has been reached, charging should stop
					state &= ~(1<<CHARGING);
					PORTB &= ~((1<<PORTB0) | (1<<PORTB1));

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
	wdt_reset();

//POWER SETTINGS
	//Disable the Analog Comparator circuit 
	//	Note: this is not the ADEN bit, so the CONVERTER is still active
	ACSR &= ~(1<<ACIE); //Disables the interrupt; should strictly be unnecessary
	ACSR |= (1<<ACD); //Disable Comparator

	//Enable deep, power-down mode, sleep as default
	MCUCR &= ~(1<<SM0);
	MCUCR |= (1<<SM1);

/*PINS AND HOW THEY ARE USED
	pin 1: 
		RESET | when signal low - used for programming
		PB5 | Toggle signal LED
	pin 2: 
		PCINT3 | Pin-change interrupt
		PB3 | Digital IN
	pin 3:
		PB4 | Circuit enable
	pin 4: 
		GND
	pin 5: 
		PB0 | Enable constant current charging
	pin 6: 
		PB1 | Enable constant voltage charging
	pin 7: 	
		ADC1 | Measure battery voltage
		PB2 | Disabled
	pin 8: 
		VCC
*/

/*PORTB Pin settings*/
	//Configure input or output
	PORTB = 0; //Disables all lingering outputs and pull-ups
	DDRB = (1<<DDB0) | (1<<DDB1) | (0<<DDB2) | (0<<DDB3) | (1<<DDB4) | (1<<DDB5);

/*Interrupts*/

	/*Watch dog start*/

		startWatchdog();

	/*USB-sense (pin-chage) interupt*/

		//Select PCINT3 as interrupt source (pin 2)
		PCMSK = (1<<PCINT3);
		GIMSK = (0<<INT0) | (1<<PCIE);

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
	ADCSRA = (0<<ADATE) | (0<<ADIE) | (0<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);

//Enable interrupts again
	sei();

}

void startWatchdog(){

	//Reset MCU status register
	MCUSR &= ~(1<<WDRF);

	//Start watchdog interrupts, firing every 2 seconds
	WDTCR |= (1<<WDCE) | (1<<WDE);
	WDTCR =  (1<<WDTIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);

}

void stopWatchdog(){

	//Reset MCU status register
	MCUSR &= ~(1<<WDRF);

	WDTCR |= (1<<WDCE) | (1<<WDE);	
	WDTCR = 0; //This disables RESETs being fired

}


/*
	Reads voltage from the battery-voltage port
		The 
*/
uint16_t readVoltage(){

	// THE BELOW SNIPPET IS CURRENTLY NOT USED, BUT CAN BE ACTIVATED IF NEEDED.
	// Add a 'unit_8 port' argument to the method first
	// Select the corresponding port for conversion
	// switch (port){

	// 		//MUX[0:0]
	// 	case 0:
	// 		ADMUX &= ~(1<<MUX0 | 1<<MUX1);
	// 		break;

	// 		//MUX[0:1]
	// 	case 1:
	// 		ADMUX |= (1<<MUX0);
	// 		ADMUX &= ~(1<<MUX1);
	// 		break;

	// 		//MUX[1:0]
	// 	case 2:
	// 		ADMUX &= ~(1<<MUX0);
	// 		ADMUX |= (1<<MUX1);
	// 		break;

	// 		//MUX[0:1]
	// 	case 3:
	// 		ADMUX |= (1<<MUX0) | (1<<MUX1);
	// 		break;

	// 	default:
	// 		return 0;

	// }
	
	//Make sure that the ADC is enabled
	ADCSRA |= (1<<ADEN);

	//Make sure internal bandgap has stabilized
	_delay_loop_1(BANDGAPDELAY);

	//Start voltage conversion
	ADCSRA |= (1<<ADSC);

	//Wait for conversion to finish
	while((ADCSRA>>ADSC) & 1){}

	//Save reading to memory
	uint16_t reading = ADCL;
	reading |= (ADCH<<8);

	return reading;

}



void sleep(void){

	cli(); //Disable interrupts 

	if(((state>>BATTERYDEPLETED) & 1) && !((state>>USBCONNECTED) & 1)){

		/*If the battery is depleted and USB is not connected,
		 	The watchdog (with its voltage checking) should be disabled.
			This means that only a pin change can wake the device (i.e. USB Connected)
		*/
		wdt_reset();
		stopWatchdog();

	} 

	//General powerdown
	ADCSRA &= ~(1<<ADEN); //ADC off
	//Turn of ADC and Timer0
	// IF defines were working, the following is equivalent and easier to understand:
	//  PRR |= (1<<PRADC) | (1<<PRTIM0);
	_SFR_IO8(0x25) |= (1<<0) | (1<<1);

	sei(); //Enable interrupts again

}



/*


	Interrupt catching


*/

/*	[--WatchDog interrupts--]
	
	Watchdog is used for different funcitons depending on state. When the watchdog 
		timer runs out, there are the following options:

To reduce power consumption, the processor is put to sleep most of the time. 

	- If USB is connected: start charging logic - _TURNS WATCHDOG FUNCTION OFF_
		- Blinking LED when charging
		- Constant LED when not charging 

	- If 3.2 < V(bat) < 4.2: allow circuit power.
		- [NO!] Turn on an interrupt on RESET-pin, connected to a "circuit-on" button
			When the on-button is pressed, the circuit-on funciton is enabled: voltage on pin is LOGIC
			When the off-button is pressed, the circuit-on funciton is disabled: voltage on pin is GROUND

	- If V(bat) < 3.2: disallow circuit power, and turn off the ATtiny (this chip).
		Turn on interrupt on connected USB. This will reduce power as much as possible.

*/
ISR ( WDT_vect ) {

	//Disable sleep
	MCUCR &= ~(1<<SE);

	if((state>>USBCONNECTED) & 1){

		if((PINB>>PINB3) ^ 1){

			//Fool-proofing: check USB-connected pin to make sure that interrupt wasn't faulty
			//	This is done to make sure power conservation is activated properly
			state &= ~(1<<USBCONNECTED | 1<<CHARGING);
			PORTB &= ~((1<<PORTB0) | (1<<PORTB1));

		} else if((state>>CHARGING) & 1){
			
			//Blink signal light if the battery is charging
			PORTB ^= 1<<PORTB3;

		} else {

			//When charging is done, signal light should be constant on
			PORTB |= 1<<PORTB3;

		}		

	} else {

		//Turn of LED, and for safety (and because it doesn't cost anything)
		//	any charge pin settings
		PORTB &= !((1<<PORTB3) | (1<<PORTB1) | (1<<PORTB0));

		batteryVoltage = readVoltage();

		if(batteryVoltage < VLOWLIMIT){

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

	if((WDTCR>>WDCE) ^ 1){

		//Start watchdog if it is disabled
		startWatchdog();

	}

	if((PINB>>PINB3) & 1){

		state |= (1<<USBCONNECTED);

	} else {

		//Reset all charging logic
		state &= ~(1<<USBCONNECTED | 1<<CHARGING);
		PORTB &= ~((1<<PORTB0) | (1<<PORTB1));

		sleep();

	}

}