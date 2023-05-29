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

#define opcode_reg8_to_reg16(a) (a[1] << 8 | a[0] & 0xff)
#define opcode_set_reg16_val(a, b) a[1] = (uint16_t) b >> 8; a[0] = (uint16_t) b & 0xff;

void opcode_execute(struct cpu *cpu);

#endif
