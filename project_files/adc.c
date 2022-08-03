#include <avr/io.h>
#include "adc.h"

void adc_init(void)
{
    // Initialize the ADC
	
	// Clears ADMUX register bit
	ADMUX &= ~(1 << REFS1);
	
	// Sets ADMUX register bits to 1
	ADMUX |=  (1 << REFS0);
	ADMUX |= (1 << ADLAR);
	
	// Sets ADCSRA register bits to 1 
    ADCSRA |= (1 << ADPS2);
    ADCSRA |= (1 << ADPS1);
    ADCSRA |= (1 << ADPS0);
    ADCSRA |= (1 << ADEN);
}

unsigned char adc_sample(unsigned char channel)
{
    // Set ADC input mux bits to 'channel' value
	ADMUX &= ~(0x0f);
	ADMUX |= channel;
	ADCSRA |= (1 << ADSC);
	
	// Loop that checks ADSC bit for conversion
	while ( ADCSRA & (1 << ADSC) ) {}

    // Convert an analog input and return the 8-bit result
	unsigned char bit_convert = ADCH;
	return bit_convert;
}
