#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    uint16_t cur_pos = r * 80 + c;
 
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
    memset(MEMORY_FRAMEBUFFER, 0x00, 80*25*2);
}


