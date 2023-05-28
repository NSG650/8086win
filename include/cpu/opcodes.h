#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>
#include <stddef.h>

#include <cpu/cpu.h>

struct opcode {
    char name[32];
    size_t operand_length;
    void *function;
};

void opcode_execute(struct cpu *cpu);

#endif
