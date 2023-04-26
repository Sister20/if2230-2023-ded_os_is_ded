#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    uint16_t cur_pos = r * BUFFER_WIDTH + c;
 
	out(CURSOR_PORT_CMD, 0x0F);
	out(CURSOR_PORT_DATA, (uint8_t) (cur_pos & 0xFF));
	out(CURSOR_PORT_CMD, 0x0E);
	out(CURSOR_PORT_DATA, (uint8_t) ((cur_pos >> 8) & 0xFF));
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    uint16_t style = (bg << 4) | (fg & 0xF);
    uint16_t * position;
    position = (uint16_t *) MEMORY_FRAMEBUFFER + (row * 80 + col);
    *position = c | (style << 8);
}

void framebuffer_clear(void) {
    memset(MEMORY_FRAMEBUFFER,0x00, BUFFER_WIDTH * BUFFER_HEIGHT * 2);
}

/* get cursor position */
uint16_t framebuffer_get_cursor(void){
    uint16_t pos = 0;
    out(CURSOR_PORT_CMD, 0x0F);
    pos |= in(0x3D5);
    out(CURSOR_PORT_CMD, 0x0E);
    pos |= ((uint16_t) in(0x3D5)) << 8;

    return pos;
}

/* get row position of cursor */
uint8_t framebuffer_get_row(void){
    uint16_t pos = framebuffer_get_cursor();
    return pos / BUFFER_WIDTH;
}

/* get col position of cursor */
uint8_t framebuffer_get_col(void){
    uint16_t pos = framebuffer_get_cursor();
    return pos % BUFFER_WIDTH;
}

/* move cursor left */
void framebuffer_move_cursor_left(void){
    if (framebuffer_get_col() > 0){
        framebuffer_set_cursor(framebuffer_get_row(), framebuffer_get_col()-1);
    } else {
        if (framebuffer_get_row() > 0){
            framebuffer_move_cursor_most_right();
            framebuffer_move_cursor_up();
        }
    }
}

/* move cursor right */
void framebuffer_move_cursor_right(void){
    if (framebuffer_get_col() < BUFFER_WIDTH-1){
        framebuffer_set_cursor(framebuffer_get_row(), framebuffer_get_col()+1);
    } else {
        if (framebuffer_get_row() < BUFFER_HEIGHT-1){
            framebuffer_move_cursor_most_left();
            framebuffer_move_cursor_down();
        }
    }
}

/* move cursor up */
void framebuffer_move_cursor_up(void){
    if (framebuffer_get_row() > 0){
        framebuffer_set_cursor(framebuffer_get_row()-1, framebuffer_get_col());
    }
}

/* move cursor down */
void framebuffer_move_cursor_down(void){
    if (framebuffer_get_row() < BUFFER_HEIGHT-1){
        framebuffer_set_cursor(framebuffer_get_row()+1, framebuffer_get_col());
    }
}

/* move cursor most left*/
void framebuffer_move_cursor_most_left(void){
    framebuffer_set_cursor(framebuffer_get_row(), 0);
}

/* move cursor most right*/
void framebuffer_move_cursor_most_right(void){
    framebuffer_set_cursor(framebuffer_get_row(), BUFFER_WIDTH-1);
}

/* write current cursor */
void framebuffer_write_curCursor(char c, uint8_t fg, uint8_t bg){
    framebuffer_write(framebuffer_get_row(), framebuffer_get_col(),
        c, fg, bg);
}

void puts(char* str, uint32_t len, uint32_t fg) {
    for (int i = 0; i < (int) len; i++) {
        if (str[i] == '\n') {
            framebuffer_move_cursor_down();
            framebuffer_move_cursor_most_left();
        } else {
            framebuffer_write_curCursor(str[i], fg, 0);
            framebuffer_move_cursor_right();
        }
    }
}