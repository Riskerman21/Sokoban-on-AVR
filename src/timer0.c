#include "timer0.h"
#include "game.h"
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

static volatile uint32_t clock_ticks_ms;
int digit;
uint8_t value;
uint32_t time = 0;
uint8_t seven_seg[10] = { 63, 6, 91, 79, 102, 109, 125, 7, 127, 111 };

#define DIGIT_PIN PD3

void display_digit(uint8_t number, uint8_t digit) 
{
    PORTD &= ~(1 << DIGIT_PIN);
    PORTC = seven_seg[number];
    if (digit == 0) {
        PORTD &= ~(1 << DIGIT_PIN);
    } else {
        PORTD |= (1 << DIGIT_PIN);
    }
}

void seven_seg_update(void) {
    if (digit == 0) {
        value = get_steps() % 10;
    } else {
        value = (get_steps() / 10) % 10;
    }
    display_digit(value, digit);
    digit = 1 - digit;
}

void init_timer0(void)
{
    DDRD |= (1 << DIGIT_PIN);  // Set Pin D2 as output for digit switching
    clock_ticks_ms = 0L;       // Reset clock tick count

    // Set up timer 0 to generate an interrupt every 1ms
    TCNT0 = 0;                 // Clear the timer
    OCR0A = 124;               // Output compare value for 1ms

    // Set the timer to clear on compare match (CTC mode) and divide the clock by 64
    TCCR0A = (1 << WGM01);
    TCCR0B = (1 << CS01) | (1 << CS00);

    // Enable an interrupt on output compare match
    TIMSK0 |= (1 << OCIE0A);
    TIFR0 = (1 << OCF0A);      // Clear the interrupt flag
}

uint32_t get_current_time(void)
{
    uint8_t interrupts_were_enabled = bit_is_set(SREG, SREG_I);
    cli();
    uint32_t result = clock_ticks_ms;
    if (interrupts_were_enabled) {
        sei();
    }
    return result;
}

ISR(TIMER0_COMPA_vect)
{
    clock_ticks_ms++;
    if (clock_ticks_ms >= time + 5) {
        seven_seg_update();
        time = clock_ticks_ms;
    }
}