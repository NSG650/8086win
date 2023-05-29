#include <cpu/opcodes.h>
#include <cpu/memory.h>

#include <stdio.h>

#define debug_print printf

static const uint8_t parity_table[0x100] = {
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
        1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1};

static inline uint16_t opcode_get_segment_register(struct cpu *cpu, uint8_t reg_id) {
    switch (reg_id) {
        case 0: return cpu->reg.es;
        case 1: return cpu->reg.cs;
        case 2: return cpu->reg.ss;
        case 3: return cpu->reg.ds;
        case 4:
        case 5:
        case 6:
        case 7: return cpu->reg.es;
        default: return -1;
    }
}

static inline uint8_t opcode_get_byte_register(struct cpu *cpu, uint8_t reg_id)  {
    switch (reg_id) {
        case 0: return cpu->reg.ax[0];
        case 1: return cpu->reg.cx[0];
        case 2: return cpu->reg.dx[0];
        case 3: return cpu->reg.bx[0];
        case 4: return cpu->reg.ax[1];
        case 5: return cpu->reg.cx[1];
        case 6: return cpu->reg.dx[1];
        case 7: return cpu->reg.bx[1];
        default: return -1;
    }
}

static inline uint16_t opcode_get_word_register(struct cpu *cpu, uint8_t reg_id)  {
    switch (reg_id) {
        case 0: return opcode_reg8_to_reg16(cpu->reg.ax);
        case 1: return opcode_reg8_to_reg16(cpu->reg.cx);
        case 2: return opcode_reg8_to_reg16(cpu->reg.dx);
        case 3: return opcode_reg8_to_reg16(cpu->reg.bx);
        case 4: return cpu->reg.sp;
        case 5: return cpu->reg.bp;
        case 6: return cpu->reg.si;
        case 7: return cpu->reg.di;
        default: return -1;
    }
}

static inline void opcode_set_segment_register(struct cpu *cpu, uint8_t reg_id, uint16_t val) {
    switch (reg_id) {
        case 0: cpu->reg.es = val; return;
        case 1: cpu->reg.cs = val; return;
        case 2: cpu->reg.ss = val; return;
        case 3: cpu->reg.ds = val; return;
        case 4:
        case 5:
        case 6:
        case 7: cpu->reg.es = val; return;
        default: return;
    }
}

static inline void opcode_set_byte_register(struct cpu *cpu, uint8_t reg_id, uint8_t val)  {
    switch (reg_id) {
        case 0: cpu->reg.ax[0] = val; return;
        case 1: cpu->reg.cx[0] = val; return;
        case 2: cpu->reg.dx[0] = val; return;
        case 3: cpu->reg.bx[0] = val; return;
        case 4: cpu->reg.ax[1] = val; return;
        case 5: cpu->reg.cx[1] = val; return;
        case 6: cpu->reg.dx[1] = val; return;
        case 7: cpu->reg.bx[1] = val; return;
        default: return;
    }
}

static inline void opcode_set_word_register(struct cpu *cpu, uint8_t reg_id, uint16_t val)  {
    switch (reg_id) {
        case 0: opcode_set_reg16_val(cpu->reg.ax, val); return;
        case 1: opcode_set_reg16_val(cpu->reg.cx, val); return;
        case 2: opcode_set_reg16_val(cpu->reg.dx, val); return;
        case 3: opcode_set_reg16_val(cpu->reg.bx, val); return;
        case 4: cpu->reg.sp = val; return;
        case 5: cpu->reg.bp = val; return;
        case 6: cpu->reg.si = val; return;
        case 7: cpu->reg.di = val; return;
        default: return;
    }
}

static inline uint8_t opcode_get_byte_registers_offset(struct cpu *cpu, uint8_t reg_id, uint16_t nex_op) {
    switch (reg_id) {
        case 0: return memory_read_byte(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.si + nex_op);
        case 1: return memory_read_byte(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.di + nex_op);
        case 2: return memory_read_byte(cpu, cpu->reg.bp + cpu->reg.si + nex_op);
        case 3: return memory_read_byte(cpu, cpu->reg.bp + cpu->reg.di + nex_op);
        case 4: return memory_read_byte(cpu, cpu->reg.si + nex_op);
        case 5: return memory_read_byte(cpu, cpu->reg.bp + nex_op);
        case 6: return memory_read_byte(cpu, cpu->reg.di + nex_op);
        case 7: return memory_read_byte(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + nex_op);
        default: return -1;
    }
}

static inline uint8_t opcode_get_byte_registers(struct cpu *cpu, uint8_t reg_id) {
    switch (reg_id) {
        case 0: return memory_read_byte(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.si);
        case 1: return memory_read_byte(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.di);
        case 2: return memory_read_byte(cpu, cpu->reg.bp + cpu->reg.si);
        case 3: return memory_read_byte(cpu, cpu->reg.bp + cpu->reg.di);
        case 4: return memory_read_byte(cpu, cpu->reg.si);
        case 5: return memory_read_byte(cpu, cpu->reg.bp);
        case 6: return memory_read_byte(cpu, cpu->reg.di);
        case 7: return memory_read_byte(cpu, opcode_reg8_to_reg16(cpu->reg.bx));
        default: return -1;
    }
}

static inline void opcode_set_byte_registers_offset(struct cpu *cpu, uint8_t reg_id, uint16_t nex_op, uint8_t byte) {
    switch (reg_id) {
        case 0: return memory_write_byte(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.si + nex_op, byte);
        case 1: return memory_write_byte(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.di + nex_op, byte);
        case 2: return memory_write_byte(cpu, cpu->reg.bp + cpu->reg.si + nex_op, byte);
        case 3: return memory_write_byte(cpu, cpu->reg.bp + cpu->reg.di + nex_op, byte);
        case 4: return memory_write_byte(cpu, cpu->reg.si + nex_op, byte);
        case 5: return memory_write_byte(cpu, cpu->reg.bp + nex_op, byte);
        case 6: return memory_write_byte(cpu, cpu->reg.di + nex_op, byte);
        case 7: return memory_write_byte(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + nex_op, byte);
        default: return;
    }
}

static inline void opcode_set_byte_registers(struct cpu *cpu, uint8_t reg_id, uint8_t byte) {
    switch (reg_id) {
        case 0: return memory_write_byte(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.si, byte);
        case 1: return memory_write_byte(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.di, byte);
        case 2: return memory_write_byte(cpu, cpu->reg.bp + cpu->reg.si, byte);
        case 3: return memory_write_byte(cpu, cpu->reg.bp + cpu->reg.di, byte);
        case 4: return memory_write_byte(cpu, cpu->reg.si, byte);
        case 5: return memory_write_byte(cpu, cpu->reg.bp, byte);
        case 6: return memory_write_byte(cpu, cpu->reg.di, byte);
        case 7: return memory_write_byte(cpu, opcode_reg8_to_reg16(cpu->reg.bx), byte);
        default: return;
    }
}

static inline uint16_t opcode_get_word_registers_offset(struct cpu *cpu, uint8_t reg_id, uint16_t nex_op) {
    switch (reg_id) {
        case 0: return memory_read_word(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.si + nex_op);
        case 1: return memory_read_word(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.di + nex_op);
        case 2: return memory_read_word(cpu, cpu->reg.bp + cpu->reg.si + nex_op);
        case 3: return memory_read_word(cpu, cpu->reg.bp + cpu->reg.di + nex_op);
        case 4: return memory_read_word(cpu, cpu->reg.si + nex_op);
        case 5: return memory_read_word(cpu, cpu->reg.bp + nex_op);
        case 6: return memory_read_word(cpu, cpu->reg.di + nex_op);
        case 7: return memory_read_word(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + nex_op);
        default: return -1;
    }
}

static inline uint16_t opcode_get_word_registers(struct cpu *cpu, uint8_t reg_id) {
    switch (reg_id) {
        case 0: return memory_read_word(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.si);
        case 1: return memory_read_word(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.di);
        case 2: return memory_read_word(cpu, cpu->reg.bp + cpu->reg.si);
        case 3: return memory_read_word(cpu, cpu->reg.bp + cpu->reg.di);
        case 4: return memory_read_word(cpu, cpu->reg.si);
        case 5: return memory_read_word(cpu, cpu->reg.bp);
        case 6: return memory_read_word(cpu, cpu->reg.di);
        case 7: return memory_read_word(cpu, opcode_reg8_to_reg16(cpu->reg.bx));
        default: return -1;
    }
}

static inline void opcode_set_word_registers_offset(struct cpu *cpu, uint8_t reg_id, uint16_t nex_op, uint16_t word) {
    switch (reg_id) {
        case 0: return memory_write_word(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.si + nex_op, word);
        case 1: return memory_write_word(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.di + nex_op, word);
        case 2: return memory_write_word(cpu, cpu->reg.bp + cpu->reg.si + nex_op, word);
        case 3: return memory_write_word(cpu, cpu->reg.bp + cpu->reg.di + nex_op, word);
        case 4: return memory_write_word(cpu, cpu->reg.si + nex_op, word);
        case 5: return memory_write_word(cpu, cpu->reg.bp + nex_op, word);
        case 6: return memory_write_word(cpu, cpu->reg.di + nex_op, word);
        case 7: return memory_write_word(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + nex_op, word);
        default: return;
    }
}

static inline void opcode_set_word_registers(struct cpu *cpu, uint8_t reg_id, uint16_t word) {
    switch (reg_id) {
        case 0: return memory_write_word(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.si, word);
        case 1: return memory_write_word(cpu, opcode_reg8_to_reg16(cpu->reg.bx) + cpu->reg.di, word);
        case 2: return memory_write_word(cpu, cpu->reg.bp + cpu->reg.si, word);
        case 3: return memory_write_word(cpu, cpu->reg.bp + cpu->reg.di, word);
        case 4: return memory_write_word(cpu, cpu->reg.si, word);
        case 5: return memory_write_word(cpu, cpu->reg.bp, word);
        case 6: return memory_write_word(cpu, cpu->reg.di, word);
        case 7: return memory_write_word(cpu, opcode_reg8_to_reg16(cpu->reg.bx), word);
        default: return;
    }
}

static inline uint8_t opcode_decode_mod_rm8l_and_read(struct cpu *cpu, uint8_t reg_id) {
    uint8_t nex_op = 0, nex_op1 = 0;
    switch (reg_id >> 6) {
        case 0b11:
            return opcode_get_byte_register(cpu, reg_id & ((1 << 3) - 1));
        case 0b10:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op1 = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_get_byte_registers_offset(cpu, reg_id & ((1 << 3) - 1), ((nex_op1 << 8) & (nex_op & 0xff)));
        case 0b01:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_get_byte_registers_offset(cpu, reg_id & ((1 << 3) - 1), nex_op);
        case 0b00:
            return opcode_get_byte_registers(cpu, reg_id & ((1 << 3) - 1));
        default:
            return -1;
    }
}

static inline uint8_t opcode_decode_mod_rm8h_and_read(struct cpu *cpu, uint8_t reg_id) {
    uint8_t nex_op = 0, nex_op1 = 0;
    switch (reg_id >> 6) {
        case 0b11:
            return opcode_get_byte_register(cpu, ((reg_id & 0b00111000) >> 3));
        case 0b10:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op1 = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_get_byte_registers_offset(cpu, ((reg_id & 0b00111000) >> 3), ((nex_op1 << 8) & (nex_op & 0xff)));
        case 0b01:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_get_byte_registers_offset(cpu, ((reg_id & 0b00111000) >> 3), nex_op);
        case 0b00:
            return opcode_get_byte_registers(cpu, ((reg_id & 0b00111000) >> 3));
        default:
            return -1;
    }
}

static inline void opcode_decode_mod_rm8l_and_write(struct cpu *cpu, uint8_t reg_id, uint8_t val) {
    uint8_t nex_op = 0, nex_op1 = 0;
    switch (reg_id >> 6) {
        case 0b11:
            return opcode_set_byte_register(cpu, reg_id & ((1 << 3) - 1), val);
        case 0b10:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op1 = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_set_byte_registers_offset(cpu, reg_id & ((1 << 3) - 1), ((nex_op1 << 8) & (nex_op & 0xff)), val);
        case 0b01:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_set_byte_registers_offset(cpu, reg_id & ((1 << 3) - 1), nex_op, val);
        case 0b00:
            return opcode_set_byte_registers(cpu, reg_id & ((1 << 3) - 1), val);
        default:
            return;
    }
}

static inline void opcode_decode_mod_rm8h_and_write(struct cpu *cpu, uint8_t reg_id, uint8_t val) {
    uint8_t nex_op = 0, nex_op1 = 0;
    switch (reg_id >> 6) {
        case 0b11:
            return opcode_set_byte_register(cpu, ((reg_id & 0b00111000) >> 3), val);
        case 0b10:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op1 = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_set_byte_registers_offset(cpu, ((reg_id & 0b00111000) >> 3), ((nex_op1 << 8) & (nex_op & 0xff)), val);
        case 0b01:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_set_byte_registers_offset(cpu, ((reg_id & 0b00111000) >> 3), nex_op, val);
        case 0b00:
            return opcode_set_byte_registers(cpu, ((reg_id & 0b00111000) >> 3), val);
        default:
            return;
    }
}

static inline uint16_t opcode_decode_mod_rm16l_and_read(struct cpu *cpu, uint8_t reg_id) {
    uint8_t nex_op = 0, nex_op1 = 0;
    switch (reg_id >> 6) {
        case 0b11:
            return opcode_get_word_register(cpu, reg_id & ((1 << 3) - 1));
        case 0b10:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op1 = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_get_word_registers_offset(cpu, reg_id & ((1 << 3) - 1), ((nex_op1 << 8) & (nex_op & 0xff)));
        case 0b01:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_get_word_registers_offset(cpu, reg_id & ((1 << 3) - 1), nex_op);
        case 0b00:
            return opcode_get_word_registers(cpu, reg_id & ((1 << 3) - 1));
        default:
            return -1;
    }
}

static inline uint16_t opcode_decode_mod_rm16h_and_read(struct cpu *cpu, uint8_t reg_id) {
    uint8_t nex_op = 0, nex_op1 = 0;
    switch (reg_id >> 6) {
        case 0b11:
            return opcode_get_word_register(cpu, ((reg_id & 0b00111000) >> 3));
        case 0b10:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op1 = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_get_word_registers_offset(cpu, ((reg_id & 0b00111000) >> 3), ((nex_op1 << 8) & (nex_op & 0xff)));
        case 0b01:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_get_word_registers_offset(cpu, ((reg_id & 0b00111000) >> 3), nex_op);
        case 0b00:
            return opcode_get_word_registers(cpu, ((reg_id & 0b00111000) >> 3));
        default:
            return -1;
    }
}

static inline void opcode_decode_mod_rm16l_and_write(struct cpu *cpu, uint8_t reg_id, uint16_t val) {
    uint8_t nex_op = 0, nex_op1 = 0;
    switch (reg_id >> 6) {
        case 0b11:
            return opcode_set_word_register(cpu, reg_id & ((1 << 3) - 1), val);
        case 0b10:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op1 = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_set_word_registers_offset(cpu, reg_id & ((1 << 3) - 1), ((nex_op1 << 8) & (nex_op & 0xff)), val);
        case 0b01:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_set_word_registers_offset(cpu, reg_id & ((1 << 3) - 1), nex_op, val);
        case 0b00:
            return opcode_set_word_registers(cpu, reg_id & ((1 << 3) - 1), val);
        default:
            return;
    }
}

static inline void opcode_decode_mod_rm16h_and_write(struct cpu *cpu, uint8_t reg_id, uint8_t val) {
    uint8_t nex_op = 0, nex_op1 = 0;
    switch (reg_id >> 6) {
        case 0b11:
            return opcode_set_byte_register(cpu, ((reg_id & 0b00111000) >> 3), val);
        case 0b10:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op1 = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_set_byte_registers_offset(cpu, ((reg_id & 0b00111000) >> 3), ((nex_op1 << 8) & (nex_op & 0xff)), val);
        case 0b01:
            cpu->reg.ip++;
            cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
            nex_op = memory_read_byte(cpu, cpu->reg.ip32);
            return opcode_set_byte_registers_offset(cpu, ((reg_id & 0b00111000) >> 3), nex_op, val);
        case 0b00:
            return opcode_set_byte_registers(cpu, ((reg_id & 0b00111000) >> 3), val);
        default:
            return;
    }
}

static inline void opcode_push(struct cpu *cpu, uint16_t val) {
    memory_write_word(cpu, cpu->reg.ss * 16 + cpu->reg.sp, val);
    cpu->reg.sp -= 2;
}

static inline uint16_t opcode_pop(struct cpu *cpu) {
    cpu->reg.sp += 2;
    uint16_t t = memory_read_word(cpu, cpu->reg.ss * 16 + cpu->reg.sp);
    return t;
}

static inline void opcode_set_flags_based_on_result(struct cpu *cpu, uint16_t val) {
    if (!val) cpu->reg.flags |= CPU_FLAGS_ZERO;
    else cpu->reg.flags &= ~(CPU_FLAGS_ZERO);

    if (val & 0x8000) { cpu->reg.flags |= CPU_FLAGS_SIGN; }
    else { cpu->reg.flags &= ~(CPU_FLAGS_SIGN); }

    if (parity_table[val & 0xff]) { cpu->reg.flags |= CPU_FLAGS_PARITY; }
    else cpu->reg.flags &= ~(CPU_FLAGS_PARITY);
}

static inline uint16_t opcode_add(struct cpu *cpu, uint16_t a, uint16_t b) {
    uint32_t res = a + b;

    opcode_set_flags_based_on_result(cpu, res);

    if (res & 0xFFFF0000) { cpu->reg.flags |= CPU_FLAGS_CARRY ; }
    else cpu->reg.flags &= ~(CPU_FLAGS_CARRY);

    if (((res ^ a) & (res ^ b) & 0x8000) == 0x8000) { cpu->reg.flags |= CPU_FLAGS_OVERFLOW ; }
    else cpu->reg.flags &= ~(CPU_FLAGS_OVERFLOW);

    if (((a ^ b ^ res) & 0x10) == 0x10) { cpu->reg.flags |= CPU_FLAGS_ACARRY ; }
    else cpu->reg.flags &= ~(CPU_FLAGS_ACARRY);

    return res;
}

static inline uint16_t opcode_sub(struct cpu *cpu, uint16_t a, uint16_t b) {
    uint32_t res = a - b;

    opcode_set_flags_based_on_result(cpu, res);

    if (res & 0xFFFF0000) { cpu->reg.flags |= CPU_FLAGS_CARRY ; }
    else cpu->reg.flags &= ~(CPU_FLAGS_CARRY);

    if (((res ^ a) & (a ^ b) & 0x8000) == 0x8000) { cpu->reg.flags |= CPU_FLAGS_OVERFLOW ; }
    else cpu->reg.flags &= ~(CPU_FLAGS_OVERFLOW);

    if (((a ^ b ^ res) & 0x10) == 0x10) { cpu->reg.flags |= CPU_FLAGS_ACARRY ; }
    else cpu->reg.flags &= ~(CPU_FLAGS_ACARRY);

    return res;
}

// START OF OPCODE IMPLEMENTATIONS

static void opcode_hlt(struct cpu *cpu) {
    cpu->state |= CPU_HALTED;
    debug_print("[*] Halting the CPU\n");
}

static void opcode_xorrm8(struct cpu *cpu, uint8_t op0, uint8_t op1) {
    uint8_t a = opcode_decode_mod_rm8l_and_read(cpu, op0);
    uint8_t b = opcode_decode_mod_rm8h_and_read(cpu, op0);

    uint8_t result = a ^ b;
    opcode_set_flags_based_on_result(cpu, result);

    opcode_decode_mod_rm8l_and_write(cpu, op0, result);
}

static void opcode_xorrm16(struct cpu *cpu, uint8_t op0, uint8_t op1) {
    uint16_t a = opcode_decode_mod_rm16l_and_read(cpu, op0);
    uint16_t b = opcode_decode_mod_rm16h_and_read(cpu, op0);

    uint16_t result = a ^ b;
    opcode_set_flags_based_on_result(cpu, result);

    opcode_decode_mod_rm16l_and_write(cpu, op0, result);
}

static void opcode_andrm8(struct cpu *cpu, uint8_t op0, uint8_t op1) {
    uint8_t a = opcode_decode_mod_rm8l_and_read(cpu, op0);
    uint8_t b = opcode_decode_mod_rm8h_and_read(cpu, op0);

    uint8_t result = a & b;
    opcode_set_flags_based_on_result(cpu, result);

    opcode_decode_mod_rm8l_and_write(cpu, op0, result);
}

static void opcode_andrm16(struct cpu *cpu, uint8_t op0, uint8_t op1) {
    uint16_t a = opcode_decode_mod_rm16l_and_read(cpu, op0);
    uint16_t b = opcode_decode_mod_rm16h_and_read(cpu, op0);

    uint16_t result = a & b;
    opcode_set_flags_based_on_result(cpu, result);

    opcode_decode_mod_rm16l_and_write(cpu, op0, result);
}

static void opcode_orrm8(struct cpu *cpu, uint8_t op0, uint8_t op1) {
    uint8_t a = opcode_decode_mod_rm8l_and_read(cpu, op0);
    uint8_t b = opcode_decode_mod_rm8h_and_read(cpu, op0);

    uint8_t result = a | b;
    opcode_set_flags_based_on_result(cpu, result);

    opcode_decode_mod_rm8l_and_write(cpu, op0, result);
}

static void opcode_orrm16(struct cpu *cpu, uint8_t op0, uint8_t op1) {
    uint16_t a = opcode_decode_mod_rm16l_and_read(cpu, op0);
    uint16_t b = opcode_decode_mod_rm16h_and_read(cpu, op0);

    uint16_t result = a | b;
    opcode_set_flags_based_on_result(cpu, result);

    opcode_decode_mod_rm16l_and_write(cpu, op0, result);
}

static void opcode_addrm8(struct cpu *cpu, uint8_t op0, uint8_t op1) {
    uint8_t a = opcode_decode_mod_rm8l_and_read(cpu, op0);
    uint8_t b = opcode_decode_mod_rm8h_and_read(cpu, op0);

    uint16_t result = opcode_add(cpu, a, b);

    opcode_decode_mod_rm8l_and_write(cpu, op0, result);
}

static void opcode_addrm16(struct cpu *cpu, uint8_t op0, uint8_t op1) {
    uint16_t a = opcode_decode_mod_rm16l_and_read(cpu, op0);
    uint16_t b = opcode_decode_mod_rm16h_and_read(cpu, op0);

    uint16_t result = opcode_add(cpu, a, b);

    opcode_decode_mod_rm8l_and_write(cpu, op0, result);
}

static void opcode_pushax(struct cpu *cpu) {
    opcode_push(cpu,opcode_reg8_to_reg16(cpu->reg.ax));
}

static void opcode_pushcx(struct cpu *cpu) {
    opcode_push(cpu,opcode_reg8_to_reg16(cpu->reg.cx));
}
static void opcode_pushdx(struct cpu *cpu) {
    opcode_push(cpu,opcode_reg8_to_reg16(cpu->reg.dx));
}
static void opcode_pushbx(struct cpu *cpu) {
    opcode_push(cpu,opcode_reg8_to_reg16(cpu->reg.bx));
}

static void opcode_pushsp(struct cpu *cpu) {
    opcode_push(cpu,cpu->reg.sp);
}

static void opcode_pushbp(struct cpu *cpu) {
    opcode_push(cpu,cpu->reg.bp);
}
static void opcode_pushsi(struct cpu *cpu) {
    opcode_push(cpu,cpu->reg.si);
}
static void opcode_pushdi(struct cpu *cpu) {
    opcode_push(cpu,cpu->reg.di);
}

static void opcode_popax(struct cpu *cpu) {
    uint16_t t =  opcode_pop(cpu);
    opcode_set_reg16_val(cpu->reg.ax, t);
}

static void opcode_popcx(struct cpu *cpu) {
    uint16_t t =  opcode_pop(cpu);
    opcode_set_reg16_val(cpu->reg.cx, t);
}
static void opcode_popdx(struct cpu *cpu) {
    uint16_t t =  opcode_pop(cpu);
    opcode_set_reg16_val(cpu->reg.dx, t);
}
static void opcode_popbx(struct cpu *cpu) {
    uint16_t t =  opcode_pop(cpu);
    opcode_set_reg16_val(cpu->reg.bx, t);
}

static void opcode_popsp(struct cpu *cpu) {
    cpu->reg.sp = opcode_pop(cpu);
}

static void opcode_popbp(struct cpu *cpu) {
    cpu->reg.bp = opcode_pop(cpu);
}
static void opcode_popsi(struct cpu *cpu) {
    cpu->reg.si = opcode_pop(cpu);
}
static void opcode_popdi(struct cpu *cpu) {
    cpu->reg.di = opcode_pop(cpu);
}

static void opcode_incax(struct cpu *cpu) {
    uint16_t res = opcode_add(cpu, opcode_reg8_to_reg16(cpu->reg.ax), 1);
    opcode_set_reg16_val(cpu->reg.ax, res);
}

static void opcode_inccx(struct cpu *cpu) {
    uint16_t res = opcode_add(cpu, opcode_reg8_to_reg16(cpu->reg.cx), 1);
    opcode_set_reg16_val(cpu->reg.cx, res);
}

static void opcode_incdx(struct cpu *cpu) {
    uint16_t res = opcode_add(cpu, opcode_reg8_to_reg16(cpu->reg.dx), 1);
    opcode_set_reg16_val(cpu->reg.dx, res);
}

static void opcode_incbx(struct cpu *cpu) {
    uint16_t res = opcode_add(cpu, opcode_reg8_to_reg16(cpu->reg.bx), 1);
    opcode_set_reg16_val(cpu->reg.bx, res);
}

static void opcode_incsp(struct cpu *cpu) {
    uint16_t res = opcode_add(cpu, cpu->reg.sp, 1);
    cpu->reg.sp = res;
}

static void opcode_incbp(struct cpu *cpu) {
    uint16_t res = opcode_add(cpu, cpu->reg.bp, 1);
    cpu->reg.bp = res;
}

static void opcode_incsi(struct cpu *cpu) {
    uint16_t res = opcode_add(cpu, cpu->reg.bp, 1);
    cpu->reg.bp = res;
}

static void opcode_incdi(struct cpu *cpu) {
    uint16_t res = opcode_add(cpu, cpu->reg.di, 1);
    cpu->reg.di = res;
}

static void opcode_decax(struct cpu *cpu) {
    uint16_t res = opcode_sub(cpu, opcode_reg8_to_reg16(cpu->reg.ax), 1);
    opcode_set_reg16_val(cpu->reg.ax, res);
}

static void opcode_deccx(struct cpu *cpu) {
    uint16_t res = opcode_sub(cpu, opcode_reg8_to_reg16(cpu->reg.cx), 1);
    opcode_set_reg16_val(cpu->reg.cx, res);
}

static void opcode_decdx(struct cpu *cpu) {
    uint16_t res = opcode_sub(cpu, opcode_reg8_to_reg16(cpu->reg.dx), 1);
    opcode_set_reg16_val(cpu->reg.dx, res);
}

static void opcode_decbx(struct cpu *cpu) {
    uint16_t res = opcode_sub(cpu, opcode_reg8_to_reg16(cpu->reg.bx), 1);
    opcode_set_reg16_val(cpu->reg.bx, res);
}

static void opcode_decsp(struct cpu *cpu) {
    uint16_t res = opcode_sub(cpu, cpu->reg.sp, 1);
    cpu->reg.sp = res;
}

static void opcode_decbp(struct cpu *cpu) {
    uint16_t res = opcode_sub(cpu, cpu->reg.bp, 1);
    cpu->reg.bp = res;
}

static void opcode_decsi(struct cpu *cpu) {
    uint16_t res = opcode_sub(cpu, cpu->reg.bp, 1);
    cpu->reg.bp = res;
}

static void opcode_decdi(struct cpu *cpu) {
    uint16_t res = opcode_sub(cpu, cpu->reg.di, 1);
    cpu->reg.di = res;
}

// END OF OPCODE IMPLEMENTATIONS


/*
    addr8 = 8-bit address of I/O port
    reg8 = AL = 0, CL = 1, DL = 2, BL = 3, AH =4, CH = 5, DH = 6, BH = 7
    reg16 = AX = 0, CX =1, DX =2, BX =3, SP = 4, BP = 5, SI = 6, DI = 7
    sreg = ES = 0, CS = 1, SS = 2, DS = 3
    mem8 = memory byte (direct addressing only)
    mem16 = memory word (direct addressing only)
    r/m8 = reg8 or mem8
    r/m16 = reg16 or mem16
    imm8 = 8 bit immediate
    imm16 = 16 bit immediate
 */

const struct opcode opcodes[256] = {
        {"ADD r/m8, r8", 2, opcode_addrm8},
        {"ADD r/m16, r16", 2, opcode_addrm16},
        {"ADD r8, r/m8", 2, NULL},
        {"ADD r16, r/m16", 2, NULL},
        {"ADD al, imm8", 1, NULL},
        {"ADD ax, imm16", 2, NULL},
        {"PUSH es", 0, NULL},
        {"POP es", 0, NULL},
        {"OR r/m8, r8", 2, opcode_orrm8},
        {"OR r/m16, r16", 2, opcode_orrm16},
        {"OR r8, r/m8", 2, NULL},
        {"OR r16, r/m16", 2, NULL},
        {"OR al, imm8", 1, NULL},
        {"OR ax, imm16", 2, NULL},
        {"PUSH cs", 0, NULL},
        {"POP cs", 0, NULL},
        {"ADC r/m8, r8", 2, NULL},
        {"ADC r/m16, r16", 2, NULL},
        {"ADC r8, r/m8", 2, NULL},
        {"ADC r16, r/m16", 2, NULL},
        {"ADC al, imm8", 1, NULL},
        {"ADC ax, imm16", 2, NULL},
        {"PUSH ss", 0, NULL},
        {"POP ss", 0, NULL},
        {"SBB r/m8, r8", 2, NULL},
        {"SBB r/m16, r16", 2, NULL},
        {"SBB r8, r/m8", 2, NULL},
        {"SBB r16, r/m16", 2, NULL},
        {"SBB al, imm8", 1, NULL},
        {"SBB ax, imm8", 1, NULL},
        {"PUSH ds", 0, NULL},
        {"POP ds", 0, NULL},
        {"AND r/m8, r8", 2, opcode_andrm8},
        {"AND r/m16, r16", 2, opcode_andrm16},
        {"AND r8, r/m8", 2, NULL},
        {"AND r16, r/m16", 2, NULL},
        {"AND al, imm8", 1, NULL},
        {"AND ax, imm16", 2, NULL},
        {"DAA", 0, NULL},
        {"SUB r/m8, r8", 2, NULL},
        {"SUB r/m16, r16", 2, NULL},
        {"SUB r8, r/m8", 2, NULL},
        {"SUB r16, r/m16", 2, NULL},
        {"SUB al, imm8", 1, NULL},
        {"SUB ax, imm16", 2, NULL},
        {"", 0, NULL},
        {"", 0, NULL},
        {"DAS", 0, NULL},
        {"XOR r/m8, r8", 2, opcode_xorrm8},
        {"XOR r/m16, r16", 2, opcode_xorrm16},
        {"XOR r8, r/m8", 2, NULL},
        {"XOR r16, r/m16", 2, NULL},
        {"XOR al, imm8", 1, NULL},
        {"XOR ax, imm16", 2, NULL},
        {"", 0, NULL},
        {"AAA", 0, NULL},
        {"CMP r/m8, r8", 2, NULL},
        {"CMP r/m16, r16", 2, NULL},
        {"CMP r8, r/m8", 2, NULL},
        {"CMP r16, r/m16", 2, NULL},
        {"CMP al, imm8", 1, NULL},
        {"CMP ax, imm16", 2, NULL},
        {"", 0, NULL},
        {"AAS", 0, NULL},
        {"INC ax", 0, opcode_incax},
        {"INC cx", 0, opcode_inccx},
        {"INC dx", 0, opcode_incdx},
        {"INC bx", 0, opcode_incbx},
        {"INC sp", 0, opcode_incsp},
        {"INC bp", 0, opcode_incbp},
        {"INC si", 0, opcode_incsi},
        {"INC di", 0, opcode_incdi},
        {"DEC ax", 0, opcode_decax},
        {"DEC cx", 0, opcode_deccx},
        {"DEC dx", 0, opcode_decdx},
        {"DEC bx", 0, opcode_decbx},
        {"DEC sp", 0, opcode_decsp},
        {"DEC bp", 0, opcode_decbp},
        {"DEC si", 0, opcode_decsi},
        {"DEC di", 0, opcode_decdi},
        {"PUSH ax", 0, opcode_pushax},
        {"PUSH cx", 0, opcode_pushcx},
        {"PUSH dx", 0, opcode_pushdx},
        {"PUSH bx", 0, opcode_pushbx},
        {"PUSH sp", 0, opcode_pushsp},
        {"PUSH bp", 0, opcode_pushbp},
        {"PUSH si", 0, opcode_pushsi},
        {"PUSH di", 0, opcode_pushdi},
        {"POP ax", 0, opcode_popax},
        {"POP cx", 0, opcode_popcx},
        {"POP dx", 0, opcode_popdx},
        {"POP bx", 0, opcode_popbx},
        {"POP sp", 0, opcode_popsp},
        {"POP bp", 0, opcode_popbp},
        {"POP si", 0, opcode_popsi},
        {"POP di", 0, opcode_popdi},
        {"PUSHA", 0, NULL},
        {"POPA", 0, NULL},
        {"BOUND r16, m16", 3, NULL},
        {"", 0, NULL},
        {"", 0, NULL},
        {"", 0, NULL},
        {"", 0, NULL},
        {"", 0, NULL},
        {"PUSH imm16", 1, NULL},
        {"IMUL r16, r/m16, imm16", 4, NULL},
        {"PUSH imm8", 1, NULL},
        {"IMUL r16, r/m16, imm8", 3, NULL},
        {"INSB", 0, NULL},
        {"INSW", 0, NULL},
        {"OUTSB", 0, NULL},
        {"OUTSW", 0, NULL},
        {"JO rel8", 1, NULL},
        {"JNO rel8", 1, NULL},
        {"JB rel8", 1, NULL},
        {"JNB rel8", 1, NULL},
        {"JZ rel8", 1, NULL},
        {"JNZ rel8", 1, NULL},
        {"JBE rel8", 1, NULL},
        {"JA rel8", 1, NULL},
        {"JS rel8", 1, NULL},
        {"JNS rel8", 1, NULL},
        {"JPE rel8", 1, NULL},
        {"JPO rel8", 1, NULL},
        {"JL rel8", 1, NULL},
        {"JGE rel8", 1, NULL},
        {"JLE rel8", 1, NULL},
        {"JG rel8", 1, NULL},
        {"GRP1 r/m8, imm8", 2, NULL},
        {"GRP1 r/m16, imm8", 2, NULL},
        {"GRP1 r/m8, imm8", 2, NULL},
        {"GRP1 r/m16, imm8", 2, NULL},
        {"TEST r8, r/m8", 2, NULL},
        {"TEST r16, r/m16", 2, NULL},
        {"XCHG r8, r/m8", 2, NULL},
        {"XCHG r16, r/m16", 2, NULL},
        {"MOV r/m8, r8", 2, NULL},
        {"MOV r/m16, r16", 2, NULL},
        {"MOV r8, r/m8", 2, NULL},
        {"MOV r16, r/m16", 2, NULL},
        {"MOV r/m16, sreg", 2, NULL},
        {"LEA r16, mem16", 2, NULL},
        {"MOV sreg, r/m16", 2, NULL},
        {"POP r/m16", 1, NULL},
        {"NOP", 0, NULL},
        {"XCHG CX, AX", 0, NULL},
        {"XCHG DX, AX", 0, NULL},
        {"XCHG BX, AX", 0, NULL},
        {"XCHG SP, AX", 0, NULL},
        {"XCHG BP, AX", 0, NULL},
        {"XCHG SI, AX", 0, NULL},
        {"XCHG DI, AX", 0, NULL},
        {"CBW", 0, NULL},
        {"CWD", 0, NULL},
        {"CALL m16:16", 2, NULL},
        {"WAIT", 0, NULL},
        {"PUSHF", 0, NULL},
        {"POPF", 0, NULL},
        {"SAHF", 0, NULL},
        {"LAHF", 0, NULL},
        {"MOV al, moffs8", 1, NULL},
        {"MOV ax, moffs16", 1, NULL},
        {"MOV moffs8, al", 1, NULL},
        {"MOV moffs16, ax", 1, NULL},
        {"MOVSB", 0, NULL},
        {"MOVSW", 0, NULL},
        {"CMPSB", 0, NULL},
        {"CMPSW", 0, NULL},
        {"TEST al, imm8", 1, NULL},
        {"TEST ax, imm16", 2, NULL},
        {"STOSB", 0, NULL},
        {"STOSW", 0, NULL},
        {"LODSB", 0, NULL},
        {"LODSW", 0, NULL},
        {"SCASB", 0, NULL},
        {"SCASW", 0, NULL},
        {"MOV al, imm8", 1, NULL},
        {"MOV cl, imm8", 1, NULL},
        {"MOV dl, imm8", 1, NULL},
        {"MOV bl, imm8", 1, NULL},
        {"MOV ah, imm8", 1, NULL},
        {"MOV ch, imm8", 1, NULL},
        {"MOV dh, imm8", 1, NULL},
        {"MOV bh, imm8", 1, NULL},
        {"MOV ax, imm16", 2, NULL},
        {"MOV cx, imm16", 2, NULL},
        {"MOV dx, imm16", 2, NULL},
        {"MOV bx, imm16", 2, NULL},
        {"MOV sp, imm16", 2, NULL},
        {"MOV bp, imm16", 2, NULL},
        {"MOV si, imm16", 2, NULL},
        {"MOV di, imm16", 2, NULL},
        {"GRP2 r/m8, imm8", 1, NULL},
        {"GRP2 r/m16, imm8", 1, NULL},
        {"RET imm16", 2, NULL},
        {"RET", 0, NULL},
        {"LES r16, m16:16", 3, NULL},
        {"LDS r16, m16:16", 3, NULL},
        {"MOV r8, m8", 2, NULL},
        {"MOV r16, m16", 2, NULL},
        {"ENTER", 0, NULL},
        {"LEAVE", 0, NULL},
        {"RETF imm16", 2, NULL},
        {"RETF", 0, NULL},
        {"INT3", 0, NULL},
        {"INT imm8", 1, NULL},
        {"INTO", 0, NULL},
        {"IRET", 0, NULL},
        {"GRP2 r/m8, 1", 1, NULL},
        {"GRP2 r/m16, 1", 1, NULL},
        {"GRP2 r/m8, cl", 1, NULL},
        {"GRP2 r/m16, cl", 1, NULL},
        {"AAM imm8", 1, NULL},
        {"AAD imm8", 1, NULL},
        {"SALC", 0, NULL},
        {"XLAT", 0, NULL},
        {"[x87ONLY]", 0, NULL},
        {"[x87ONLY]", 0, NULL},
        {"[x87ONLY]", 0, NULL},
        {"[x87ONLY]", 0, NULL},
        {"[x87ONLY]", 0, NULL},
        {"[x87ONLY]", 0, NULL},
        {"[x87ONLY]", 0, NULL},
        {"[x87ONLY]", 0, NULL},
        {"LOOPNZ rel8", 1, NULL},
        {"LOOPZ rel8", 1, NULL},
        {"LOOP rel8", 1, NULL},
        {"JCXZ rel8", 1, NULL},
        {"IN al, imm8", 1, NULL},
        {"IN ax, imm8", 1, NULL},
        {"OUT imm8, al", 1, NULL},
        {"OUT imm8, ax", 1, NULL},
        {"CALL rel16", 1, NULL},
        {"JMP rel16", 1, NULL},
        {"JMP m16:16", 1, NULL},
        {"JMP rel8", 1, NULL},
        {"IN al, dx", 0, NULL},
        {"IN ax, dx", 0, NULL},
        {"OUT dx, al", 0, NULL},
        {"OUT dx, ax", 0, NULL},
        {"LOCK", 0, NULL},
        {"", 0, NULL},
        {"", 0, NULL},
        {"", 0, NULL},
        {"HLT", 0, opcode_hlt},
        {"CMC", 0, NULL},
        {"GRP3a r/m8", 1, NULL},
        {"GRP3b r/m16", 1, NULL},
        {"CLC", 0, NULL},
        {"STC", 0, NULL},
        {"CLI", 0, NULL},
        {"STI", 0, NULL},
        {"CLD", 0, NULL},
        {"STD", 0, NULL},
        {"GRP4 r/m8", 0, NULL},
        {"GRP5 r/m16", 0, NULL},
};

void opcode_execute(struct cpu *cpu) {
    uint8_t opcode_byte = memory_read_byte(cpu, cpu->reg.ip32);
    struct opcode opcode = opcodes[opcode_byte];

    if (opcode.function == NULL) {
        debug_print("[!] Not implemented or invalid instruction %#x (%s) hit, bailing out.\n", opcode_byte, opcode.name);
        cpu->state |= CPU_HALTED;
    } else {
        uint8_t op0, op1, op2, op3;
        printf("[*] %s\n", opcode.name);
        switch (opcode.operand_length) {
            case 0:
                ((void (*)(struct cpu *cpu))opcode.function)(cpu);
                break;
            case 1:
                op0 = memory_read_byte(cpu, cpu->reg.ip32 + 1);
                ((void (*)(struct cpu *cpu, uint8_t))opcode.function)(cpu, op0);
            case 2:
                op0 = memory_read_byte(cpu, cpu->reg.ip32 + 1);
                op1 = memory_read_byte(cpu, cpu->reg.ip32 + 2);
                ((void (*)(struct cpu *cpu, uint8_t, uint8_t))opcode.function)(cpu, op0, op1);
                break;
            case 3:
                op0 = memory_read_byte(cpu, cpu->reg.ip32 + 1);
                op1 = memory_read_byte(cpu, cpu->reg.ip32 + 2);
                op2 = memory_read_byte(cpu, cpu->reg.ip32 + 3);
                ((void (*)(struct cpu *cpu, uint8_t, uint8_t, uint8_t))opcode.function)(cpu, op0, op1, op2);
                break;
            case 4:
                op0 = memory_read_byte(cpu, cpu->reg.ip32 + 1);
                op1 = memory_read_byte(cpu, cpu->reg.ip32 + 2);
                op2 = memory_read_byte(cpu, cpu->reg.ip32 + 3);
                op3 = memory_read_byte(cpu, cpu->reg.ip32 + 4);
                ((void (*)(struct cpu *cpu, uint8_t, uint8_t, uint8_t, uint8_t))opcode.function)(cpu, op0, op1, op2, op3);
                break;
            default:
                return;
        }
    }

    if (opcode.operand_length) cpu->reg.ip += opcode.operand_length;
    else cpu->reg.ip++;

    // PhysicalAddress = Segment * 16 + Offset
    cpu->reg.ip32 = cpu->reg.cs * 16 + cpu->reg.ip;
}