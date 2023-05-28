#include <cpu/cpu.h>
#include <cpu/opcodes.h>

int cpu_run(struct cpu *cpu, size_t steps) {
    for (size_t step = 0; step < steps; step++) {
        if (cpu->state & CPU_HALTED) return 1;
        opcode_execute(cpu);
    }
    return 0;
}