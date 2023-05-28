#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

uint8_t memory_read_byte(struct cpu *cpu, uintptr_t addr);
uint16_t memory_read_word(struct cpu *cpu, uintptr_t addr);
void memory_write_byte(struct cpu *cpu, uintptr_t addr, uint8_t byte);
void memory_write_word(struct cpu *cpu, uintptr_t addr, uint16_t word);

#endif