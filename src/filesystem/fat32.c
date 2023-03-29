#include "../lib-header/stdtype.h"
#include "../lib-header/fat32.h"
#include "../lib-header/stdmem.h"

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

struct FAT32DriverState driver_state;
struct FAT32DriverRequest driver_request;

/**
 * Convert cluster number to logical block address
 * 
 * @param cluster Cluster number to convert
 * @return uint32_t Logical Block Address
 */
uint32_t cluster_to_lba(uint32_t cluster){
    return cluster * CLUSTER_BLOCK_COUNT;
}
/**
 * Initialize DirectoryTable value with parent DirectoryEntry and directory name
 * 
 * @param dir_table          Pointer to directory table
 * @param name               8-byte char for directory name
 * @param parent_dir_cluster Parent directory cluster number
 */
void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster){
    memcpy(dir_table->table->name, name, 8);
    dir_table->table->cluster_low = (uint16_t) parent_dir_cluster;
    dir_table->table->cluster_high = (uint16_t) (parent_dir_cluster >> 16);
    dir_table->table->attribute = ATTR_SUBDIRECTORY;
    dir_table->table->user_attribute = UATTR_NOT_EMPTY;
}

/**
 * Checking whether filesystem signature is missing or not in boot sector
 * 
 * @return True if memcmp(boot_sector, fs_signature) returning inequality
 */
bool is_empty_storage(void){
    struct BlockBuffer temp;
    read_blocks(temp.buf, BOOT_SECTOR, 1);
    return (memcmp(fs_signature, temp.buf, BLOCK_SIZE) != 0);
}

/**
 * Create new FAT32 file system. Will write fs_signature into boot sector and 
 * proper FileAllocationTable (contain CLUSTER_0_VALUE, CLUSTER_1_VALUE, 
 * and initialized root directory) into cluster number 1
 */
void create_fat32(void){
    
    struct FAT32FileAllocationTable table;
    table.cluster_map[0] = CLUSTER_0_VALUE;
    table.cluster_map[1] = CLUSTER_1_VALUE;
    table.cluster_map[2] = ROOT_CLUSTER_NUMBER;

    // write fs_signature into boot sector
    write_blocks(fs_signature, BOOT_SECTOR, 1);

    // write FAT cluster map to cluster number 1
    write_clusters(table.cluster_map, FAT_CLUSTER_NUMBER, 1);
}

/**
 * Initialize file system driver state, if is_empty_storage() then create_fat32()
 * Else, read and cache entire FileAllocationTable (located at cluster number 1) into driver state
 */
void initialize_filesystem_fat32(void){
    if (is_empty_storage()){
        create_fat32();
    } else {
        read_clusters(driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
    }
}

/**
 * Write cluster operation, wrapper for write_blocks().
 * Recommended to use struct ClusterBuffer
 * 
 * @param ptr            Pointer to source data
 * @param cluster_number Cluster number to write
 * @param cluster_count  Cluster count to write, due limitation of write_blocks block_count 255 => max cluster_count = 63
 */
void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    write_blocks(ptr, cluster_to_lba(cluster_number), cluster_count*CLUSTER_BLOCK_COUNT);
}

/**
 * Read cluster operation, wrapper for read_blocks().
 * Recommended to use struct ClusterBuffer
 * 
 * @param ptr            Pointer to buffer for reading
 * @param cluster_number Cluster number to read
 * @param cluster_count  Cluster count to read, due limitation of read_blocks block_count 255 => max cluster_count = 63
 */
void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    read_blocks(ptr, cluster_to_lba(cluster_number), cluster_count*CLUSTER_BLOCK_COUNT);
}


/* -- CRUD OPERATION -- */

/* Additional Operations */
int8_t dirtable_linear_search(struct FAT32DriverRequest request) {
    const int REQUEST_UNKNOWN_RETURN = -1;

    bool parent_is_not_dir = driver_state.dir_table_buf.table[0].attribute = ATTR_SUBDIRECTORY;
    if (parent_is_not_dir) {
        return (REQUEST_UNKNOWN_RETURN);
    }

    // check if any file in parent have same name and extension
    int dir_length = sizeof(struct FAT32DirectoryTable)/sizeof(struct FAT32DirectoryEntry);
    uint32_t next_cluster = request.parent_cluster_number;

    do {
        if (next_cluster != request.parent_cluster_number) {
            read_clusters((void*) &driver_state.dir_table_buf, next_cluster, 1);
        }
        for (int8_t i = 1; i < dir_length; i++) {
            struct FAT32DirectoryEntry current_entry = driver_state.dir_table_buf.table[i];
            bool current_entry_name_equal = memcmp(current_entry.name, request.name, 8);
            bool current_entry_ext_equal = memcmp(current_entry.ext, request.ext, 3);
            if (current_entry_ext_equal && current_entry_name_equal) {
                return (i);
            }
        }
        next_cluster = driver_state.fat_table.cluster_map[next_cluster]; 
    } while (next_cluster != FAT32_FAT_END_OF_FILE);

    return (REQUEST_UNKNOWN_RETURN);
} 

/* Optimized read() and read_directory() */
int8_t optimized_read_directory(struct FAT32DriverRequest request) {
    const int REQUEST_SUCCESS_RETURN = 0;
    const int REQUEST_NOT_A_FOLDER_RETURN = 1;
    const int REQUEST_NOT_FOUND_RETURN = 2;
    
    // load request parent to buffer
    read_clusters((void*) &driver_state.dir_table_buf, request.parent_cluster_number, 1);
    
    int8_t search = dirtable_linear_search(request);
    if (search == -1) {
        return (search);
    } else {
        struct FAT32DirectoryEntry current_entry = driver_state.dir_table_buf.table[search];
        
        bool current_entry_is_dir = current_entry.attribute == ATTR_SUBDIRECTORY;
        if (current_entry_is_dir) {
            uint32_t request_cluster_number = current_entry.cluster_high << 16 
                                                | current_entry.cluster_low;
            read_clusters((void*) &driver_state.dir_table_buf, request_cluster_number, 1);
            return (REQUEST_SUCCESS_RETURN);
        } else {
            return (REQUEST_NOT_A_FOLDER_RETURN);
        }
    }
    return (REQUEST_NOT_FOUND_RETURN);
}

int8_t optimized_read(struct FAT32DriverRequest request) {
    const int REQUEST_SUCCESS_RETURN = 0;
    const int REQUEST_NOT_A_FILE_RETURN = 1;
    const int NOT_ENOUGH_BUFFER_RETURN = 2;
    const int REQUEST_NOT_FOUND_RETURN = 3;

    // load request parent to buffer
    read_clusters((void*) &driver_state.dir_table_buf, request.parent_cluster_number, 1);

    int8_t search = dirtable_linear_search(request);
    if (search == -1) {
        return (search);
    } else {
        struct FAT32DirectoryEntry current_entry = driver_state.dir_table_buf.table[search];

        bool current_entry_is_file = current_entry.attribute != ATTR_SUBDIRECTORY;
        if (current_entry_is_file) {
            uint32_t request_cluster_number = current_entry.cluster_high << 16 
                                                | current_entry.cluster_low;
            int buffer_size = sizeof(driver_state.dir_table_buf) + sizeof(driver_state.cluster_buf);
            int fragment = 0;
            while (request_cluster_number != FAT32_FAT_END_OF_FILE) {
                buffer_size -= CLUSTER_SIZE;
                if (buffer_size < 0) {
                    return (NOT_ENOUGH_BUFFER_RETURN);
                }
                read_clusters(&request.buffer_size + CLUSTER_SIZE*fragment, request_cluster_number, 1);
                request_cluster_number = driver_state.fat_table.cluster_map[request_cluster_number];
            }
            return (REQUEST_SUCCESS_RETURN);
            
        } else {
            return (REQUEST_NOT_A_FILE_RETURN);
        }
    }

    return (REQUEST_NOT_FOUND_RETURN);
}



/**
 *  FAT32 Folder / Directory read
 *
 * @param request buf point to struct FAT32DirectoryTable,
 *                name is directory name,
 *                ext is unused,
 *                parent_cluster_number is target directory table to read,
 *                buffer_size must be exactly sizeof(struct FAT32DirectoryTable)
 * @return Error code: 0 success - 1 not a folder - 2 not found - -1 unknown
 */
int8_t read_directory(struct FAT32DriverRequest request) {
    const int REQUEST_SUCCESS_RETURN = 0;
    const int REQUEST_NOT_A_FOLDER_RETURN = 1;
    const int REQUEST_NOT_FOUND_RETURN = 2;
    const int REQUEST_UNKNOWN_RETURN = -1;
    
    // load request parent to buffer
    read_clusters((void*) &driver_state.dir_table_buf, request.parent_cluster_number, 1);

    bool parent_is_not_dir = driver_state.dir_table_buf.table[0].attribute = ATTR_SUBDIRECTORY;
    if (parent_is_not_dir) {
        return REQUEST_UNKNOWN_RETURN;
    }

    // check if any file in parent have same name and extension
    int dir_length = sizeof(struct FAT32DirectoryTable)/sizeof(struct FAT32DirectoryEntry);
    uint32_t next_cluster = request.parent_cluster_number;
    do 
    {
        if (next_cluster != request.parent_cluster_number) {
            read_clusters((void*) &driver_state.dir_table_buf, next_cluster, 1);
        }
        for (int i = 1; i < dir_length; i++) {
            struct FAT32DirectoryEntry current_entry = driver_state.dir_table_buf.table[i];
            bool current_entry_name_equal = memcmp(current_entry.name, request.name, 8);
            bool current_entry_ext_equal = memcmp(current_entry.ext, request.ext, 3);
            if (current_entry_ext_equal && current_entry_name_equal) {
                bool current_entry_is_dir = current_entry.attribute == ATTR_SUBDIRECTORY;
                if (current_entry_is_dir) {
                    uint32_t request_cluster_number = current_entry.cluster_high << 16 
                                                        | current_entry.cluster_low;
                    read_clusters(request.buf, request_cluster_number, 1);
                    return REQUEST_SUCCESS_RETURN;
                } else {
                    return REQUEST_NOT_A_FOLDER_RETURN;
                }
            }
        }
        next_cluster = driver_state.fat_table.cluster_map[next_cluster]; 
    } while (next_cluster != FAT32_FAT_END_OF_FILE);

    return REQUEST_NOT_FOUND_RETURN;
}


/**
 * FAT32 read, read a file from file system.
 *
 * @param request All attribute will be used for read, buffer_size will limit reading count
 * @return Error code: 0 success - 1 not a file - 2 not enough buffer - 3 not found - -1 unknown
 */
int8_t read(struct FAT32DriverRequest request) {
    const int REQUEST_SUCCESS_RETURN = 0;
    const int REQUEST_NOT_A_FILE_RETURN = 1;
    const int NOT_ENOUGH_BUFFER_RETURN = 2;
    const int REQUEST_NOT_FOUND_RETURN = 3;
    const int REQUEST_UNKNOWN_RETURN = -1;
    
    // load request parent to buffer
    read_clusters((void*) &driver_state.dir_table_buf, request.parent_cluster_number, 1);

    bool parent_is_not_dir = driver_state.dir_table_buf.table[0].attribute != ATTR_SUBDIRECTORY;
    if (parent_is_not_dir) {
        return REQUEST_UNKNOWN_RETURN;
    }

    // check if any file in parent have same name and extension
    int dir_length = sizeof(struct FAT32DirectoryTable)/sizeof(struct FAT32DirectoryEntry);

    for (int i = 1; i < dir_length; i++) {
        struct FAT32DirectoryEntry current_entry = driver_state.dir_table_buf.table[i];
        bool current_entry_name_equal = memcmp(current_entry.name, request.name, 8) == 0;
        bool current_entry_ext_equal = memcmp(current_entry.ext, request.ext, 3) == 0;
        if (current_entry_ext_equal && current_entry_name_equal) {
            bool current_entry_is_file = current_entry.attribute != ATTR_SUBDIRECTORY;
            if (current_entry_is_file) {
                uint32_t request_cluster_number = current_entry.cluster_high << 16 
                                                    | current_entry.cluster_low;
                int buffer_size = sizeof(driver_state.dir_table_buf) + sizeof(driver_state.cluster_buf);
                int fragment = 0;
                while (request_cluster_number != FAT32_FAT_END_OF_FILE) {
                    buffer_size -= CLUSTER_SIZE;
                    if (buffer_size < 0) {
                        return NOT_ENOUGH_BUFFER_RETURN;
                    }
                    int offset = CLUSTER_SIZE*fragment;
                    read_clusters((char*) request.buf + offset, request_cluster_number, 1);
                    request_cluster_number = driver_state.fat_table.cluster_map[request_cluster_number];
                }
                return REQUEST_SUCCESS_RETURN;
                
            } else {
                return REQUEST_NOT_A_FILE_RETURN;
            }
        }
    }
      

    return REQUEST_NOT_FOUND_RETURN;
}

uint32_t get_empty_cluster() {
    for (uint32_t i = 2; i < CLUSTER_MAP_SIZE; i++) {
        bool is_current_cluster_empty = driver_state.fat_table.cluster_map[i] == FAT32_FAT_EMPTY_ENTRY;
        if (is_current_cluster_empty) {
            driver_state.fat_table.cluster_map[i] == FAT32_FAT_END_OF_FILE;
            return i;
        }
    }
    return -1;
}

/**
 * FAT32 write, write a file or folder to file system.
 *
 * @param request All attribute will be used for write, buffer_size == 0 then create a folder / directory
 * @return Error code: 0 success - 1 file/folder already exist - 2 invalid parent cluster - -1 unknown
 */
int8_t write(struct FAT32DriverRequest request) {
    const int REQUEST_SUCCESS_RETURN = 0;
    const int REQUEST_FILE_ALREADY_EXIST_RETURN = 1;
    const int REQUEST_INVALID_PARENT_RETURN = 2;
    const int REQUEST_UNKNOWN_RETURN = -1;

    // load request parent to table buffer, load fat table
    read_clusters((void*) &driver_state.dir_table_buf, request.parent_cluster_number, 1);
    read_clusters((void*) &driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);

    bool parent_is_not_dir = driver_state.dir_table_buf.table[0].attribute != ATTR_SUBDIRECTORY;
    if (parent_is_not_dir) {
        return REQUEST_UNKNOWN_RETURN;
    }

    int dir_length = sizeof(struct FAT32DirectoryTable)/sizeof(struct FAT32DirectoryEntry);
    for (int i = 1; i < dir_length; i++) {
        struct FAT32DirectoryEntry current_entry = driver_state.dir_table_buf.table[i];
        bool current_entry_valid = current_entry.undelete;
        bool current_entry_name_equal = memcmp(current_entry.name, request.name, 8);
        bool current_entry_ext_equal = memcmp(current_entry.ext, request.ext, 3);
        if (current_entry_ext_equal && current_entry_name_equal && current_entry_valid) {
            return REQUEST_FILE_ALREADY_EXIST_RETURN;
        }
    }

    int num_cluster_needed = ((request.buffer_size -  1)/ CLUSTER_SIZE) + 1; 
    int num_cluster_avail = 0;

    for (int i = 2; i < CLUSTER_MAP_SIZE && num_cluster_avail < num_cluster_needed; i++) {
        if (driver_state.fat_table.cluster_map[i] == FAT32_FAT_EMPTY_ENTRY) {
            num_cluster_avail++;
        }
    }

    if (num_cluster_avail < num_cluster_needed) {
        return REQUEST_UNKNOWN_RETURN;
    }

    int entry_num = -1;
    for (int i = 1; i < dir_length && entry_num == -1; i++) {
        if (driver_state.dir_table_buf.table[i].undelete == 0) {
            entry_num = i;
        }
    }
    if (entry_num == -1) {
        return REQUEST_UNKNOWN_RETURN;
    }

    
    if (request.buffer_size == 0) { 
        // create new dir
        struct FAT32DirectoryTable request_directory_table;
        init_directory_table(&request_directory_table, request.name, 
                                request.parent_cluster_number);
        uint32_t cluster_num_to_write =  get_empty_cluster();

        struct FAT32DirectoryEntry request_entry;
        // request_entry acc date,
        request_entry.attribute = ATTR_SUBDIRECTORY;
        request_entry.cluster_high = (uint16_t) cluster_num_to_write  >> 16;
        request_entry.cluster_low = (uint16_t) cluster_num_to_write;
        memcpy(request_entry.ext, request.ext, 3);
        memcpy(request_entry.name, request.name, 8);
        request_entry.undelete = 1;

        driver_state.fat_table.cluster_map[cluster_num_to_write] == FAT32_FAT_END_OF_FILE;
        driver_state.dir_table_buf.table[1] = request_entry;
        write_clusters(&request_directory_table, cluster_num_to_write, 1);
        
        // update at storage
        write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
        write_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
    } else {
        uint32_t cluster_num_to_write =  get_empty_cluster();
        struct FAT32DirectoryEntry request_entry;
        request_entry.attribute = ATTR_SUBDIRECTORY;
        request_entry.cluster_high = (uint16_t) cluster_num_to_write  >> 16;
        request_entry.cluster_low = (uint16_t) cluster_num_to_write;
        memcpy(request_entry.ext, request.ext, 3);
        memcpy(request_entry.name, request.name, 8);
        request_entry.undelete = 1;

        write_clusters(&request.buf, cluster_num_to_write, 1);
        
        for (int j = 1; j < num_cluster_needed; j++) {
            uint32_t next_cluster_num_to_write = get_empty_cluster();
            driver_state.fat_table.cluster_map[cluster_num_to_write] = next_cluster_num_to_write;
            int offset = CLUSTER_SIZE*j;
            write_clusters((char*) request.buf + offset, next_cluster_num_to_write, 1);
            cluster_num_to_write = next_cluster_num_to_write;
        }
        write_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
        write_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);

    }

    return REQUEST_SUCCESS_RETURN;
}


/**
 * FAT32 delete, delete a file or empty directory (only 1 DirectoryEntry) in file system.
 *
 * @param request buf and buffer_size is unused
 * @return Error code: 0 success - 1 not found - 2 folder is not empty - -1 unknown
 */
int8_t delete(struct FAT32DriverRequest request) {
    const int REQUEST_SUCCESS_RETURN = 0;
    const int REQUEST_NOT_FOUND_RETURN = 1;
    const int FOLDER_NOT_EMPTY_RETURN = 2;
    const int REQUEST_UNKNOWN_RETURN = -1;

    read_clusters((void*) &driver_state.dir_table_buf, request.parent_cluster_number, 1);
    read_clusters((void*) &driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);

    bool parent_is_not_dir = driver_state.dir_table_buf.table[0].attribute == ATTR_SUBDIRECTORY;
    if (parent_is_not_dir) {
        return REQUEST_UNKNOWN_RETURN;
    }

    int dir_length = sizeof(struct FAT32DirectoryTable) / sizeof(struct FAT32DirectoryEntry);
    for (int i = 0; i < dir_length; i++) {
        struct FAT32DirectoryEntry current = driver_state.dir_table_buf.table[i];
        if (memcmp(current.name, request.name, 8) && memcmp(current.ext, request.ext, 3)) {
            if (i != 0) {
                if (current.attribute == ATTR_SUBDIRECTORY) {
                    if (request.parent_cluster_number == ROOT_CLUSTER_NUMBER) {
                        return REQUEST_UNKNOWN_RETURN;
                    } else {
                        int fileExist = 0;
                        for (int j = 0; j < dir_length; j++) {
                            struct FAT32DirectoryEntry current_check = driver_state.dir_table_buf.table[j];
                            if (current_check.undelete != 0) {
                                fileExist = 1;
                                break;
                            }
                        }

                        if (fileExist == 1) {
                            return FOLDER_NOT_EMPTY_RETURN;
                        } else {
                            // HAPUS FOLDER
                            uint32_t deleted_cluster_number = current.cluster_high << 16 | current.cluster_low;
                            driver_state.fat_table.cluster_map[deleted_cluster_number] = FAT32_FAT_EMPTY_ENTRY;
                            return REQUEST_SUCCESS_RETURN;
                        }
                    }
                } else {
                    // HAPUS FILE
                    if (driver_state.fat_table.cluster_map[i] == FAT32_FAT_END_OF_FILE) {
                        driver_state.fat_table.cluster_map[i] = 0x00;
                        return REQUEST_SUCCESS_RETURN;
                    } else {
                        while (driver_state.fat_table.cluster_map[i] != FAT32_FAT_END_OF_FILE) {
                            driver_state.fat_table.cluster_map[i] = 0x00;
                        }
                        i = driver_state.fat_table.cluster_map[i];
                        return REQUEST_SUCCESS_RETURN;
                    }
                }
            } else {
                return REQUEST_UNKNOWN_RETURN;
            }
        }
    }

    return REQUEST_NOT_FOUND_RETURN;
}