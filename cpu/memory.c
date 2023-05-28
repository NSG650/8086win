#include <cpu/cpu.h>
#include <cpu/memory.h>

uint8_t memory_read_byte(struct cpu *cpu, uintptr_t addr) {
    return (*(uint8_t*)(cpu->memory + addr));
}

uint16_t memory_read_word(struct cpu *cpu, uintptr_t addr) {
    return (*(uint16_t *)(cpu->memory + addr));
}

void memory_write_byte(struct cpu *cpu, uintptr_t addr, uint8_t byte) {
    (*(uint8_t*)(cpu->memory + addr)) = byte;
}

void memory_write_word(struct cpu *cpu, uintptr_t addr, uint16_t word) {
    (*(uint16_t *)(cpu->memory + addr)) = word;
}
