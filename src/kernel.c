#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "lib-header/idt.h"
#include "lib-header/interrupt.h"
#include "lib-header/keyboard.h"
#include "lib-header/disk.h"
#include "lib-header/fat32.h"

void kernel_setup(void) {
    // uint32_t a;
    // uint32_t volatile b = 0x0000BABE;
    // __asm__("mov $0xCAFE0000, %0" : "=r"(a));
    // while (TRUE) b += 1;

    // // Testing Frame Buffer
    // enter_protected_mode(&_gdt_gdtr);
    // framebuffer_clear();
    // framebuffer_write(3, 8,  'H', 0xF, 0);
    // framebuffer_write(3, 9,  'a', 0xF, 0);
    // framebuffer_write(3, 10, 'i', 0xF, 0);
    // framebuffer_write(3, 11, '!', 0xF, 0);
    // framebuffer_set_cursor(3, 10);
    // while (TRUE);

    //=========================== Milestone 2 =======================
    // // Testing IDT and Interrupt
    // enter_protected_mode(&_gdt_gdtr);
    // pic_remap();
    // initialize_idt();
    // framebuffer_clear();
    // // framebuffer_write(0,0, 'A', 0xF, 0);
    // framebuffer_set_cursor(0, 0);
    // __asm__("int $0x4");
    // while (TRUE);

    // Testing Keyboard
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    framebuffer_write(0,0, ' ', 0xF, 0);
    framebuffer_set_cursor(0,0);
    initialize_filesystem_fat32();
    struct FAT32DirectoryTable haha;
    init_directory_table(&haha, "huhu", 0);
    while (TRUE){
        keyboard_state_activate();
    }

}







