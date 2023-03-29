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
                read_clusters((void*) &driver_state.dir_table_buf + CLUSTER_SIZE*fragment, request_cluster_number, 1);
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
                    read_clusters((void*) &driver_state.dir_table_buf, request_cluster_number, 1);
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

    bool parent_is_not_dir = driver_state.dir_table_buf.table[0].attribute == ATTR_SUBDIRECTORY;
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
                        read_clusters((void*) &driver_state.dir_table_buf + CLUSTER_SIZE*fragment, request_cluster_number, 1);
                        request_cluster_number = driver_state.fat_table.cluster_map[request_cluster_number];
                    }
                    return REQUEST_SUCCESS_RETURN;
                    
                } else {
                    return REQUEST_NOT_A_FILE_RETURN;
                }
            }
        }
        next_cluster = driver_state.fat_table.cluster_map[next_cluster]; 
    } while (next_cluster != FAT32_FAT_END_OF_FILE);

    return REQUEST_NOT_FOUND_RETURN;
}

/**
 * FAT32 write, write a file or folder to file system.
 *
 * @param request All attribute will be used for write, buffer_size == 0 then create a folder / directory
 * @return Error code: 0 success - 1 file/folder already exist - 2 invalid parent cluster - -1 unknown
 */
int8_t write(struct FAT32DriverRequest request);


/**
 * FAT32 delete, delete a file or empty directory (only 1 DirectoryEntry) in file system.
 *
 * @param request buf and buffer_size is unused
 * @return Error code: 0 success - 1 not found - 2 folder is not empty - -1 unknown
 */
int8_t delete(struct FAT32DriverRequest request) ;
