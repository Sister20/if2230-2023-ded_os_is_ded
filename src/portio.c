#include "lib-header/stdtype.h"
#include "lib-header/portio.h"

/** x86 inb/outb:
 * @param dx target port 
 * @param al input/output byte
 */

void out(uint16_t port, uint8_t data) {
    __asm__(
        "outb %0, %1"
        : // <Empty output operand>
        : "a"(data), "Nd"(port)
    );
}

uint8_t in(uint16_t port) {
    uint8_t result;
    __asm__ volatile(
        "inb %1, %0" 
        : "=a"(result) 
        : "Nd"(port)
    );
    return result;
}

// uint16_t in16(uint16_t port){
//     uint8_t result;
//     __asm__ volatile(
//         "in %%ad, %%al"
//         : "=a"(result)
//         : "Nd"(port));
//     return result;
// }

// void out16(uint16_t port, uint8_t data) {
//     __asm__(
//         "out %%al, %%ad"
//         : // <Empty output operand>
//         : "a"(data), "Nd"(port)
//         );
// }