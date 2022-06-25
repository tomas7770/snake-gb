#include <gb/gb.h>
#include <stdint.h>
#include <stdbool.h>
#include <gbdk/font.h>

#include "tiles.h"
#include "tilemaps.h"

#define STATE_GAME 0
#define STATE_GAMEOVER 1
#define STATE_TITLE 2
#define STATE_PAUSE 3

#define DIR_LEFT 0
#define DIR_UP 1
#define DIR_RIGHT 2
#define DIR_DOWN 3

// Background tiles
#define TILE_EMPTY 0
#define TILE_SNAKE 37

// Sprite tiles
#define TILE_FOOD 0
#define TILE_ARROW 1

#define DUMMY_BODY_TILE 65535

#define get_plr_tile() ( (uint16_t) (plr_y >> 1) * 5 + (plr_x >> 3) ) // (plr_y/8)*20 = plr_y*(5/2)
#define get_max_snake_size() (score + 3)
#define get_arrow_height() (64 + (selected_difficulty << 3))

// spd values
const uint8_t spd_table[5] = {12, 10, 8, 6, 4};

// Game state
uint8_t state = STATE_TITLE;
uint8_t plr_x, plr_y, plr_dir; // snake head position and direction
uint8_t food_x, food_y;
uint8_t score;
uint8_t spd; // snake moves every spd frames
bool move_lock; // if true, cannot change snake direction
bool wrap; // whether screen wrap is enabled or not

uint8_t selected_difficulty; // selected difficulty in menu

uint8_t body_tiles[360]; // tiles occupied by snake body (160/8 * 144/8),
                         // i.e. tile map with empty tiles or snake tiles

uint16_t ordered_body_tiles[360]; // stores snake body tile indexes in order
                                  // of "creation"
uint16_t current_body_tile_i; // index of next element in the array above

uint8_t high_scores[12]; // 6 speed values * (screen wrap or not)

uint8_t joypad_status; // keys held (in this frame)
uint8_t joypad_lock = 255; // lock to check if key started being held in this frame;
                           // locks at the end of game loop if key held,
                           // unlocks if key not held; 1 is unlocked, 0 is locked

inline void tick();
inline void init_state_game();
void init_state_title();

void main(void)
{
    // Initialize tile data
    font_init();
    font_load(font_min);
    set_bkg_data(37, 1, SnakeTiles);
    set_sprite_data(0, 2, SnakeTiles+16);

    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;

    init_state_title();

    while(1) {
		// Game main loop processing
        tick();

		// Done processing, yield CPU and wait for start of next frame
        wait_vbl_done();
    }
}

// Game main loop
inline void tick()
{
    joypad_status = joypad();
    switch (state)
    {
        case STATE_GAME:
            // Input
            if (!move_lock) {
                if (plr_dir == DIR_LEFT || plr_dir == DIR_RIGHT) {
                    if (joypad_status & J_UP) {
                        plr_dir = DIR_UP;
                        move_lock = true;
                    }
                    else if (joypad_status & J_DOWN) {
                        plr_dir = DIR_DOWN;
                        move_lock = true;
                    }
                }
            }
            if (!move_lock) {
                if (plr_dir == DIR_UP || plr_dir == DIR_DOWN) {
                    if (joypad_status & J_LEFT) {
                        plr_dir = DIR_LEFT;
                        move_lock = true;
                    }
                    else if (joypad_status & J_RIGHT) {
                        plr_dir = DIR_RIGHT;
                        move_lock = true;
                    }
                }
            }

            if (!(sys_time % spd)) {
                // Movement
                switch (plr_dir)
                {
                    case DIR_UP:
                        plr_y -= 8;
                        break;
                    case DIR_DOWN:
                        plr_y += 8;
                        break;
                    case DIR_LEFT:
                        plr_x -= 8;
                        break;
                    case DIR_RIGHT:
                        plr_x += 8;
                        break;
                }
                move_lock = false;

                // Wrapping/border collision
                if (wrap) {
                    if (plr_x == 248) // unsigned 248 == -8
                        plr_x = 152;
                    else if (plr_x == 160)
                        plr_x = 0;

                    if (plr_y == 0)
                        plr_y = 136;
                    else if (plr_y == 144)
                        plr_y = 8;
                }
                else if (plr_x == 248 || plr_x == 160 || plr_y == 0 || plr_y == 144) {
                    state = STATE_GAMEOVER;
                    break;
                }

                // Delete an old position
                uint16_t* tile_i = ordered_body_tiles + current_body_tile_i;
                if (*tile_i != DUMMY_BODY_TILE) {
                    body_tiles[*tile_i] = TILE_EMPTY;
                    set_bkg_tile_xy((*tile_i)%20, (*tile_i)/20, TILE_EMPTY);
                }
                
                // Record new position
                uint16_t tile = get_plr_tile();
                body_tiles[tile] = TILE_SNAKE;
                set_bkg_tile_xy(plr_x >> 3, plr_y >> 3, TILE_SNAKE);
                *tile_i = tile;
                current_body_tile_i++;
                if (current_body_tile_i >= get_max_snake_size())
                    current_body_tile_i = 0;
            }
            break;

        case STATE_GAMEOVER:
            break;

        case STATE_TITLE:
            if ((joypad_status & joypad_lock & J_UP) && selected_difficulty >= 1) {
                selected_difficulty--;
                move_sprite(1, 40, get_arrow_height());
            }
            else if ((joypad_status & joypad_lock & J_DOWN) && selected_difficulty <= 3) {
                selected_difficulty++;
                move_sprite(1, 40, get_arrow_height());
            }
            if (joypad_status & (J_A | J_START)) {
                init_state_game();
                state = STATE_GAME;
            }
            break;
            
        case STATE_PAUSE:
            break;
    }
    joypad_lock = ~joypad_status;
}

inline void init_state_game()
{
    set_bkg_tiles(0, 0, 20, 18, body_tiles);
    uint16_t* tile_i;
    for (tile_i = ordered_body_tiles; tile_i < ordered_body_tiles+360; tile_i++)
        *tile_i = DUMMY_BODY_TILE; // set to some unused value
    plr_x = 80;
    plr_y = 64;
    plr_dir = DIR_LEFT;
    move_lock = false;
    score = 0;
    spd = spd_table[selected_difficulty];
    wrap = true;
    set_sprite_tile(1, TILE_FOOD);
}

void init_state_title()
{
    set_bkg_tiles(0, 0, 20, 18, TitleMap);
    set_sprite_tile(1, TILE_ARROW);
    selected_difficulty = 2;
    move_sprite(1, 40, get_arrow_height());
}
