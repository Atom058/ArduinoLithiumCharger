#ifndef HEADER_FILE
#define HEADER_FILE

//Includes
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//Function declarations
uint16_t readVoltage(void);
void setup(void);
void sleep(void);
void startWatchdog(void);
void stopWatchdog(void);

//Constants
#define BANDGAPDELAY 100 //Calculated as follows: 100us needed
	/*Voltages:
		- Using the 1.1V interal reference
		- Voltage divider of 43k + 12k = 0.218 ratio
		- 0V < Vsense < 5.04V, with a resolution of ~5mV (4.92mV) per step
		- Low battery is considered to be 3.2V, giving 650/1024
		- Full battery is considered to be 4.2V, giving 853/1024
		- Charge limit, i.e. level above which new charging will not start, is set at 4.0V, giving 812/1024
	*/
#define VLOWLVL (uint16_t) 650 //Voltage where battery is considered depleted
#define VHIGHLVL (uint16_t) 853 //Charge to this voltage - 4.15V
#define VCHARGELVL (uint16_t) 812 //Voltage above which charging will not initiate - 4.0V

//Booleans - Stored in state variable
#define CHARGING 0
#define BATTERYDEPLETED 1

//double define these to make code work
#define PRR _SFR_IO8(0x25)
#define PRADC 0
#define PRTIM0 1

/*PINS AND HOW THEY ARE USED
	pin 1: 
		RESET | when signal low - used for programming
	pin 2: INPUT
		PCINT3 | Pin-change interrupt
		PB3 | Digital IN
	pin 3: OUTPUT
		PB4 | Circuit enable
	pin 4: 
		GND
	pin 5: OUTPUT
		PB0 | Enable charging
	pin 6: OUTPUT
		PB1 | Toggle battery sense ciruitry
	pin 7: 	INPUT
		ADC1 | Measure battery voltage
		PB2 | Disabled
	pin 8: 
		VCC
*/

#endif