#include "../lib-header/keyboard.h"
#include "../lib-header/portio.h"
#include "../lib-header/framebuffer.h"
#include "../lib-header/stdmem.h"

const char keyboard_scancode_1_to_ascii_map[256] = {
      0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',  'o', 'p', '[',  ']', '\n',   0,  'a',  's',
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',  'z', 'x',  'c',  'v',
    'b',  'n', 'm', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,

      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

static struct KeyboardDriverState keyboard_state = {
    .read_extended_mode= FALSE,
    .keyboard_input_on = FALSE,
    .buffer_index = 0
};

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void){
    activate_keyboard_interrupt();
    keyboard_state.keyboard_input_on = TRUE;
}

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void){
    keyboard_state.keyboard_input_on = FALSE;
}

// Get keyboard buffer values - @param buf Pointer to char buffer, recommended size at least KEYBOARD_BUFFER_SIZE
void get_keyboard_buffer(char *buf){
    for (int i = 0; i < KEYBOARD_BUFFER_SIZE; i++){
        buf[i] = keyboard_state.keyboard_buffer[i];
    }
}

// Check whether keyboard ISR is active or not - @return Equal with keyboard_input_on value
bool is_keyboard_blocking(void){
    return keyboard_state.keyboard_input_on;
}

void keyboard_isr(void) {
    if (!keyboard_state.keyboard_input_on){
        keyboard_state.buffer_index = 0;
    }
    else {
        uint8_t  scancode    = in(KEYBOARD_DATA_PORT);
        char     mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
        // TODO : Implement scancode processing
        if (mapped_char != '\0'){
            if (mapped_char == '\n'){
                keyboard_state_deactivate();
                if (framebuffer_get_row() < BUFFER_HEIGHT-1){
                    framebuffer_move_cursor_down();
                    framebuffer_move_cursor_most_left();
                    framebuffer_write_curCursor(' ', 0xF, 0);
                }
            }
            else if (mapped_char == '\b'){
                if (framebuffer_get_row() > 0 || framebuffer_get_col() > 0){
                    // keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = '\0';
                    // keyboard_state.buffer_index--;
                    framebuffer_write_curCursor('\0', 0xF,0);
                    framebuffer_move_cursor_left();
                    framebuffer_write_curCursor(' ', 0xF, 0);
                }
            } 
            else if (mapped_char != '\b' && mapped_char != '\n'){
                // keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = mapped_char;
                // keyboard_state.buffer_index++;
                framebuffer_write_curCursor(mapped_char, 0xF, 0);
                framebuffer_move_cursor_right();
                framebuffer_write_curCursor(' ', 0xF, 0);
            }

            // for (int i = 0; i < keyboard_state.buffer_index; i++){
            //     framebuffer_write(framebuffer_get_row(), i, 
            //         keyboard_state.keyboard_buffer[i], 0xF, 0
            //         );
            // }
            // framebuffer_set_cursor(framebuffer_get_row(), keyboard_state.buffer_index);
            // framebuffer_write_curCursor(' ', 0xF, 0);
        }
        pic_ack(IRQ_KEYBOARD);
    }
}

