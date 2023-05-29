#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <cpu/cpu.h>
#include <cpu/opcodes.h>

int main(void) {
    printf("Hello World!\n");

    struct cpu cpu = {0};

    // 640KB of goodness
    cpu.memory = malloc(640 * 1024);
    cpu.memory_size = 640 * 1024;

    cpu.reg.ip32 = 0;
    cpu.reg.ip = 0;
    cpu.reg.cs = 0;

    opcode_set_reg16_val(cpu.reg.ax, 0xAAFF);
    opcode_set_reg16_val(cpu.reg.bx, 0xBBBB);

    printf("AX: 0x%x BX: 0x%x\n", opcode_reg8_to_reg16(cpu.reg.ax), opcode_reg8_to_reg16(cpu.reg.bx));

    /*
        xor ax, bx
        xor bx, bx
        and ax, bx
        hlt
    */
    char code[] = "\x31\xd8\x31\xdb\x21\xd8\xf4";

    memcpy(cpu.memory, code, sizeof(code) - 1);

    cpu_run(&cpu, sizeof(code) - 1);

    printf("AX: 0x%x BX: 0x%x FLAGS 0x%x\n", opcode_reg8_to_reg16(cpu.reg.ax), opcode_reg8_to_reg16(cpu.reg.bx), cpu.reg.flags);
}
