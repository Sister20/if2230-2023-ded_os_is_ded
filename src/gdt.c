#include "lib-header/stdtype.h"
#include "lib-header/gdt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to GDT definition in Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
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
        {
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
        {
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
        }
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
