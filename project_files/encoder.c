#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "lcd.h"
#include "adc.h"
#include "ds18b20.h"
#include "encoder.h"

void rot_init() {
	/* Initialize DDR registers */
	DDRC |= (1 << PC3);							// DS18B20
	DDRD |= (1 << PD2) | (1 << PD3);			// LEDs	
		
	/* Initialize PORT registers */
	PORTC |= (1 << PC1) | (1 << PC2);			// Buttons	
	PORTB |= (1 << PB3) | (1 << PB4);			// Rotary Encoder Pull-ups
	
	/* Initialize Interrupts */
	PCICR |= (1 << PCIE0) | (1 << PCIE1);		// Enable pin change interrupts PORTS B and C
	PCMSK0 |= (1 << PCINT3) | (1 << PCINT4);	// Enable interrupts for PB3 and PB4
	PCMSK1 |= (1 << PCINT9) | (1 << PCINT10);	// Enable interrupts for PC1 and PC2
	sei();										// Enable global interrupts
	
	// Read the A and B inputs to determine the intial state
    // Warning: Do NOT read A and B separately.  You should read BOTH inputs
    // at the same time, then determine the A and B values from that value.    
	input_bit = PINB;					// Created to read input bits simultaneously
	
	if ( input_bit & (1 << PB3) )		// First Input bit	
		a = 1;							// A determined
	else
		a = 0;							// A determined
	
	if ( input_bit & (1 << PB4) )		// Second Input bit
		b = 1;							// B determined
	else
		b = 0;							// B determined

    if (!b && !a)
	old_state = 0;
    else if (!b && a)
	old_state = 1;
    else if (b && !a)
	old_state = 2;
    else
	old_state = 3;

    new_state = old_state;
}

/*
	Deals with changes in state (and technically count)
	
	***Now also deals with the EEPROM***
*/
ISR(PCINT0_vect) {
	/*
	lcd_moveto(0, 7); 	
	snprintf(buf5, 17, "%1d", mode);	
	lcd_stringout(buf5);
	*/
	// Read the input bits and determine A and B
	input_bit = PINB;					// Created to read input bits simultaneously
	
	if ( input_bit & (1 << PB3) )		// First Input bit	
		a = 1;							// A determined
	else
		a = 0;							// A determined
	
	if ( input_bit & (1 << PB4) )		// Second Input bit
		b = 1;							// B determined
	else
		b = 0;							// B determined

	// For each state, examine the two input bits to see if state
	// has changed, and if so set "new_state" to the new state,
	// and adjust the count value.
	if (old_state == 0) {
	    // Handle A and B inputs for state 0
		if (a && !b) {
			new_state = 1;		// Update state		
			count++;			// Update count
		
			if (mode == 0) {
				if (low_temp < 100) low_temp++;
				eeprom_update_byte( (void *) 100, low_temp );
			}
			else if (mode == 1) {
				if (high_temp < 24) high_temp++;
				eeprom_update_byte( (void *) 200, high_temp);
			}
		}
		
		if (b && !a) {
			new_state = 2;		// Update state
			count--;			// Update count
			
			if (mode == 0) {
				if (low_temp == 100) low_temp = 99;
				else if (low_temp > 60) low_temp--;
				eeprom_update_byte( (void *) 100, low_temp );
			}
			else if (mode == 1) {
				if (high_temp == 24) high_temp = 23;
				else if (high_temp > 0) high_temp--;
				eeprom_update_byte( (void *) 200, high_temp );
			}
		}
	}
	else if (old_state == 1) {
	    // Handle A and B inputs for state 1
		if (!a && !b) {
			new_state = 0;		// Update state
			count--;			// Update count
			
			if (mode == 0) {
				if (low_temp == 100) low_temp = 99;
				else if (low_temp > 60) low_temp--;
				eeprom_update_byte( (void *) 100, low_temp );
			}
			else if (mode == 1) {
				if (high_temp == 24) high_temp = 23;
				else if (high_temp > 0) high_temp--;
				eeprom_update_byte( (void *) 200, high_temp );
			}
		}
		
		if (b && a) {
			new_state = 3;		// Update state
			count++;			// Update count
			
			if (mode == 0) {
				if (low_temp < 100) low_temp++;
				eeprom_update_byte( (void *) 100, low_temp );
			}
			else if (mode == 1) {
				if (high_temp < 24) high_temp++;
				eeprom_update_byte( (void *) 200, high_temp );
			}
		}
	}
	else if (old_state == 2) {
	    // Handle A and B inputs for state 2
		if (a && b) {
			new_state = 3;		// Update state
			count--;			// Update count
			
			if (mode == 0) {
				if (low_temp == 100) low_temp = 99;
				else if (low_temp > 60) low_temp--;
				eeprom_update_byte( (void *) 100, low_temp );
			}
			else if (mode == 1) {
				if (high_temp == 24) high_temp = 23;
				else if (high_temp > 0) high_temp--;
				eeprom_update_byte( (void *) 200, high_temp );
			}
		}
		
		if (!b && !a) {
			new_state = 0;		// Update state
			count++;			// Update count
			
			if (mode == 0) {
				if (low_temp < 100) low_temp++;
				eeprom_update_byte( (void *) 100, low_temp );
			}
			else if (mode == 1) {
				if (high_temp < 24) high_temp++;
				eeprom_update_byte( (void *) 200, high_temp );
			}
		}
	}
	else {   // old_state = 3
	    // Handle A and B inputs for state 3
		if (!a && b) {
			new_state = 2;		// Update state
			count++;			// Update count
			
			if (mode == 0) {
				if (low_temp < 100) low_temp++;
				eeprom_update_byte( (void *) 100, low_temp );
			}
			else if (mode == 1) {
				if (high_temp < 24) high_temp++;
				eeprom_update_byte( (void *) 200, high_temp );
			}
		}
		
		if (!b && a) {
			new_state = 1;		// Update state
			count--;			// Update count
			
			if (mode == 0) {
				if (low_temp == 100) low_temp = 99;
				else if (low_temp > 60) low_temp--;
				eeprom_update_byte( (void *) 100, low_temp );
			}
			else if (mode == 1) {
				if (high_temp == 24) high_temp = 23;
				else if (high_temp > 0) high_temp--;
				eeprom_update_byte( (void *) 200, high_temp );
			}
		}
	}

	// If state changed, update the value of old_state,
	// and set a flag that the state has changed.
	if (new_state != old_state) {
	    changed = 1;
	    old_state = new_state;
	}
}