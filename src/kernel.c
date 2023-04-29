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
#include "lib-header/paging.h"

/*======================= MILESTONE 3 ============================*/

void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    gdt_install_tss();
    set_tss_register();

    // Allocate first 4 MiB virtual memory
    allocate_single_user_page_frame((uint8_t*) 0);

    // Write shell into memory
    struct FAT32DriverRequest request = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0x100000,
    };
    read(request);
    
    // struct FAT32DriverRequest request2 = {
    //     .buf                   = (uint8_t*) "Lorem ipsum dolor sit amet, consectetur adipiscing elit, \n",
    //     .name                  = "lorem",
    //     .ext                   = "txt",
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    //     .buffer_size           = 1*CLUSTER_SIZE,
    // };
    // write(request2);

    // struct FAT32DriverRequest request3 = {
    //     .buf                   = (uint8_t*) "segs bebas",
    //     .name                  = "segs",
    //     .ext                   = "dir",
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    //     .buffer_size           = 0,
    // };
    // write(request3);

    // struct FAT32DriverRequest request4 = {
    //     .buf                   = (uint8_t*) "segs",
    //     .name                  = "sugar",
    //     .ext                   = "dir",
    //     .parent_cluster_number = 5,
    //     .buffer_size           = 0,
    // };
    // write(request4);

    // insert_index("zipdig\0\0", "txt", 3);
    // insert_index("bento\0\0\0", "txt", 3);
    // insert_index("brobro\0\0", "txt",3);
    // insert_index("askadas\0", "txt",3);
    // insert_index("kafkasqu", "txt",3);
    // insert_index("zipdig\0\0", "txt",3);

    // delete_index("zipdig\0\0", "txt");
    // delete_index("bento\0\0\0", "txt");
    // delete_index("brobro\0\0", "txt");
    // delete_index("askadas\0", "txt");
    // delete_index("kafkasqu", "txt");
    // delete_index("zipdig\0\0", "txt");






    // Set TSS $esp pointer and jump into shell 
    set_tss_kernel_current_stack();
    kernel_execute_user_program((uint8_t*) 0);

    while (TRUE);
}