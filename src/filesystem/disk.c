#include "lib-header/disk.h"
#include "lib-header/portio.h"

static void ATA_busy_wait() {
    while (in(0x1F7) & ATA_STATUS_BSY);
}

static void ATA_DRQ_wait() {
    while (!(in(0x1F7) & ATA_STATUS_RDY));
}

/**
 * ATA PIO logical block address read blocks. Will blocking until read is completed.
 * Note: ATA PIO will use 2-bytes per read/write operation.
 * Recommended to use struct BlockBuffer
 * 
 * @param ptr                   Pointer for storing reading data, this pointer should point to already allocated memory location.
 *                              With allocated size positive integer multiple of BLOCK_SIZE, ex: buf[1024]
 * @param logical_block_address Block address to read data from. Use LBA addressing
 * @param block_count           How many block to read, starting from block logical_block_address to lba-1
 */
void read_blocks(void *ptr, uint32_t logical_block_address, uint8_t block_count) {
    ATA_busy_wait();
    out(0x1F6, 0xE0 | ((logical_block_address >> 24) & 0xF));
    out(0x1F2, block_count);
    out(0x1F3, (uint8_t) logical_block_address);
    out(0x1F4, (uint8_t) (logical_block_address >> 8));
    out(0x1F5, (uint8_t) (logical_block_address >> 16));
    out(0x1F7, 0x20);

    uint16_t *target = (uint16_t*) ptr;

    for (uint32_t i = 0; i < block_count; i++) {
        ATA_busy_wait();
        ATA_DRQ_wait();
        for (uint32_t j = 0; j < HALF_BLOCK_SIZE; j++)
            target[j] = in16(0x1F0);
        // Note : uint16_t => 2 bytes, HALF_BLOCK_SIZE*2 = BLOCK_SIZE with pointer arithmetic
        target += HALF_BLOCK_SIZE;
    }
}

/**
 * ATA PIO logical block address write blocks. Will blocking until write is completed.
 * Note: ATA PIO will use 2-bytes per read/write operation.
 * Recommended to use struct BlockBuffer
 *
 * @param ptr                   Pointer to data that to be written into disk. Memory pointed should be positive integer multiple of BLOCK_SIZE
 * @param logical_block_address Block address to write data into. Use LBA addressing
 * @param block_count           How many block to write, starting from block logical_block_address to lba-1
 */
void write_blocks(const void *ptr, uint32_t logical_block_address, uint8_t block_count) {
    ATA_busy_wait();
    out(0x1F6, 0xE0 | ((logical_block_address >> 24) & 0xF));
    out(0x1F2, block_count);
    out(0x1F3, (uint8_t) logical_block_address);
    out(0x1F4, (uint8_t) (logical_block_address >> 8));
    out(0x1F5, (uint8_t) (logical_block_address >> 16));
    out(0x1F7, 0x30);

    for (uint32_t i = 0; i < block_count; i++) {
        ATA_busy_wait();
        ATA_DRQ_wait();
        /* Note : uint16_t => 2 bytes, i is current block number to write
           HALF_BLOCK_SIZE*i = block_offset with pointer arithmetic
        */
        for (uint32_t j = 0; j < HALF_BLOCK_SIZE; j++)
            out16(0x1F0, ((uint16_t*) ptr)[HALF_BLOCK_SIZE*i + j]);
    }
}
