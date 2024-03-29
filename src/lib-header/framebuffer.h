#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include "stdtype.h"

#define MEMORY_FRAMEBUFFER (uint8_t *) 0xC00B8000
#define CURSOR_PORT_CMD    0x03D4
#define CURSOR_PORT_DATA   0x03D5
#define BUFFER_WIDTH 80
#define BUFFER_HEIGHT 25

#define EOF -1

/**
 * Terminal framebuffer
 * Resolution: 80x25
 * Starting at MEMORY_FRAMEBUFFER,
 * - Even number memory: Character, 8-bit
 * - Odd number memory:  Character color lower 4-bit, Background color upper 4-bit
*/

/**
 * Set framebuffer character and color with corresponding parameter values.
 * More details: https://en.wikipedia.org/wiki/BIOS_color_attributes
 *
 * @param row Vertical location (index start 0)
 * @param col Horizontal location (index start 0)
 * @param c   Character
 * @param fg  Foreground / Character color
 * @param bg  Background color
 */
void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg);

/**
 * Set cursor to specified location. Row and column starts from 0
 * 
 * @param r row
 * @param c column
*/
void framebuffer_set_cursor(uint8_t r, uint8_t c);

/** 
 * Set all cell in framebuffer character to 0x00 (empty character)
 * and color to 0x07 (gray character & black background)
 * 
 */
void framebuffer_clear(void);

/* get cursor position */
uint16_t framebuffer_get_cursor(void);

/* get row position of cursor */
uint8_t framebuffer_get_row(void);

/* get col position of cursor */
uint8_t framebuffer_get_col(void);

/* move cursor left */
void framebuffer_move_cursor_left(void);

/* move cursor right */
void framebuffer_move_cursor_right(void);

/* move cursor up */
void framebuffer_move_cursor_up(void);

/* move cursor down */
void framebuffer_move_cursor_down(void);

/* move cursor most left*/
void framebuffer_move_cursor_most_left(void);

/* move cursor most right*/
void framebuffer_move_cursor_most_right(void);

/* write current cursor */
void framebuffer_write_curCursor(char c, uint8_t fg, uint8_t bg);

void putchar(char character, uint32_t fg);

void puts(char* str, uint32_t len, uint32_t fg);

void show_file(char* buffer, uint32_t len_bound);

#endif