#include "lib-header/stdtype.h"
#include "lib-header/fat32.h"
#include "lib-header/stdmem.h"

#define BIOS_LIGHT_GREEN    0b1010
#define BIOS_GREY           0b0111
#define BIOS_LIGHT_BLUE     0b1001
#define BIOS_LIGHT_RED      0b1100
#define BIOS_WHITE          0b1111

#define KEYBOARD_BUFFER_SIZE    256
#define BUFFER_SIZE            (512*4)


uint32_t cwd_cluster_number = ROOT_CLUSTER_NUMBER;
char request_buf[BUFFER_SIZE];
struct FAT32DriverRequest request;
char keyboard_buf[KEYBOARD_BUFFER_SIZE];

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    __asm__ volatile("int $0x30");
}

bool is_blank(char character) {
    return character == ' '
         || character == '\0';
}

int length(char* text) { 
    int i = 0;
    while (*text != '\n' && !is_blank(*text++)) i++;
    return i;
}

void print(char* text , int color) {
    int length = 0;
    while (*text++) length++;
    syscall(5, (uint32_t) text - length - 1, length, color);
}

char* get_word(int num) {
    char* temp = keyboard_buf;
    int word_count = 0;
    while (1) {
        while (is_blank(*temp)) temp++;
        if (word_count == num) {
            return temp;
        } else {
            while (*temp != '\n' && !is_blank(*temp)) temp++;
            word_count++;
        }
    }
    return temp;
}

void reset_buffer() {
    memset(request_buf, 0, BUFFER_SIZE);
    memset(&request, 0, sizeof(struct FAT32DriverRequest));
    memset(keyboard_buf, 0, KEYBOARD_BUFFER_SIZE);
}

int remove(char* argument1) {
    int retcode;
    int temp_i;
    request.parent_cluster_number = cwd_cluster_number;

    for (int i = 0; i < 8; i++) {
        if (argument1[i] == '.') {
            temp_i = i;
            break;
        } else {
            request.name[i] = argument1[i];
        }
    }
    for (int j = 0; j < 3; j++) {
        request.ext[j] = argument1[temp_i + 1 + j];
    }

    syscall(3, (uint32_t) &request, (uint32_t) &retcode, 0);
    if (retcode == D_REQUEST_UNKNOWN_RETURN) {
        print("FAILED TO REMOVE..\n", BIOS_LIGHT_RED);
    } else if (retcode == D_REQUEST_NOT_FOUND_RETURN) {
        print("FAILED TO REMOVE..\n", BIOS_LIGHT_RED);
    } else {
        print("File has been removed successfully!\n", BIOS_LIGHT_GREEN);
    }
    return retcode;
}



int cd(char* argument1, int argument1_length) {
    int ret;
    uint32_t retcode;
    if (memcmp("..", argument1, 2) == 0 && argument1_length == 2) {
        request.parent_cluster_number = cwd_cluster_number;
        if (cwd_cluster_number != ROOT_CLUSTER_NUMBER) {
            syscall(10, (uint32_t) &request, (uint32_t) &retcode, 0);
            cwd_cluster_number = retcode;
            ret = 1;
        }
    } else {
        request.buffer_size = BUFFER_SIZE;
        request.buf = request_buf;
        memcpy(request.name, argument1, argument1_length);
        int idx = argument1_length;
        while (idx <= 7) {
            request.name[idx] = '\0';
            idx++;
        }
        memcpy(request.ext, "dir", 3);
        request.parent_cluster_number = cwd_cluster_number;
        syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
        if (retcode == RD_REQUEST_SUCCESS_RETURN) {
            syscall(9, (uint32_t) &request, (uint32_t) &retcode, 0);
            cwd_cluster_number = retcode;
            ret = 1;
        } else if (retcode == RD_REQUEST_NOT_FOUND_RETURN) {
            ret = 0;
        }
    }
    return ret;
}

int relative_cd(char* argument1, int argument1_length) {
    char *arg = argument1;
    int len = 0;
    if (argument1_length == 0) {
        return 0;
    }
    if (*(arg + argument1_length - 1) == '/') {
        return 0;
    }
    int ret;
    for (int i = 0; i < argument1_length; i++) {
        if (*(arg + len) == '/') {
            ret = cd(arg, len);
            if (!ret) return 0;
            arg = arg + len + 1;
            len = 0;
        } else {
            len++;
        }
    }
    return cd(arg, len);
}

int copy(char* argument1, char* argument2, int mv) {
    char file_buffer[BUFFER_SIZE];
    char* arg = argument1;
    request.buffer_size = BUFFER_SIZE;
    request.buf = file_buffer;
    request.parent_cluster_number = cwd_cluster_number;
    int i;
    for (i = 0; i < 8 && *arg != '.' && *arg != '\n'; i++) {
        request.name[i] = *arg++;
    }
    memset(request.name + i, 0, 8 - i);
    arg++;
    for (i = 0; i < 3 && *arg != ' ' && *arg != '\n'; i++) {
        request.ext[i] = *arg++;  
    }
    memset(request.name + i, 0, 3 - i);
    int retcode;
    syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);
    if (retcode != R_REQUEST_SUCCESS_RETURN) {
        print("Here", BIOS_WHITE);
        return 0;
    }

    uint32_t temp_cwd = cwd_cluster_number;
    char *last_path = argument2;
    int is_path_valid;
    if (!mv) {
        for (int i = 0; i < length(argument2); i++) {
            if (argument2[i] == '/') 
                last_path = argument2 + i + 1;
        }
        is_path_valid = relative_cd(argument2, length(argument2) - length(last_path) - 1) 
            || length(argument2) == length(last_path);
        if (!is_path_valid) {
            cwd_cluster_number = temp_cwd;
            return 0;
        }
    } else {
        is_path_valid = relative_cd(argument2, length(argument2)) 
            || length(argument2) == length(last_path);
        if (!is_path_valid) {
            cwd_cluster_number = temp_cwd;
            return 0;
        }
    }

    if (!mv) {
        arg = last_path;
        for (i = 0; i < 8 && *arg != '.' && *arg != '\n'; i++) {
            request.name[i] = *arg++;
        }
        memset(request.name + i, 0, 8 - i);
        arg++;
        for (i = 0; i < 3 && *arg != ' ' && *arg != '\n'; i++) {
            request.ext[i] = *arg++;  
        }
        memset(request.name + i, 0, 3 - i);
    } else {
        arg = argument1;
        for (i = 0; i < 8 && *arg != '.' && *arg != '\n'; i++) {
            request.name[i] = *arg++;
        }
        memset(request.name + i, 0, 8 - i);
        arg++;
        for (i = 0; i < 3 && *arg != ' ' && *arg != '\n'; i++) {
            request.ext[i] = *arg++;  
        }
        memset(request.name + i, 0, 3 - i);
    }
    request.buf = file_buffer;
    request.parent_cluster_number = cwd_cluster_number;
    syscall(2, (uint32_t) &request, (uint32_t) &retcode, 0);
    cwd_cluster_number = temp_cwd;
    return 1;
}

int main(void) {
    while (TRUE) {
        reset_buffer();
        syscall(5, (uint32_t) "ded-os-is-ded", 13, BIOS_LIGHT_GREEN);
        syscall(5, (uint32_t) ":", 1, BIOS_WHITE);
        syscall(6, (uint32_t) request_buf, cwd_cluster_number, 0);
        print(request_buf, BIOS_LIGHT_BLUE);
        memset(request_buf, 0, BUFFER_SIZE);
        syscall(5, (uint32_t) "$ ", 2, BIOS_WHITE);
        syscall(4, (uint32_t) keyboard_buf, 256, 0);
        char *command = get_word(0);
        char *argument1 = get_word(1);
        int argument1_length = length(argument1);
        char *argument2 = get_word(2);
        int argument2_length = length(argument2);

        if (*get_word(3) != '\n') {
            syscall(5, (uint32_t) "Command invalid!\n", 17, BIOS_LIGHT_RED);
            continue;
        }

        if (memcmp(command, "cd", 2) == 0 && !argument2_length) {
            uint32_t temp_cwd = cwd_cluster_number;
            int ret = relative_cd(argument1, argument1_length);
            if (!ret) {
                cwd_cluster_number = temp_cwd;
                print("cd: ", BIOS_WHITE);
                syscall(5, (uint32_t) argument1, (uint32_t) argument1_length, BIOS_WHITE);
                print(": No such file or directory\n", BIOS_WHITE);
            }
        } else if (memcmp(command, "ls", 2) == 0 && !argument1_length) {
            syscall(8, (uint32_t) request_buf, cwd_cluster_number, 0);
            if (request_buf[0] == 0) {
                print("DIRECTORY EMPTY\n", BIOS_LIGHT_BLUE);
            } else {
                print(request_buf, BIOS_WHITE);
            }
        } else if (memcmp(command, "mkdir", 5) == 0 && argument1_length != 0) {
            uint32_t retcode;
            request.buffer_size = 0;
            request.buf = request_buf;
            memcpy(request.name, argument1, argument1_length);
            int idx = argument1_length;
            while (idx <= 7) {
                request.name[idx] = '\0';
                idx++;
            }
            memcpy(request.ext, "dir", 3);
            request.parent_cluster_number = cwd_cluster_number;
            syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
            if (retcode == RD_REQUEST_SUCCESS_RETURN) {
                print("FOLDER ALREADY EXISTS\n", BIOS_LIGHT_RED);
            } else if (retcode == RD_REQUEST_NOT_FOUND_RETURN) {
                memset(request_buf, 0, BUFFER_SIZE);
                syscall(2, (uint32_t) &request, (uint32_t) &retcode, 0);
                if (retcode != W_REQUEST_SUCCESS_RETURN) {
                    print("Unknown fault. try again.\n", BIOS_LIGHT_RED);
                }
            }
        } else if (memcmp(command, "cat", 3) == 0 && argument1_length) {
            char* arg = argument1;
            request.buffer_size = BUFFER_SIZE;
            request.buf = request_buf;
            request.parent_cluster_number = cwd_cluster_number;
            int i;
            for (i = 0; i < 8 && *arg != '.' && *arg != '\n'; i++) {
                request.name[i] = *arg++;
            }
            memset(request.name + i, 0, 8 - i);
            arg++;
            for (i = 0; i < 3 && *arg != ' ' && *arg != '\n'; i++) {
                request.ext[i] = *arg++;  
            }
            memset(request.name + i, 0, 3 - i);
            int retcode;
            syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);
            print("cat: ", BIOS_WHITE);
            syscall(5, (uint32_t) argument1, (uint32_t) argument1_length, BIOS_WHITE);
            print(": ", BIOS_WHITE);
            if (retcode == R_NOT_ENOUGH_BUFFER_RETURN) {
                print("File size is too large\n", BIOS_WHITE);
            } else if (retcode == R_REQUEST_NOT_A_FILE_RETURN) {
                print("Is a directory\n", BIOS_WHITE);
            } else if (retcode == R_REQUEST_NOT_FOUND_RETURN) {
                print("No such file or directory\n", BIOS_WHITE);
            } else if (retcode == R_REQUEST_UNKNOWN_RETURN) {
                print("Unknown error occurs\n", BIOS_LIGHT_RED);
            } else {
                syscall(7, (uint32_t) request_buf, BUFFER_SIZE, 0);
            }
        } else if (memcmp(command, "cp", 2) == 0 && argument1_length != 0) {
            copy(argument1, argument2 , 0);
        } else if (memcmp(command, "rm", 2) == 0 && *argument1) {
            remove(argument1);
        } else if (memcmp(command, "mv", 2) == 0 && *argument1) {
            copy(argument1, argument2, 1);
            remove(argument1);
        } else if (memcmp(command, "whereis", 7) == 0) {
            char* arg = argument1;
            request.buffer_size = BUFFER_SIZE;
            request.buf = request_buf;
            request.parent_cluster_number = cwd_cluster_number;
            int i;
            for (i = 0; i < 8 && *arg != '.' && *arg != '\n'; i++) {
                request.name[i] = *arg++;
            }
            memset(request.name + i, 0, 8 - i);
            bool ext = *arg++ == '.';
            if (ext) {
                for (i = 0; i < 3 && *arg != ' ' && *arg != '\n'; i++) {
                    request.ext[i] = *arg++;  
                }
                memset(request.name + i, 0, 3 - i);
            } else {
                memcpy(request.ext, "dir", 3);
            }
            int retcode;
            request.buf = request_buf;
            syscall(12, (uint32_t) &request, (uint32_t) &retcode, 0);
            syscall(5, (uint32_t) argument1, (uint32_t) argument1_length, BIOS_WHITE);
            memset(request_buf + 3 * CLUSTER_SIZE, 0, CLUSTER_SIZE);
            memcpy(request_buf + 3 * CLUSTER_SIZE, argument1, argument1_length);
            print(": ", BIOS_WHITE);
            for (int i = 0; i < retcode; i++) {
                char path [KEYBOARD_BUFFER_SIZE];
                print("/", BIOS_WHITE);
                syscall(6, (uint32_t) path, *(((uint32_t *) request_buf) + i), 0);
                print(path, BIOS_WHITE);
                memset(request_buf + CLUSTER_SIZE, 0, CLUSTER_SIZE);
                print("/", BIOS_WHITE);
                print(request_buf + 3 * CLUSTER_SIZE, BIOS_WHITE);
                print("  ", BIOS_WHITE);
            }
            print("\n", BIOS_WHITE);
        } else if (memcmp(command, "clear", 5) == 0) {
            syscall(11, 0, 0, 0);
        } else {
            print("command invalid\n", BIOS_LIGHT_RED);
        }
    }
    return 0;
}
