/********************************************
Author: Imtiaz Uddin
 ********************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "lcd.h"
#include "adc.h"
#include "ds18b20.h"
#include "encoder.h"

// Serial communications functions and variables
void serial_init(unsigned short);
void serial_stringout(char *);
void serial_txchar(char);
void variable_delay_us(int);
void init_timer1(unsigned short m);				// Signature for timer function

// ADC functions and variables
#define ADC_CHAN 0              // Buttons use ADC channel 0

/*
  The note_freq array contains the frequencies of the notes from
  C3 to C5 in array locations 0 to 24.  Used calculate the timer delay
  and for the length of the time the note is played.
*/

#define NUM_TONES 25

unsigned int note_freq[NUM_TONES] =
{ 131, 139, 147, 156, 165, 176, 185, 196, 208, 220, 233, 247,
  262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523};
  
char *notes[NUM_TONES] = 
{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "C", 
"C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", "C"};

volatile char ch;								// Created to get recieved character
volatile char buf2[17];							// Character array of sufficient size
volatile char x = 0;							// Created to check characters in a message
volatile char flag = 0;							// Flag to show message received and display message
volatile char buf1[17];							// Character array of sufficient size
// **buf2 is used in last ISR
volatile char buf3[17];							// Character array of sufficient size
volatile char buf4[17];							// Character array of sufficient size
volatile char buf5[17];							// Character array of sufficient size
volatile unsigned char new_state, old_state;	// States
volatile unsigned char changed = 0;  			// Flag for state change
volatile int count;								// Count to display
volatile unsigned char a, b;
volatile char input_bit;						// Created to read input bits simultaneously
volatile char low_thresh = 60;					// Low temperature threshold 
volatile char high_thresh = 100;				// High temperature threshold 
volatile char low_temp ;						// Low temperature character variable 
volatile char high_temp;						// High temperature character variable
volatile int low = 0;							// Low set button configuration
volatile int high = 0;							// High set button configuration
volatile int mode = 2;							// Set mode to something invalid to initiate
volatile unsigned int temperature;				// Temperature displayed on LCD
unsigned char t[2];								// Character Array for temperature
volatile char *status = "OK";					// To determine temp status
volatile char *octave = "3";					// To print number of note


// Timer function initializing registers from 109 slides
void init_timer1(unsigned short m)
{
	TCCR1B |= (1 << WGM12);
    TIMSK1 |= (1 << OCIE1A);
    OCR1A = m;
    TCCR1B |= (1 << CS12);
}

int main(void) {
	// Initialize the LCD, ADC, DS18B20, Rotary Encoder, and Serial Modules
	lcd_init();							// LCD
	adc_init();							// ADC
	ds_init();							// DS18B20
	rot_init();							// Rotary Encoder
	init_timer1(31250);					// Calls timer function
	
	if (ds_init() == 0) {    			// Initialize the DS18B20
         // Sensor not responding
		 lcd_stringout("Sensor Fail");
    }
	
	/* Initialize DDR registers */
	DDRC |= (1 << PC3);							// DS18B20
	DDRD |= (1 << PD2) | (1 << PD3);			// LEDs	
	DDRC |= (1 << PC5);							// Buzzer	
		
	/* Initialize PORT registers */
	PORTC |= (1 << PC1) | (1 << PC2);			// Buttons	
	PORTB |= (1 << PB3) | (1 << PB4);			// Rotary Encoder Pull-ups
	
	/* Initialize Interrupts */
	PCICR |= (1 << PCIE0) | (1 << PCIE1);		// Enable pin change interrupts PORTS B and C
	PCMSK0 |= (1 << PCINT3) | (1 << PCINT4);	// Enable interrupts for PB3 and PB4
	PCMSK1 |= (1 << PCINT9) | (1 << PCINT10);	// Enable interrupts for PC1 and PC2
	TCCR0A |= (1 << WGM01);
	TIMSK0 |= (1 << OCIE0A); 
	sei();										// Enable global interrupts
	
	// Write a spash screen to the LCD
	lcd_writecommand(1); 						// Clears LCD
	lcd_moveto(0, 2); 							// Centers cursor on 1st row
	lcd_stringout("Mini Thermometer"); 			// Print out project name
	lcd_moveto(1, 2); 							// Centers cursor on 2nd row
	lcd_stringout("Hello Student"); 			// Prints out welcome message
	_delay_ms(1000); 							// Delays 1 second
	lcd_writecommand(1);						// Clears LCD
	
    ds_convert();    							// Start first temperature conversion
	
	unsigned input_bit = PINB;			// Created to read input bits simultaneously
	
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
	
	int integ = 0;
	int frac = 0;
	
	t[0] = t[1] = 0;
	temperature = 0;
	
	while (1) {                 // Loop forever
		if (ds_temp(t)) {    	// True if conversion complete
			// Process the values returned in t[0] and t[1] to find the temperature.
			temperature = ( (t[1] << 8) + t[0] );
			
			int res = ((temperature*9) /8) + 320;
			
			integ = res / 10;
			frac = res % 10;
			
			ds_convert();   	// Start next conversion
        }
		
		// Encoder controlled temperature 
		if ( (eeprom_read_byte((void *) 100) >= 60 ) && (eeprom_read_byte((void *) 100) <= 100 ) )
			low_temp = eeprom_read_byte((void *) 100);
		
		// Tone index 
		if ( (eeprom_read_byte((void *) 200) >= 0 ) & (eeprom_read_byte((void *) 200) <= 25 ) )
			high_temp = eeprom_read_byte((void *) 200);
		
		lcd_moveto(1,0)	;						// Sets cursor on 1st block on 2nd row
		snprintf(buf3, 17, "%3d", low_temp);	// Create a string with encoder temp	
		lcd_stringout(buf3); 					// Print out encoder temperature
		
		lcd_moveto(0, 0); 								// Sets cursor on 1st block on 1st row
		snprintf(buf1, 17, "%2d.%1d", integ, frac);		// Create a string with room temp
		lcd_stringout(buf1); 							// Room temperature	
		
		lcd_moveto(0, 12); 								// Sets cursor on 13th block on 1st row
		snprintf(buf4, 17, "%3s", notes[high_temp]);	// Create string with note
		lcd_stringout(buf4); 							// Print Note
		
		if (high_temp <= 11) octave = "3";
		else if ((high_temp >= 12) && (high_temp <= 23)) octave = "4";
		else octave = "5";
		
		lcd_moveto(1, 14); 						// Sets cursor on 15th block on 2nd row
		snprintf(buf5, 17, "%s", octave);		// Create string with note
		lcd_stringout(buf5); 					// Print Octave
		
		// Green LED
		if (integ <= low_temp) { // Cool
			TCCR1B &= ~(1 << WGM12);
			TCCR1B &= ~(1 << CS12);
			PORTD &= ~(1 << PD2);
			PORTD |= (1 << PD3);
			lcd_moveto(0, 7); 					// Sets cursor on 8th block on 1st row	
			lcd_stringout("OK  "); 			
		}
		// Red LED  
		else if ((integ == low_temp + 1) || (integ == low_temp)) { // Warm
			TCCR1B |= (1 << WGM12);
			TCCR1B |= (1 << CS12);
			PORTD &= ~(1 << PD3);
			lcd_moveto(0, 7); 					// Sets cursor on 8th block on 1st row
			lcd_stringout("WARM");
		} 
		else { // Too Hot
			TCCR1B &= ~(1 << WGM12);
			TCCR1B &= ~(1 << CS12);
			PORTD &= ~(1 << PD3);
			PORTD |= (1 << PD2);
			lcd_moveto(0, 7); 					// Sets cursor on 8th block on 1st row		
			lcd_stringout("HOT ");
			count = note_freq[high_temp];
			TCCR0A |= (1 << CS12);
			OCR0A = (16000000*((1/count)/2))/256;
		}				
		
		// If button for temperature is clicked
		if ( (PINC & (1 << PC2)) == 0 ) {					// Low set button
			mode = 0;										// Mode for temperature
			lcd_writecommand(1); 							// Clears LCD
			lcd_moveto(0, 0); 								// Sets cursor on 1st block on 1st row
			lcd_stringout(buf1); 							// Room temperature
			lcd_moveto(1, 0); 								// Sets cursor on 1st block on 2nd row	
			lcd_stringout(buf3); 							// Encoder temperature
			lcd_moveto(1, 4);								// Move cursor to 5th block, 1nd row			
			lcd_stringout("<");								// Prints "<"
			lcd_moveto(0, 15); 								// Sets cursor on 16th block on 1st row
			lcd_stringout(buf4); 							// Print Note
			lcd_moveto(1, 14); 								// Sets cursor on 15th block on 2nd row
			lcd_stringout(buf5); 							// Print Octave
		}
		
		// If button for note is clicked 
		if ( (PINC & (1 << PC1)) == 0 ) {					// Low set button
			mode = 1;										// Mode for note 
			lcd_writecommand(1); 							// Clears LCD
			lcd_moveto(0, 0); 								// Sets cursor on 1st block on 1st row
			lcd_stringout(buf1); 							// Room temperature
			lcd_moveto(1, 0); 								// Sets cursor on 1st block on 2nd row	
			lcd_stringout(buf3); 							// Encoder temperature
			lcd_moveto(1, 12);								// Move cursor to 13th block, 1nd row			
			lcd_stringout(">");								// Prints "<"	
			lcd_moveto(0, 15); 								// Sets cursor on 16th block on 1st row
			lcd_stringout(buf4); 							// Print Note
			lcd_moveto(1, 14); 								// Sets cursor on 15th block on 2nd row
			lcd_stringout(buf5); 							// Print Octave
		}
		
	}
}

ISR(TIMER0_COMPA_vect) {
	//output flip
	PINC ^= (1 << PC5);
	
	//decrement count
	count--; 
	
	if (count == 0) { 
	TCCR0B &= ~((1 << CS02) | (1 << CS00));
	PINC &= ~(1 << PC5);
	}
}


ISR(TIMER1_COMPA_vect) {
	PORTD ^= (1 << PD2);
}


ISR(PCINT1_vect) {
	if ( (PINC & (1 << PC1)) == 0 ) {		// Low set button
		low = 0;							// Reset low to 0
		high = 1;							// Set high to 1
	}
	else if ( (PINC & (1 << PC2)) == 0 ) {	// High set button 
		low = 1;							// Set low to 1
		high = 0;							// Reset high to 0
	}
}