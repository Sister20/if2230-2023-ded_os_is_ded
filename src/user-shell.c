#include "lib-header/stdtype.h"
#include "lib-header/fat32.h"
#include "lib-header/stdmem.h"

#define BIOS_LIGHT_GREEN    0b1010
#define BIOS_GREY           0b0111
#define BIOS_LIGHT_BLUE     0b1001
#define BIOS_WHITE          0b1111

#define KEYBOARD_BUFFER_SIZE    256
#define BUFFER_SIZE            (512*4)


uint32_t cwd_cluster_number = ROOT_CLUSTER_NUMBER;
char request_buf[4*CLUSTER_SIZE];
struct FAT32DriverRequest request;
char keyboard_buf[KEYBOARD_BUFFER_SIZE];

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    __asm__ volatile("int $0x30");
}

uint32_t hash(char* name) {
    uint32_t result = 0;
    for (int i = 0; i < 8; i++) {
        result = (result*31 + name[i]) % CLUSTER_MAP_SIZE;
    }
    return result;
}

bool is_alphanumeric(char *character) {
    return *character != ' '
        && *character != '\0';
}

int length(char* text) { 
    int i = 0;
    while (is_alphanumeric(text++)) i++;
    return i;
}

void print(char* text , int color) {
    int length = 0;
    while (*text++) length++;
    syscall(5, (uint32_t) text - length -1, length, color);
}

char* get_command() {
    char* temp = keyboard_buf;
    int i = 0;
    while (!is_alphanumeric(temp++) && i++ < KEYBOARD_BUFFER_SIZE);
    return temp - 1;
}

char* get_argument() {
    char* temp = get_command();
    int i = length(temp);
    temp += i;
    while (!is_alphanumeric(temp++) && i++ < KEYBOARD_BUFFER_SIZE);
    return temp - 1;
}

int main(void) {
    while (TRUE) {
        syscall(5, (uint32_t) "ded-os-is-ded", 13, BIOS_LIGHT_GREEN);
        syscall(5, (uint32_t) ":", 1, BIOS_WHITE);
        syscall(6, (uint32_t) request_buf, cwd_cluster_number, 0);
        print(request_buf, BIOS_LIGHT_BLUE);
        syscall(5, (uint32_t) "$ ", 2, BIOS_WHITE);
        syscall(4, (uint32_t) keyboard_buf, 256, 0);
        char *command = get_command();
        char *argument = get_argument();
        
        int argument_length = length(argument);
        if (memcmp(command, "cd", 2) == 0 && argument_length != 0) {
            cwd_cluster_number = *argument - '0';
        } else if (memcmp(command, "ls", 2) == 0 && argument_length == 0) {
            print("command ls\n", BIOS_WHITE);
        } else if (memcmp(command, "mkdir", 5) == 0 && argument_length != 0) {
            print("command mkdir\n", BIOS_WHITE);
        } else if (memcmp(command, "cat", 3) == 0 && argument_length != 0) {
            print("command cat\n", BIOS_WHITE);
        } else if (memcmp(command, "cp", 2) == 0 && argument_length != 0) {
            print("command cp\n", BIOS_WHITE);  
        } else if (memcmp(command, "rm", 2) == 0 && argument_length != 0) {
            print("command rm\n", BIOS_WHITE);
        } else if (memcmp(command, "mv", 2) == 0 && argument_length != 0) {
            print("command mv\n", BIOS_WHITE);
        } else if (memcmp(command, "whereis", 7) == 0) {
            print("command whereis\n", BIOS_WHITE);
        } else {
            print("command invalid\n", BIOS_WHITE);
        }

    }
    return 0;
}
