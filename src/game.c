/*
 * game.c
 *
 * Authors: Jarrod Bennett, Cody Burnett, Bradley Stone, Yufeng Gao
 * Modified by: <YOUR NAME HERE>
 *
 * Game logic and state handler.
 */ 

#include "game.h"
#include "timer2.h"
#include "timer1.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "ledmatrix.h"
#include "terminalio.h"
#include <util/delay.h>


// ========================== NOTE ABOUT MODULARITY ==========================

// The functions and global variables defined with the static keyword can
// only be accessed by this source file. If you wish to access them in
// another C file, you can remove the static keyword, and define them with
// the extern keyword in the other C file (or a header file included by the
// other C file). While not assessed, it is suggested that you develop the
// project with modularity in mind. Exposing internal variables and functions
// to other .C files reduces modularity.


// ============================ GLOBAL VARIABLES =============================

// The game board, which is dynamically constructed by initialise_game() and
// updated throughout the game. The 0th element of this array represents the
// bottom row, and the 7th element of this array represents the top row.
static uint8_t board[MATRIX_NUM_ROWS][MATRIX_NUM_COLUMNS];

// The location of the player.
static uint8_t player_row;
static uint8_t player_col;
static uint8_t Changed_player_row;
static uint8_t Changed_player_col;
static uint8_t step_taken;
#define BUZZER_PIN PD2


bool play_player_moved_sound_flag = false;
bool play_invalid_move_sound_flag = false;
bool play_box_on_target_sound_flag = false;
bool play_wall_hit_sound_flag = false;

bool animation_running = false;
int32_t animation_ticks = 0;
uint8_t target_row, target_col;
uint8_t target_color_state = 0;

int get_steps(void) {
	return step_taken;
} 

void play_tone(uint16_t frequency, uint16_t duration_ms) {
    set_sound_frequency(frequency);
	
}

void play_box_on_target_sound(int order) {
	switch (order)
	{
	case 1:
		play_tone(523, 40); 
		break;
	case 2:
		play_tone(587, 40);
		break;
	case 3:
		play_tone(659, 40);
		break;
	case 4:
		play_tone(784, 40);
		reset_sound();
		break;
	default:
		break;
	}
}

void play_player_moved_sound() {
	play_tone(523, 25);
	reset_sound();
}

void play_invalid_move_sound(int order) {
		switch (order)
	{
	case 1:
		play_tone(400, 50);
		break;

	case 2:
		play_tone(400, 50);
		reset_sound();
		break;

	default:
		break;
	}
}

void play_game_over_sound(int order) {
    switch (order) {
        case 0:
			play_tone(1046, 50);  // C6
            break;
        case 1:
            break;
        case 2:
			play_tone(1175, 50);  // D6
            break;
        case 3:
            break;
        case 4:
			play_tone(1319, 50);  // E6
            break;
		case 5:
            break;
        case 6:
			play_tone(1568, 50);  // G6
            break;
        case 7:
            break;
        case 8:
			play_tone(1046, 50);  // C6 again	
            break;
        case 9:
            break;
        default:
            break;
    }
	
}
        
void animate_target(uint8_t row, uint8_t col) {
    target_row = row;
    target_col = col;
    animation_ticks = 0;
    animation_running = true;
    target_color_state = 0;

}
void halt_animation(void) {
    animation_running = false;
	animation_ticks = 0;
	target_color_state = 0;
    ledmatrix_update_pixel(target_row, target_col, COLOUR_DONE);
}


// A flag for keeping track of whether the player is currently visible.
static bool player_visible;
static bool targets_visible;


static const char* wall_messages[] = {
    "There is a wall there",
    "You've got be kidding me, that's a wall",
    "Cannot go through walls!"
};


// ========================== GAME LOGIC FUNCTIONS ===========================


void update_square(uint8_t row, uint8_t col) {
    uint8_t terminal_row = 10 + (MATRIX_NUM_ROWS - 1 - row) * 2;
    uint8_t terminal_col = 5 + col * 4; 

    move_terminal_cursor(terminal_row, terminal_col);

    uint8_t object = board[row][col];

    if (object == WALL) {
        set_display_attribute(BG_WHITE);
    } else if (object == BOX) {
        set_display_attribute(BG_YELLOW);
    } else if (object == TARGET) {
        set_display_attribute(BG_RED);
    } else if (object == (BOX | TARGET)) {
        set_display_attribute(BG_CYAN);   
    } else if (row == player_row && col == player_col) {
        set_display_attribute(BG_GREEN);  
    } else {
        normal_display_mode(); 
    }

    putchar(' ');
    putchar(' ');

    normal_display_mode();
}
void reset_sound() {
	play_player_moved_sound_flag = false;
	play_invalid_move_sound_flag = false;
	play_box_on_target_sound_flag = false;
	play_wall_hit_sound_flag = false;
	sound = 0;
}
static void paint_square(uint8_t row, uint8_t col)
{
	uint8_t terminal_row = 10 + (MATRIX_NUM_ROWS - 1 - row) * 2;
    uint8_t terminal_col = 5 + col * 4; 

    move_terminal_cursor(terminal_row, terminal_col);

    uint8_t object = board[row][col];

    if (object == WALL) {
        set_display_attribute(BG_WHITE);
    } else if (object == BOX) {
        set_display_attribute(BG_YELLOW);
    } else if (object == TARGET) {
        set_display_attribute(BG_RED);
    } else if (object == (BOX | TARGET)) {
        set_display_attribute(BG_CYAN);
    } else if (row == player_row && col == player_col) {
        set_display_attribute(BG_GREEN);
    } else {
        normal_display_mode(); 
    }

    putchar(' ');
    putchar(' ');

    normal_display_mode();

	switch (board[row][col] & OBJECT_MASK)
	{
		case ROOM:
			ledmatrix_update_pixel(row, col, COLOUR_BLACK);
			break;
		case WALL:
			ledmatrix_update_pixel(row, col, COLOUR_WALL);
			break;
		case BOX:
			ledmatrix_update_pixel(row, col, COLOUR_BOX);
			break;
		case TARGET:
			ledmatrix_update_pixel(row, col, COLOUR_TARGET);
			break;
		case BOX | TARGET:
			ledmatrix_update_pixel(row, col, COLOUR_DONE);
			break;
		default:
			break;
	}
}

// This function initialises the global variables used to store the game
// state, and renders the initial game display.
void initialise_game(bool next_level)
{
	// Short definitions of game objects used temporarily for constructing
	// an easier-to-visualise game layout.
	#define _	(ROOM)
	#define W	(WALL)
	#define T	(TARGET)
	#define B	(BOX)
	move_terminal_cursor(2, 40); 
	step_taken = 0;

	if (!next_level) printf_P(PSTR("%s\n"), "AVR Sokoban - Level 1");
	else printf_P(PSTR("%s\n"), "AVR Sokoban - Level 2");


	// The starting layout of level 1. In this array, the top row is the
	// 0th row, and the bottom row is the 7th row. This makes it visually
	// identical to how the pixels are oriented on the LED matrix, however
	// the LED matrix treats row 0 as the bottom row and row 7 as the top
	// row.
	static const uint8_t lv1_layout[MATRIX_NUM_ROWS][MATRIX_NUM_COLUMNS] =
	{
		{ _, W, _, W, W, W, _, W, W, W, _, _, W, W, W, W },
		{ _, W, T, W, _, _, W, T, _, B, _, _, _, _, T, W },
		{ _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _ },
		{ W, _, B, _, _, _, _, W, _, _, B, _, _, B, _, W },
		{ W, _, _, _, W, _, B, _, _, _, _, _, _, _, _, _ },
		{ _, _, _, _, _, _, T, _, _, _, _, _, _, _, _, _ },
		{ _, _, _, W, W, W, W, W, W, T, _, _, _, _, _, W },
		{ W, W, _, _, _, _, _, _, W, W, _, _, W, W, W, W }
	};
		static const uint8_t lv2_layout[MATRIX_NUM_ROWS][MATRIX_NUM_COLUMNS] =
	{
		{ _, _, W, W, W, W, _, _, W, W, _, _, _, _, _, W },
		{ _, _, W, _, _, W, _, W, W, _, _, _, _, B, _, _ },
		{ _, _, W, _, B, W, W, W, _, _, T, W, _, T, W, W },
		{ _, _, W, _, _, _, _, T, _, _, B, W, W, W, _, _ },
		{ W, W, W, W, _, W, _, _, _, _, _, W, _, W, W, _ },
		{ W, T, B, _, _, _, _, B, _, _, _, W, W, _, W, W },
		{ W, _, _, _, T, _, _, _, _, _, _, B, T, _, _, _ },
		{ W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W }
	};
	// Undefine the short game object names defined above, so that you
	// cannot use use them in your own code. Use of single-letter names/
	// constants is never a good idea.
	#undef _
	#undef W
	#undef T
	#undef B

	// Set the initial player location (for level 1).
	if (!next_level) {
		player_row = 5;
		player_col = 2;
	} else {
		player_row = 6;
		player_col = 15;
	}

	// Make the player icon initially invisible.
	player_visible = false;
	targets_visible = false;
	// Copy the starting layout (level 1 map) to the board array, and flip
	// all the rows.
	for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++)
	{
		for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++)
		{
			if (!next_level) {
				board[MATRIX_NUM_ROWS - 1 - row][col] =
					lv1_layout[row][col];
			} else {
				board[MATRIX_NUM_ROWS - 1 - row][col] =
					lv2_layout[row][col];
			}
		}
	}

	// Draw the game board (map).
	for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++)
	{
		for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++)
		{
			paint_square(row, col);
		}
	}
	
}
bool on_top_of_target_flag = false;

void flash_targets(void) {

    targets_visible = !targets_visible;  // Toggle visibility

    for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++) {
            if (board[row][col] == TARGET && !(row == player_row && col == player_col)) {
                if (targets_visible) {
                    ledmatrix_update_pixel(row, col, COLOUR_RED); 
                } else {
                    ledmatrix_update_pixel(row, col, COLOUR_BLACK);
                }

                // For terminal display
                move_terminal_cursor(10 + (MATRIX_NUM_ROWS - 1 - row) * 2, 5 + col * 4);
                if (targets_visible) {
                    set_display_attribute(BG_RED);
                } else {
                    set_display_attribute(BG_BLACK);
                }
                putchar(' ');
                putchar(' ');
                normal_display_mode();
            } else if (board[row][col] == (BOX | TARGET)) {
                ledmatrix_update_pixel(row, col, COLOUR_DONE); 
                move_terminal_cursor(10 + (MATRIX_NUM_ROWS - 1 - row) * 2, 5 + col * 4);
                set_display_attribute(BG_CYAN); 
                putchar(' ');
                putchar(' ');
                normal_display_mode();
            }
        }
    }
}
// This function flashes the player icon. If the icon is currently visible, it
// is set to not visible and removed from the display. If the player icon is
// currently not visible, it is set to visible and rendered on the display.
// The static global variable "player_visible" indicates whether the player
// icon is currently visible.
void flash_player(void)
{
    player_visible = !player_visible;
    
    if (player_visible)
    {
		//led
        ledmatrix_update_pixel(player_row, player_col, COLOUR_PLAYER);
		//terminal
        move_terminal_cursor(10 + (MATRIX_NUM_ROWS - 1 - player_row) * 2, 5 + player_col * 4);
        set_display_attribute(BG_GREEN); 
        putchar(' ');  
        putchar(' ');
        normal_display_mode();
    }
    else
    {

        paint_square(player_row, player_col);

        uint8_t object = board[player_row][player_col];
        if (object == ROOM)
        {
            move_terminal_cursor(10 + (MATRIX_NUM_ROWS - 1 - player_row) * 2, 5 + player_col * 4);
            set_display_attribute(BG_BLACK);
            putchar(' ');
            putchar(' ');
            normal_display_mode();
        }
        else
        {
            update_square(player_row, player_col);  
        }
    }
}


void display_hit_wall_message(void) {
    int random_index = rand() % 3;
    move_terminal_cursor(3, 20); 
	clear_to_end_of_line();
    printf_P(PSTR("%s\n"), wall_messages[random_index]);
	reset_sound();
	play_invalid_move_sound_flag = true;
}



// This function handles player movements.
void move_player(int8_t delta_row, int8_t delta_col)
{
	//                    Implementation Suggestions
	//                    ==========================
	//
	//    Below are some suggestions for how to implement the first few
	//    features. These are only suggestions, you are absolutely not
	//   required to follow them if you know what you're doing, they're
	//     just here to help you get started. The suggestions for the
	//       earlier features are more detailed than the later ones.
	//
	// +-----------------------------------------------------------------+
	// |            Move Player with Push Buttons/Terminal               |
	// +-----------------------------------------------------------------+
	// | 1. Remove the display of the player icon from the current       |
	// |    location.                                                    |
	// |      - You may find the function flash_player() useful.         |
	// | 2. Calculate the new location of the player.                    |
	// |      - You may find creating a function for this useful.        |
	// | 3. Update the player location (player_row and player_col).      |
	// | 4. Draw the player icon at the new player location.             |
	// |      - Once again, you may find the function flash_player()     |
	// |        useful.                                                  |
	// | 5. Reset the icon flash cycle in the caller function (i.e.,     |
	// |    play_game()).                                                |
	// +-----------------------------------------------------------------+
	//
	// +-----------------------------------------------------------------+
	// |                      Game Logic - Walls                         |
	// +-----------------------------------------------------------------+
	// | 1. Modify this function to return a flag/boolean for indicating |
	// |    move validity - you do not want to reset icon flash cycle on |
	// |    invalid moves.                                               |
	// | 2. Modify this function to check if there is a wall at the      |
	// |    target location.                                             |
	// | 3. If the target location contains a wall, print one of your 3  |
	// |    'hit wall' messages and return a value indicating an invalid |
	// |    move.                                                        |
	// | 4. Otherwise make the move, clear the message area of the       |
	// |    terminal and return a value indicating a valid move.         |
	// +-----------------------------------------------------------------+
	//
	// +-----------------------------------------------------------------+
	// |                      Game Logic - Boxes                         |
	// +-----------------------------------------------------------------+
	// | 1. Modify this function to check if there is a box at the       |
	// |    target location.                                             |
	// | 2. If the target location contains a box, see if it can be      |
	// |    pushed. If not, print a message and return a value           |
	// |    indicating an invalid move.                                  |
	// | 3. Otherwise push the box and move the player, then clear the   |
	// |    message area of the terminal and return a valid indicating a |
	// |    valid move.                                                  |
	// +-----------------------------------------------------------------+

	    paint_square(player_row, player_col);

    uint8_t old_row = player_row;  // Save the current row before moving
    uint8_t old_col = player_col;  // Save the current col before moving

    Changed_player_row = (player_row + delta_row + MATRIX_NUM_ROWS) % MATRIX_NUM_ROWS;
    Changed_player_col = (player_col + delta_col + MATRIX_NUM_COLUMNS) % MATRIX_NUM_COLUMNS;

    if (board[Changed_player_row][Changed_player_col] == WALL) {
        display_hit_wall_message();
        return;
    } else if (board[Changed_player_row][Changed_player_col] == BOX || board[Changed_player_row][Changed_player_col] == (BOX | TARGET)) {
        uint8_t box_behind_row = (Changed_player_row + delta_row + MATRIX_NUM_ROWS) % MATRIX_NUM_ROWS;
        uint8_t box_behind_col = (Changed_player_col + delta_col + MATRIX_NUM_COLUMNS) % MATRIX_NUM_COLUMNS;

        if (board[box_behind_row][box_behind_col] == WALL) {
            move_terminal_cursor(3, 20); 
            clear_to_end_of_line();
            printf_P(PSTR("Unless you got a drill, I Cannot push box onto wall"));
            reset_sound();
            play_invalid_move_sound_flag = true;
            return;

        } else if (board[box_behind_row][box_behind_col] == BOX || board[box_behind_row][box_behind_col] == (BOX | TARGET)) {
            move_terminal_cursor(3, 20); 
            clear_to_end_of_line();
            printf_P(PSTR("That's too heavy, Cannot stack boxes"));
            reset_sound();
            play_invalid_move_sound_flag = true;
            return;

        } else if (board[Changed_player_row][Changed_player_col] == (BOX | TARGET)) {
            // Box is on target and is now being moved off
            halt_animation();  // Stop animation as box is moved off target

            board[box_behind_row][box_behind_col] = BOX;
            board[Changed_player_row][Changed_player_col] = TARGET;
            paint_square(Changed_player_row, Changed_player_col);
            paint_square(box_behind_row, box_behind_col);
            player_row = Changed_player_row;
            player_col = Changed_player_col;
            move_terminal_cursor(3, 20); 
            clear_to_end_of_line();
            printf_P(PSTR("Box moved off target"));
            step_taken++;
            update_square(old_row, old_col); 
            update_square(player_row, player_col);
            reset_sound();
            play_player_moved_sound_flag = true;
            return;

        } else if (board[box_behind_row][box_behind_col] == TARGET) {
            // Box is moved onto a new target
            board[box_behind_row][box_behind_col] = BOX | TARGET;
            board[Changed_player_row][Changed_player_col] = (board[Changed_player_row][Changed_player_col] == (BOX | TARGET)) ? TARGET : ROOM;
            paint_square(Changed_player_row, Changed_player_col);
            paint_square(box_behind_row, box_behind_col);
            player_row = Changed_player_row;
            player_col = Changed_player_col;
            move_terminal_cursor(3, 20); 
            clear_to_end_of_line();
            printf_P(PSTR("Box moved onto target"));
            animate_target(box_behind_row, box_behind_col);
            step_taken++;
            update_square(old_row, old_col); 
            update_square(player_row, player_col); 
            reset_sound();
            play_box_on_target_sound_flag = true;
            return;

        } else {
            board[box_behind_row][box_behind_col] = BOX;
            board[Changed_player_row][Changed_player_col] = (board[Changed_player_row][Changed_player_col] == (BOX | TARGET)) ? TARGET : ROOM;
            paint_square(Changed_player_row, Changed_player_col);
            paint_square(box_behind_row, box_behind_col);
            player_row = Changed_player_row;
            player_col = Changed_player_col;
        }
    } else if (board[Changed_player_row][Changed_player_col] == TARGET) {
        player_row = Changed_player_row;
        player_col = Changed_player_col;
    } else {
        player_row = Changed_player_row;
        player_col = Changed_player_col;
    }
    move_terminal_cursor(3, 20); 
    clear_to_end_of_line();
    step_taken++;
    update_square(old_row, old_col); 
    update_square(player_row, player_col); 
    reset_sound();
    play_player_moved_sound_flag = true;
	
}
void clear_game_board(void) {
    uint8_t terminal_start_row = 10; 
    uint8_t terminal_start_col = 5;  

    for (int8_t row = MATRIX_NUM_ROWS - 1; row >= 0; row--) {
        for (uint8_t repeat = 0; repeat < 2; repeat++) {  
            move_terminal_cursor(terminal_start_row + (MATRIX_NUM_ROWS - 1 - row) * 2 + repeat, terminal_start_col);
            clear_to_end_of_line();
        }
    }
	
}
// This function checks if the game is over (i.e., the level is solved), and
// returns true iff (if and only if) the game is over.
bool is_game_over(void)
{
    for (uint8_t row = 0; row < MATRIX_NUM_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_NUM_COLUMNS; col++) {
            if ((board[row][col] & TARGET) && !(board[row][col] & BOX)) {
				
                return false; 
            }
        }
    }
	player_visible = false;
	paint_square(player_row, player_col);
	flash_targets();
	move_terminal_cursor(2, 40); 
	clear_to_end_of_line();
	halt_animation();

	clear_game_board();
    return true;
}
