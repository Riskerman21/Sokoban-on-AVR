/*
 * project.c
 *
 * Authors: Peter Sutton, Luke Kamols, Jarrod Bennett, Cody Burnett,
 *          Bradley Stone, Yufeng Gao
 * Modified by: <YOUR NAME HERE>
 *
 * Main project event loop and entry point.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define F_CPU 8000000UL
#define max(a,b) ((a) > (b) ? (a) : (b))
#include <util/delay.h>

#include "game.h"
#include "startscrn.h"
#include "ledmatrix.h"
#include "buttons.h"
#include "serialio.h"
#include "terminalio.h"
#include "timer0.h"
#include "timer1.h"
#include "timer2.h"


// Function prototypes - these are defined below (after main()) in the order
// given here.
void initialise_hardware(void);
void start_screen(void);
void new_game(void);
void play_game(void);
void handle_game_over(void);


/////////////////////////////// main //////////////////////////////////

static uint32_t second_pased = 0;

bool set = true;
bool paused = false;
bool level_2 = false;
bool muted = false;




int main(void)
{
	// Setup hardware and callbacks. This will turn on interrupts.
	initialise_hardware();
	// Show the start screen. Returns when the player starts the game.
	start_screen();

	// Loop forever and continuously play the game.
	while (1)
	{
		new_game();
		play_game();
		handle_game_over();
	}
}

void initialise_hardware(void)
{
	init_ledmatrix();
	init_buttons();
	init_serial_stdio(19200, false);
	init_timer0();
	init_timer1();
	init_timer2();

	// Turn on global interrupts.
	sei();
	DDRC |= 0x3F;
}

void start_screen(void)
{
	// Hide terminal cursor and set display mode to default.
	
	hide_cursor();
	normal_display_mode();

	// Clear terminal screen and output the title ASCII art.
	clear_terminal();
	display_terminal_title(3, 5);
	move_terminal_cursor(11, 5);
	// Change this to your name and student number. Remember to remove the
	// chevrons - "<" and ">"!
	printf_P(PSTR("CSSE2010/7201 Project by Abdallah Azazy - 47994832"));

	// Setup the start screen on the LED matrix.
	setup_start_screen();

	// Clear button presses registered as the result of powering on the
	// I/O board. This is just to work around a minor limitation of the
	// hardware, and is only done here to ensure that the start screen is
	// not skipped when you power cycle the I/O board.
	clear_button_presses();

	// Wait until a button is pushed, or 's'/'S' is entered.
	while (1)
	{

		// Check for button presses. If any button is pressed, exit
		// the start screen by breaking out of this infinite loop.
		if (button_pushed() != NO_BUTTON_PUSHED)
		{
			break;
		}

		// No button was pressed, check if we have terminal inputs.
		if (serial_input_available())
		{
			// Terminal input is available, get the character.
			int serial_input = fgetc(stdin);

			// If the input is 's'/'S', exit the start screen by
			// breaking out of this loop.
			if (serial_input == 's' || serial_input == 'S')
			{
				break;
			} else if (serial_input == 'q' || serial_input == 'Q') {
				muted = !muted;
			}
			else if (serial_input == '2') {
				level_2 = true;
				break;
			}
		}

		// No button presses and no 's'/'S' typed into the terminal,
		// we will loop back and do the checks again. We also update
		// the start screen animation on the LED matrix here.
		update_start_screen();
	}
}


void new_game(void)
{
	// Clear the serial terminal.
	hide_cursor();
	clear_terminal();

	// Initialise the game and display.
	initialise_game(level_2);

	// Clear all button presses and serial inputs, so that potentially
	// buffered inputs aren't going to make it to the new game.
	clear_button_presses();
	clear_serial_input_buffer();

	move_terminal_cursor(5, 20); 
	clear_to_end_of_line();
	printf_P(PSTR("Time Elapsed: %ld s\n"), second_pased);
}



void play_game(void)
{
	uint32_t last_flash_time = get_current_time();
	uint32_t last_tflash_time = get_current_time();
	uint32_t last_second_time = get_current_time();
	uint32_t last_sound_time = get_current_time();
	uint32_t last_animation_time = get_current_time();

	
	DDRA = 0xFF;
	DDRC = 1;
	reset_sound();

	// We play the game until it's over.
	while (!is_game_over())
	{
		// We need to check if any buttons have been pushed, this will
		// be NO_BUTTON_PUSHED if no button has been pushed. If button
		// 0 has been pushed, we get BUTTON0_PUSHED, and likewise, if
		// button 1 has been pushed, we get BUTTON1_PUSHED, and so on.
		ButtonState btn = button_pushed();
		
		
		if (serial_input_available()) {
				int serial_input = fgetc(stdin);
				if (serial_input == 'W' || serial_input == 'w') {
					move_player(1, 0);
					flash_player();
				} else if (serial_input == 'D' || serial_input == 'd') {
					move_player(0, 1);
					flash_player();
				} else if (serial_input == 'A' || serial_input == 'a') {
					move_player(0, -1);
					flash_player();
				} else if (serial_input == 'S' || serial_input == 's') {
					move_player(-1, 0);
					flash_player();
				} else if (serial_input == 'P' || serial_input == 'p' ) {
					paused = !paused;
					if (paused) {
						move_terminal_cursor(6, 20); 
						printf_P(PSTR("Paused - press 'p'/'P' to continue"));
					} else {
						move_terminal_cursor(6, 20); 
						clear_to_end_of_line();
					}
					
					continue;
				} else if (serial_input == 'Q' || serial_input == 'q') {
					muted = !muted;
				} else     if (serial_input == 'z' || serial_input == 'Z') {
					//undo_last_move();
				}

			}
		
		if (set) {
			second_pased = 0;
			move_terminal_cursor(5, 20); 
			printf_P(PSTR("Time Elapsed: %ld s\n"), second_pased);
			set = false;
		}
		if (paused) {
			move_terminal_cursor(6, 20); 

			uint32_t current_time = get_current_time();
			if (current_time >= last_flash_time + 200)
			{
				last_flash_time += 200;

			}
			if (current_time >= last_second_time + 1000)
			{
				last_second_time += 1000;
			}

			if (serial_input_available()) {
				int serial_input = fgetc(stdin);
				if (serial_input == 'P' || serial_input == 'p' ) {
					paused = !paused;
					move_terminal_cursor(6, 20); 
					clear_to_end_of_line();
				}
			}

			continue;
		}


		if (btn == BUTTON0_PUSHED)
		{
			move_player(0, 1);
			flash_player();
		}
		
		if (btn == BUTTON1_PUSHED)
		{

			move_player(-1, 0);
			flash_player();
		}
		
		if (btn == BUTTON2_PUSHED)
		{
			move_player(1, 0);
			flash_player();
		}
		
		if (btn == BUTTON3_PUSHED)
		{
			move_player(0, -1);
			flash_player();
		}


		uint32_t current_time = get_current_time();
		if (current_time >= last_flash_time + 200)
		{
			flash_player();
			last_flash_time = current_time;

		}
		current_time = get_current_time();
		if (current_time >= last_tflash_time + 500)
		{
			flash_targets();
			last_tflash_time = current_time;

		}
		current_time = get_current_time();
		if (current_time >= last_second_time + 1000)
		{
			second_pased++;
			move_terminal_cursor(5, 20); 
			clear_to_end_of_line();
			printf_P(PSTR("Time Elapsed: %ld s\n"), second_pased);
			last_second_time = current_time;
		}
		current_time = get_current_time();
		
		if (current_time >= last_sound_time + 50)
		{
			play_tone(0, 0);
			if (!muted)
			{
				sound ++;
				
				if (play_invalid_move_sound_flag) {
					play_invalid_move_sound(sound);
				}
				else if (play_box_on_target_sound_flag) {
					play_box_on_target_sound(sound);

				} else if (play_player_moved_sound_flag){
					play_player_moved_sound();
					reset_sound();
				}
				last_sound_time = current_time;
				
			} else {
				reset_sound();
				sound = 0;
			}
		}
		current_time = get_current_time();
		if (current_time >= last_animation_time + 250) {
			if (animation_running){
				animation_ticks++;
				if (animation_ticks >= 9) {halt_animation(); continue;}
				printf_P(PSTR("animation %ld"), animation_ticks);
				last_sound_time = current_time;

				if (target_color_state == 0) {
					ledmatrix_update_pixel(target_row, target_col, COLOUR_BLACK);
					target_color_state = 1;
				} else if (target_color_state == 0){
					ledmatrix_update_pixel(target_row, target_col, COLOUR_BOX);
					target_color_state = 2;
				} else{
					ledmatrix_update_pixel(target_row, target_col, COLOUR_DONE);
					target_color_state = 0;
				}
				
			}
		}
	}
	

	
	// We get here if the game is over.
}

void handle_game_over(void)
{
	uint32_t time_elapsed = second_pased;  
	uint16_t score;
	uint16_t S = get_steps();  
	uint32_t T = time_elapsed; 

	score = (max(200 - S, 0) * 20) + max(1200 - T, 0);
	

	move_terminal_cursor(10, 5);
	printf_P(PSTR("Congratulations! You've completed the level.\n"));
	move_terminal_cursor(12, 5);
	printf_P(PSTR("Steps: %u\n"), S);
	move_terminal_cursor(13, 5);
	printf_P(PSTR("Time: %lu seconds\n"), T);
	move_terminal_cursor(14, 5);
	printf_P(PSTR("Score: %u\n"), score);
	
	move_terminal_cursor(16, 5);
	printf_P(PSTR("GAME OVER"));
	move_terminal_cursor(17, 5);
	printf_P(PSTR("Press 'r'/'R' to restart, 'e'/'E' to exit, or 'n'/'N' to play the next level"));

	uint32_t counter = 0;
	uint32_t last_sound_time = get_current_time();
	uint32_t current_time = get_current_time();

	while (1)
	{
		

		
		current_time = get_current_time();

		if (current_time >= last_sound_time + 100 && !muted)
		{
			counter++;
			play_tone(0, 0);
			play_game_over_sound(counter);
			last_sound_time = current_time;
		}


        int serial_input = -1;
        if (serial_input_available())
        {
            serial_input = fgetc(stdin);
        }

        // Check serial input.
        if (toupper(serial_input) == 'R')
        {
            second_pased = 0;
            return; 
        }
        else if (toupper(serial_input) == 'E')
        {
            second_pased = 0;
			initialise_hardware();
			start_screen();
            return;
        } else if (serial_input == 'q' || serial_input == 'Q') {
				muted = !muted;
		}
		else if (toupper(serial_input) == 'N' && level_2 == false) {
            second_pased = 0;
			level_2 = true;
			return;
		}
		else if (toupper(serial_input) == 'N' && level_2 == true) {
            second_pased = 0;
			level_2 = false;
			return;
		}
	}
}