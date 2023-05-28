#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stddef.h>

struct cpu_registers {
    uint8_t ax[2];
    uint8_t bx[2];
    uint8_t cx[2];
    uint8_t dx[2];

    uint16_t sp;
    uint16_t bp;
    uint16_t si;
    uint16_t di;

    uint16_t cs;
    uint16_t ss;
    uint16_t ds;
    uint16_t es;

    uint16_t ip;
    uint32_t ip32;

    uint16_t flags;
};

#define CPU_HALTED (1 << 0)

struct cpu {
    uint8_t *memory;
    size_t memory_size;

    struct cpu_registers reg;

    uint8_t state;
};

int cpu_run(struct cpu *cpu, size_t steps);

#endif