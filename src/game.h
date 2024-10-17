/*
 * game.h
 *
 * Authors: Jarrod Bennett, Cody Burnett, Bradley Stone, Yufeng Gao
 * Modified by: <YOUR NAME HERE>
 *
 * Function prototypes for game functions available externally. You may wish
 * to add extra function prototypes here to make other functions available to
 * other files.
 */

#ifndef GAME_H_
#define GAME_H_

#include <stdint.h>
#include <stdbool.h>

// Object definitions.
#define ROOM       	(0U << 0)
#define WALL       	(1U << 0)
#define BOX        	(1U << 1)
#define TARGET     	(1U << 2)
#define OBJECT_MASK	(ROOM | WALL | BOX | TARGET)

// Colour definitions.
#define COLOUR_PLAYER	(COLOUR_DARK_GREEN)
#define COLOUR_WALL  	(COLOUR_YELLOW)
#define COLOUR_BOX   	(COLOUR_ORANGE)
#define COLOUR_TARGET	(COLOUR_RED)
#define COLOUR_DONE  	(COLOUR_GREEN)


bool play_invalid_move_sound_flag;
bool play_box_on_target_sound_flag;
bool play_wall_hit_sound_flag;
bool play_player_moved_sound_flag;
bool on_top_of_target_flag;
uint8_t sound;

bool animation_running;
int32_t animation_ticks;
uint8_t target_row, target_col;
uint8_t target_color_state;


void play_invalid_move_sound(int order);
void play_box_on_target_sound(int order);
void play_player_moved_sound();
void flash_targets(void);
void play_tone(uint16_t frequency, uint16_t duration_ms);

void halt_animation(void);
/// <summary>
/// Initialises the game.
/// </summary>
void initialise_game(bool next_level);

/// <summary>
/// Moves the player based on row and column deltas.
/// </summary>
/// <param name="delta_row">The row delta.</param>
/// <param name="delta_col">The column delta.</param>
void move_player(int8_t delta_row, int8_t delta_col);


/// <summary>
/// Detects whether the game is over (i.e., current level solved).
/// </summary>
/// <returns>Whether the game is over.</returns>
void play_game_over_sound(int order);
bool is_game_over(void);
int get_steps(void);

/// <summary>
/// Flashes the player icon.
/// </summary>
void flash_player(void);

void reset_sound(void);

#endif /* GAME_H_ */
