#include "lib-header/stdtype.h"
#include "lib-header/gdt.h"
#include "lib-header/interrupt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to GDT definition in Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {   // Null Descriptor
            // TODO : Implement
            0,                              // segment_low
            0,                              // base_low
            0,                              // base_mid
            0,                              // type_bit
            0,                              // non_system - S
            0,                              // privilege_level - DPL
            0,                              // segment_present - P
            0,                              // segment_lim 
            0,                              // available_sys_soft - AVL
            0,                              // bit_code_segment - L
            0,                              // def_operation - D/B
            0,                              // granularity - G
            0                               // base_high

        },
        {   // Kernel Code Descriptor
            // TODO : Implement
            (uint16_t) 0xFFFFF,             // segment_low
            0,                              // base_low
            0,                              // base_mid
            0xA,                            // type_bit
            1,                              // non_system - S
            0,                              // privilege_level - DPL
            1,                              // segment_present - P
            0,                              // segment_lim 
            0,                              // available_sys_soft - AVL
            0,                              // bit_code_segment - L
            1,                              // def_operation - D/B
            1,                              // granularity - G
            0                               // base_high
        },
        {   // Kernel Data Descriptor
            // TODO : Implement
            (uint16_t) 0xFFFFF,             // segment_low
            0,                              // base_low
            0,                              // base_mid
            0x2,                            // type_bit
            1,                              // non_system - S
            0,                              // privilege_level - DPL
            1,                              // segment_present - P
            0,                              // segment_lim 
            0,                              // available_sys_soft - AVL
            0,                              // bit_code_segment - L
            1,                              // def_operation - D/B
            1,                              // granularity - G
            0                               // base_high
        },
        {   // User Code Descriptor
            // TODO : Implement
            (uint16_t) 0xFFFFF,             // segment_low
            0,                              // base_low
            0,                              // base_mid
            0xA,                            // type_bit
            1,                              // non_system - S
            0x3,                              // privilege_level - DPL
            1,                              // segment_present - P
            0,                              // segment_lim 
            0,                              // available_sys_soft - AVL
            0,                              // bit_code_segment - L
            1,                              // def_operation - D/B
            1,                              // granularity - G
            0                               // base_high
        },
        {   // User Data Descriptor
            // TODO : Implement
            (uint16_t) 0xFFFFF,             // segment_low
            0,                              // base_low
            0,                              // base_mid
            0x2,                            // type_bit
            1,                              // non_system - S
            0x3,                              // privilege_level - DPL
            1,                              // segment_present - P
            0,                              // segment_lim 
            0,                              // available_sys_soft - AVL
            0,                              // bit_code_segment - L
            1,                              // def_operation - D/B
            1,                              // granularity - G
            0                               // base_high
        },
        {  // TSS Selector
            .segment_low       = sizeof(struct TSSEntry),
            .base_low          = 0,

            .base_mid          = 0,
            .type_bit          = 0x9,
            .non_system        = 0,    // S bit

            .privilege         = 0,    // DPL
            .valid_bit         = 1,    // P bit

            .segment_high      = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
            .long_mode         = 0,    // L bit
            .opr_32_bit        = 1,    // D/B bit
            .granularity       = 0,    // G bit
            .base_high         = 0,
        },
        {0}
    }
};

/**
 * _gdt_gdtr, predefined system GDTR. 
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    // TODO : Implement, this GDTR will point to global_descriptor_table. 
    //        Use sizeof operator
    sizeof(global_descriptor_table), &global_descriptor_table
};

void gdt_install_tss(void) {
    uint32_t base = (uint32_t) &_interrupt_tss_entry;
    global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
    global_descriptor_table.table[5].base_mid  = (base & (0xFF << 16)) >> 16;
    global_descriptor_table.table[5].base_low  = base & 0xFFFF;
}
