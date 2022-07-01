/* Snake implementation for "DMG" Game Boy, using GBDK-2020
   Made by tomas7770
   Tested on BGB 1.5.9, not tested on real hardware
*/

#include <gb/gb.h>
#include <stdint.h>
#include <stdbool.h>
#include <rand.h>
#include <gbdk/font.h>
#include <gb/bcd.h>

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
#define get_highscore_ptr() (high_scores + selected_difficulty + (wrap ? 5 : 0))

// spd values
const uint8_t spd_table[5] = {12, 10, 8, 6, 4};

// Game state
uint8_t state;
uint8_t plr_x, plr_y, plr_dir; // snake head position and direction
uint8_t food_x, food_y;
uint8_t score;
uint8_t spd; // snake moves every spd frames
bool move_lock; // if true, cannot change snake direction
bool wrap = true; // whether screen wrap is enabled or not

uint8_t selected_difficulty = 2; // selected difficulty in menu

uint16_t ordered_body_tiles[360]; // stores snake body tile indexes in order
                                  // of "creation"
uint16_t current_body_tile_i; // index of next element in the array above

uint8_t high_scores[10]; // 5 speed values * (screen wrap or not)

uint8_t joypad_status; // keys held (in this frame)
uint8_t joypad_lock = 255; // lock to check if key started being held in this frame;
                           // locks at the end of game loop if key held,
                           // unlocks if key not held; 1 is unlocked, 0 is locked

BCD bcd = MAKE_BCD(0);
uint8_t str_buf[9]; // string buffer for numbers
uint8_t* str_point; // variable pointer for buffer above

inline void tick();
inline void init_state_game();
void init_state_title();
void init_state_gameover();
void enable_scanline_isr();
void disable_scanline_isr();
inline void set_food_pos();
void update_wrap_text();
void update_highscore_text();
void draw_number(uint8_t number, uint8_t tile_x, uint8_t tile_y);

void main(void)
{
    // Initialize tile data
    font_init();
    font_load(font_min);
    set_bkg_data(37, 1, SnakeTiles);
    set_sprite_data(0, 2, SnakeTiles+16);

    set_interrupts(VBL_IFLAG | LCD_IFLAG);
    STAT_REG |= 0x40; // enable LYC=LY interrupt
    LYC_REG = 8;
    SHOW_BKG;
    SHOW_WIN;
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

// Disable window at scanline 8 to show play field
void scanline_isr() {
    HIDE_WIN;
}

// Game main loop
inline void tick()
{
    joypad_status = joypad();
    switch (state)
    {
        case STATE_GAME:
            // Re-enable window to display score
            SHOW_WIN;

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
                    init_state_gameover();
                    break;
                }

                // Delete an old position
                uint16_t* tile_i = ordered_body_tiles + current_body_tile_i;
                if (*tile_i != DUMMY_BODY_TILE) {
                    set_bkg_tile_xy((*tile_i)%20, (*tile_i)/20, TILE_EMPTY);
                }
                
                // Record new position
                uint16_t tile = get_plr_tile();
                if (get_bkg_tile_xy(plr_x >> 3, plr_y >> 3) == TILE_SNAKE) {
                    // Collision with body
                    init_state_gameover();
                    break;
                }
                set_bkg_tile_xy(plr_x >> 3, plr_y >> 3, TILE_SNAKE);
                *tile_i = tile;
                current_body_tile_i++;
                if (current_body_tile_i >= get_max_snake_size())
                    current_body_tile_i = 0;
                
                // Snake collisions with food
                if (plr_x == food_x && plr_y == food_y) {
                    score++;
                    draw_number(score, 6, 0);
                    set_food_pos();
                }
            }

            // Pause
            if (joypad_status & joypad_lock & J_START) {
                disable_scanline_isr();
                state = STATE_PAUSE;
            }
            break;

        case STATE_GAMEOVER:
            if (joypad_status & (J_A | J_START)) {
                // Save high score (if it's the case)
                uint8_t* high_score_i = get_highscore_ptr();
                if (score > *high_score_i)
                    *high_score_i = score;

                init_state_title();
            }
            break;

        case STATE_TITLE:
            if ((joypad_status & joypad_lock & J_UP) && selected_difficulty >= 1) {
                selected_difficulty--;
                move_sprite(1, 40, get_arrow_height());
                update_highscore_text();
            }
            else if ((joypad_status & joypad_lock & J_DOWN) && selected_difficulty <= 3) {
                selected_difficulty++;
                move_sprite(1, 40, get_arrow_height());
                update_highscore_text();
            }
            if (joypad_status & joypad_lock & J_SELECT) {
                wrap = !wrap;
                update_wrap_text();
                update_highscore_text();
            }
            if (joypad_status & joypad_lock & (J_A | J_START)) {
                init_state_game();
                state = STATE_GAME;
            }
            break;
            
        case STATE_PAUSE:
            if (joypad_status & joypad_lock & J_START) {
                enable_scanline_isr();
                state = STATE_GAME;
            }
            break;
    }
    joypad_lock = ~joypad_status;
}

inline void init_state_game()
{
    // Clear background
    fill_bkg_rect(0, 0, 20, 18, 0);

    // Reset ordered_body_tiles
    uint16_t* tile_i;
    for (tile_i = ordered_body_tiles; tile_i < ordered_body_tiles+360; tile_i++)
        *tile_i = DUMMY_BODY_TILE; // set to some unused value
    current_body_tile_i = 0;

    // Reset game variables
    plr_x = 80;
    plr_y = 64;
    plr_dir = DIR_LEFT;
    move_lock = false;
    score = 0;
    spd = spd_table[selected_difficulty];

    // Reset food position
    set_sprite_tile(1, TILE_FOOD);
    initrand(DIV_REG);
    set_food_pos();

    // HUD stuff
    set_win_tiles(0, 0, 20, 18, GameHUDMap);
    enable_scanline_isr();
}

void init_state_title()
{
    set_win_tiles(0, 0, 20, 18, TitleMap);
    set_sprite_tile(1, TILE_ARROW);
    move_sprite(1, 40, get_arrow_height());
    update_wrap_text();
    update_highscore_text();
    state = STATE_TITLE;
}

void init_state_gameover()
{
    set_win_tiles(0, 0, 20, 18, GameoverMap);
    move_sprite(1, 0, 0);
    draw_number(score, 11, 10);
    disable_scanline_isr();
    state = STATE_GAMEOVER;
}

void enable_scanline_isr() {
    disable_interrupts();
    add_LCD(scanline_isr);
    enable_interrupts();
}

void disable_scanline_isr() {
    SHOW_WIN;
    remove_LCD(scanline_isr);
}

// Set food to random position
inline void set_food_pos() {
    food_x = ((rand() % 19) << 3);
    food_y = (((rand() % 16)+1) << 3);
    move_sprite(1, food_x + 8, food_y + 16);
}

void update_wrap_text() {
    if (wrap) {
        // (O)n
        set_win_tile_xy(12, 15, 24);
        set_win_tile_xy(13, 15, 0);
    }
    else {
        // (O)ff
        set_win_tile_xy(12, 15, 16);
        set_win_tile_xy(13, 15, 16);
    }
}

void update_highscore_text() {
    uint8_t* high_score_i = get_highscore_ptr();
    draw_number(*high_score_i, 5, 0);
}

// Draws a number in the window
void draw_number(uint8_t number, uint8_t tile_x, uint8_t tile_y) {
    str_point = str_buf;
    uint2bcd((uint16_t) number, &bcd);
    bcd2text(&bcd, 1, str_point);
    str_point += 5; // only display last 3 digits
    while (*str_point != '\0') {
        set_win_tile_xy(tile_x, tile_y, *str_point);
        tile_x++;
        str_point++;
    }
}
