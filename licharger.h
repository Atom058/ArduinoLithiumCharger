#ifndef HEADER_FILE
#define HEADER_FILE

//Includes
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay_basic.h>

//Function declarations
uint16_t readVoltage(void);
void setup(void);
void sleep(void);
void startWatchdog(void);
void stopWatchdog(void);

//Constants
#define VLOWLIMIT 500 //Voltage where battery is considered depleted - 3.2V
#define VHIGHLIMIT 1000 //Charge to this voltage - 4.15V
#define VCVLIMIT 850 //Voltage limit where charging switches from constant current to constant voltage.
#define VCHARGELIMIT 850 //Voltage above which charging will not initiate - 4.0V
#define BANDGAPDELAY 50 //Calculated as follows: Takes three CPU cycles per loop, and 100us needed

//Booleans - Stored in state variable
#define USBCONNECTED 1
#define CHARGING 2
#define BATTERYDEPLETED 3

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

#endif