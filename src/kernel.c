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

// void kernel_setup(void) {
//     // uint32_t a;
//     // uint32_t volatile b = 0x0000BABE;
//     // __asm__("mov $0xCAFE0000, %0" : "=r"(a));
//     // while (TRUE) b += 1;

//     // // Testing Frame Buffer
//     // enter_protected_mode(&_gdt_gdtr);
//     // framebuffer_clear();
//     // framebuffer_write(3, 8,  'H', 0xF, 0);
//     // framebuffer_write(3, 9,  'a', 0xF, 0);
//     // framebuffer_write(3, 10, 'i', 0xF, 0);
//     // framebuffer_write(3, 11, '!', 0xF, 0);
//     // framebuffer_set_cursor(3, 10);
//     // while (TRUE);

//     //=========================== Milestone 2 =======================
//     // // Testing IDT and Interrupt
//     // enter_protected_mode(&_gdt_gdtr);
//     // pic_remap();
//     // initialize_idt();
//     // framebuffer_clear();
//     // // framebuffer_write(0,0, 'A', 0xF, 0);
//     // framebuffer_set_cursor(0, 0);
//     // __asm__("int $0x4");
//     // while (TRUE);

//     // // Testing Keyboard
//     // enter_protected_mode(&_gdt_gdtr);
//     // pic_remap();
//     // initialize_idt();
//     // framebuffer_clear();
//     // framebuffer_write(0,0, ' ', 0xF, 0);
//     // framebuffer_set_cursor(0,0);
//     // while (TRUE){
//     //     keyboard_state_activate();
//     // }


//     // File System 3.3.3
//     initialize_filesystem_fat32();
//     // struct FAT32DirectoryTable haha;
//     // init_directory_table(&haha, "huhu", 0);
//     while(TRUE){
//         keyboard_state_activate();
//     }
// }

//TESTING FILE SYSTEM
void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    keyboard_state_activate();

    struct ClusterBuffer cbuf[5];
    for (uint32_t i = 0; i < 5; i++)
        for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
            cbuf[i].buf[j] = i + 'a';

    struct FAT32DriverRequest request = {
        .buf                   = cbuf,
        .name                  = "ikanaide",
        .ext                   = "uwu",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0,
    } ;

    write(request);  // Create folder "ikanaide"
    memcpy(request.name, "kano1\0\0\0", 8);
    write(request);  // Create folder "kano1"
    memcpy(request.name, "ikanaide", 8);
    delete(request); // Delete first folder, thus creating hole in FS

    memcpy(request.name, "daijoubu", 8);
    request.buffer_size = 5*CLUSTER_SIZE;
    write(request);  // Create fragmented file "daijoubu"

    struct ClusterBuffer readcbuf;
    read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER+1, 1); 
    // If read properly, readcbuf should filled with 'a'

    request.buffer_size = CLUSTER_SIZE;
    read(request);   // Failed read due not enough buffer size
    request.buffer_size = 5*CLUSTER_SIZE;
    read(request);   // Success read on file "daijoubu"

    while (TRUE);
}


// kernel for testing filesystem
// void kernel_setup(void) {
//     enter_protected_mode(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     activate_keyboard_interrupt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     framebuffer_write(0,0, ' ', 0xF, 0);
//     initialize_filesystem_fat32();
//     keyboard_state_activate();

//     struct ClusterBuffer cbuf[5];
//     for (uint32_t i = 0; i < 5; i++)
//         for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
//             cbuf[i].buf[j] = i + 'a';

//     struct FAT32DriverRequest request = {
//         .buf                   = cbuf,
//         .name                  = "ikanaide",
//         .ext                   = "uwu",
//         .parent_cluster_number = ROOT_CLUSTER_NUMBER,
//         .buffer_size           = 0,
//     } ;

//     write(request);  // Create folder "ikanaide"
//     memcpy(request.name, "kano1\0\0\0", 8);
//     write(request);  // Create folder "kano1"
//     memcpy(request.name, "ikanaide", 8);
//     // delete(request); // Delete first folder, thus creating hole in FS

//     memcpy(request.name, "daijoubu", 8);
//     request.buffer_size = 5*CLUSTER_SIZE;
//     write(request);  // Create fragmented file "daijoubu"
//     // delete(request);
//     memcpy(request.name, "kano1\0\0\0", 8);
//     delete(request);

//     struct ClusterBuffer readcbuf;
//     read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER+1, 1);
//     // If read properly, readcbuf should filled with 'a'

//     memcpy(request.name, "daijoubu", 8);

//     uint8_t temp;
//     char * temp1 = request.buf;
//     char * temp2 = (char*) request.buf + CLUSTER_SIZE;
//     char * temp3 = (char*) request.buf + 2*CLUSTER_SIZE;
//     char * temp4 = (char*) request.buf + 3*CLUSTER_SIZE;

//     request.buffer_size = CLUSTER_SIZE;
//     temp = read(request);   // Failed read due not enough buffer size
//     request.buffer_size = 5*CLUSTER_SIZE;
//     temp =  read(request);   // Success read on file "daijoubu"
    

//     request.parent_cluster_number = ROOT_CLUSTER_NUMBER;
//     memcpy(request.name, "ikanaide", 8);
//     request.buffer_size = CLUSTER_SIZE;
//     temp = read_directory(request);
//     struct FAT32DirectoryTable* p = request.buf;

    
    
//     write_clusters(&temp, 11, 1);
//     write_clusters(temp1, 11, 1);
//     write_clusters(temp2, 11, 1);
//     write_clusters(temp3, 11, 1);
//     write_clusters(temp4, 11, 1);
//     write_clusters(p, 11, 1);


    // memcpy(request.name, "hehehehe", 8);
    // write(request);

    // request.buffer_size = CLUSTER_SIZE;
    // memcpy(request.name, "kano1\0\0\0", 8);
    // read_directory(request);
    // memcpy(request.name, "xixixixi", 8);
    // write(request);

//     while (TRUE);
// }
