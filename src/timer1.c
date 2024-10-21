/*
 * timer1.c
 *
 * Author: Peter Sutton
 */

#include "timer1.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 8000000UL

void init_timer1(void)
{
    TCCR1A = 0; 
    TCCR1B = (1 << WGM12) | (1 << CS11); 
    DDRD |= (1 << PD2);
}
void set_sound_frequency(uint16_t frequency) {
    if (frequency == 0) {

        TCCR1A &= ~(1 << COM1A0);
        PORTD &= ~(1 << PD2);
        TIMSK1 &= ~(1 << OCIE1A);
    } else {
        OCR1A = (F_CPU / (2 * 8 * frequency)) - 1;
        TIMSK1 |= (1 << OCIE1A);
    }
}

ISR(TIMER1_COMPA_vect) {
    PORTD ^= (1 << PD2);
}