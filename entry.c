#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <cpu/cpu.h>

int main(void) {
    printf("Hello World!\n");

    struct cpu cpu = {0};

    // 640KB of goodness
    cpu.memory = malloc(640 * 1024);
    cpu.memory_size = 640 * 1024;

    cpu.reg.ip32 = 0;
    cpu.reg.ip = 0;
    cpu.reg.cs = 0;

    char code[] = "\x31\xc0\x40";

    memcpy(cpu.memory, code, sizeof(code) - 1);

    return cpu_run(&cpu, sizeof(code) - 1);
}
