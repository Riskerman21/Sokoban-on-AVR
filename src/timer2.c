/*
 * timer2.c
 *
 * Author: Peter Sutton
 */

#include "timer2.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "ledmatrix.h"
#include "terminalio.h"

// volatile bool animation_running = false;
// volatile uint8_t animation_ticks = 0;
// volatile uint8_t target_row, target_col;
// volatile uint8_t target_color_state = 0;

void init_timer2(void) {
    // TCCR2A = (1 << WGM21);
    // TCCR2B = (1 << CS20) | (1 << CS21);
    // OCR2A = 300;  
    // TIMSK2 = (1 << OCIE2A);
}

// ISR(TIMER2_COMPA_vect) {
//     if (animation_running) {
//         animation_ticks++;
//         if (animation_ticks >= 9) {
//             halt_animation();
//         } else {
//             printf_P(PSTR("animation"));
//             if (target_color_state == 0) {
//                 ledmatrix_update_pixel(target_row, target_col, COLOUR_BLACK);
//                 target_color_state = 1;
//             } else if (target_color_state == 0){
//                 ledmatrix_update_pixel(target_row, target_col, COLOUR_BOX);
//                 target_color_state = 2;
//             } else{
//                 ledmatrix_update_pixel(target_row, target_col, COLOUR_DONE);
//                 target_color_state = 0;
//             }
//         }
//     }

// }

// void animate_target(uint8_t row, uint8_t col) {
//     // target_row = row;
//     // target_col = col;
//     // animation_ticks = 0;
//     // animation_running = true;
//     // target_color_state = 0;

// }

// void halt_animation(void) {
//     // animation_running = false;
//     // ledmatrix_update_pixel(target_row, target_col, COLOUR_DONE);
//     // TIMSK2 &= ~(1 << OCIE2A); 
// }