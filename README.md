# OS 32-bit Development 
## Milestone 2 : Interrupt and Filesystem


## **Table of Contents**
* [Authors](#authors)
* [Program Description](#program-description)
* [Required Program](#required-program)
* [How to Run The Program](#how-to-run-the-program)
* [How to Compile The Program](#how-to-run-the-program)
* [Progress Report](#progress-report)
* [Folders and Files Description](#folders-and-files-description)

## **Authors** 
| Name | ID |
|-----|----|
| Daniel Egiant Sitanggang | 13521056 |
| Ryan Samuel Chandra | 13521140
| Juan Christopher Santoso | 13521116 | 
| Antonio Natthan Krishna | 13521162 | 

## **Program Description**
This program is an operating system development project on 32-bit architecture. In this milestone, we have developed OS interrupt system, keyboard and disk drivers, filesystem using "FAT32 - IF2230 Edition". 

## **Required Program**
Here are the required programs you need to prepare to run the program in this repository:
| Required Program      | Reference Link |
|-----------------------|----------------|
| C                  | [C Installation Guide](https://code.visualstudio.com/docs/cpp/config-mingw) |
| Visual Studio  Code|  [VSCode Installation Guide](https://visualstudio.microsoft.com/)|
-----------------------
It is required to compile and run this OS-32bit from LINUX OS. You can use Windows Subsystems for Linux (WSL) to substitute the LINUX OS.  
 
## **How to Run The Program**
1. Clone this repository (skip if the step is done)</br>
```sh
git clone https://github.com/Sister20/if2230-2023-ded_os_is_ded.git
```
2. Change the current directory into the cloned repository </br>
```sh
cd if2230-2023-ded_os_is_ded
```
3. Run command
```sh
make start
```

## **How to Compile The Program**
Under ideal circumstances, you just need to use the command `make start` to run OS-32bit as listed in [How To Run The Program](#how-to-run-the-program). If you modify some of codes from the OS-32bit source code, you need to run the subsequent steps to compile the modified OS-32bit into the executables.  
1. Clone this repository (skip if the step is done)</br>
```sh
git clone https://github.com/Sister20/if2230-2023-ded_os_is_ded.git
```
2. Change the current directory into the cloned repository </br>
```sh
cd if2230-2023-ded_os_is_ded
```
3. To only compile OS-32bit source code without run the executables
```sh
make build
```
4. To compile and run OS-32bit source code
```sh
make run
```
5. To format disk used in OS-32bit
```sh
make disk
```
WARNING: Your OS-32bit source code must be warning-free since all warnings will be converted to errors. 

## **Progress Report Milestone 2**

| Points        | Done  |
|---------------|-------|
| IDT Data Structures |  &check; |
| PIC Remapping | &check;  |
| Interrupt Handler |  &check; |
| Load IDT   |  &check; |
| IRQ1 - Keyboard Controller    |  &check; |
| Keyboard Driver Interfaces |  &check; |
| Keyboard ISR | &check;  |
| Disk Driver & Image - ATA PIO| &check; |
| FAT32 Data Structures |  &check; |
| File System Initializer |  &check; |
| File System CRUD Operation |  &check; |
| Unlimited Directory Entry |   |
| CMOS Time for File System|   |
| Custom File System |   |
| Speedrun No Guidebook |   |

## **Folders and Files Description**
    .   
    ├─ bin                              # Executables
    ├─ other                            
    ├─ src                              # Source Code
        ├─ filesystem                            
            ├─ disk.c
            ├─ fat32.c
        ├─ interrupt                        
            ├─ idt.c
            ├─ interrupt.c
            └─ intsetup.s
        ├─ keyboard                           
            └─ keyboard.c
        ├─ lib-header                           
            ├─ disk.h
            ├─ fat32.h
            ├─ framebuffer.h
            ├─ gdt.h
            ├─ idt.h
            ├─ interrupt.h
            ├─ kernel_loader.h
            ├─ keyboard.h
            ├─ portio.h
            ├─ stdmem.h
            └─ stdtype.h
        ├─ framebuffer.c
        ├─ gdt.c                              
        ├─ kernel_loader.s
        ├─ kernel.c
        ├─ linker.ld
        ├─ menu.lst
        ├─ stdmem.c
        └─ portio.c
    ├─ makefile
    └─ README.md
