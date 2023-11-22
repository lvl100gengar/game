// This module is an implementation of the ARMv4 Instruction Set

#ifndef __CPU_H__
#define __CPU_H__

#include "bits.h"
#include "memory.h"

#include <limits.h> // for CHAR_BIT
#include <memory.h>
#include <stdint.h> // for uint32_t
#include <stdio.h> // for printf

typedef uint32_t cpu_arm_instruction_t;
typedef uint16_t cpu_thumb_instruction_t;

typedef struct cpu_registers {
    uint32_t r[13]; // General Purpose Registers
    uint32_t sp; // Stack Pointer
    uint32_t lr; // Link Register
    uint32_t pc; // Program Counter

    // Current Program Status Register
    // 31: N (Negative)
    // 30: Z (Zero)
    // 29: C (Carry)
    // 28: V (Overflow)
    // 4-0: Mode bits
    uint32_t cpsr; // Current Program Status Register
    uint32_t spsr; // Saved Program Status Register
} cpu_registers_t;

// ARM Processor Mode constants
#define ARM_MODE_USER 0x10 // 10000
#define ARM_MODE_FIQ 0x11 // 10001
#define ARM_MODE_IRQ 0x12 // 10010
#define ARM_MODE_SUPERVISOR 0x13 // 10011
#define ARM_MODE_ABORT 0x17 // 10111
#define ARM_MODE_UNDEFINED 0x1B // 11011
#define ARM_MODE_SYSTEM 0x1F // 11111

uint8_t get_cpsr_mode(uint32_t cpsr)
{
    return cpsr & 0x1F;
}

void set_cpsr_mode(struct cpu_registers* registers, uint32_t value)
{
    registers->cpsr = (registers->cpsr & ~0x1F) | (value & 0x1F);
}

uint8_t get_cpsr_negative(uint32_t cpsr)
{
    return (cpsr >> 31) & 0x1;
}

void set_cpsr_negative(struct cpu_registers* registers, uint32_t value)
{
    registers->cpsr = (registers->cpsr & ~0x80000000) | ((value & 0x1) << 31);
}

uint8_t get_cpsr_zero(uint32_t cpsr)
{
    return (cpsr >> 30) & 0x1;
}

void set_cpsr_zero(struct cpu_registers* registers, uint32_t value)
{
    registers->cpsr = (registers->cpsr & ~0x40000000) | ((value & 0x1) << 30);
}

uint8_t get_cpsr_carry(uint32_t cpsr)
{
    return (cpsr >> 29) & 0x1;
}

void set_cpsr_carry(struct cpu_registers* registers, uint32_t value)
{
    registers->cpsr = (registers->cpsr & ~0x20000000) | ((value & 0x1) << 29);
}

uint8_t get_cpsr_overflow(uint32_t cpsr)
{
    return (cpsr >> 28) & 0x1;
}

void set_cpsr_overflow(struct cpu_registers* registers, uint32_t value)
{
    registers->cpsr = (registers->cpsr & ~0x10000000) | ((value & 0x1) << 28);
}

enum cpu_mode {
    USER = 0x10, // 10000
    FIQ = 0x11, // 10001
    IRQ = 0x12, // 10010
    SUPERVISOR = 0x13, // 10011
    ABORT = 0x17, // 10111
    UNDEFINED = 0x1B, // 11011
};

typedef struct cpu {
    cpu_registers_t registers;
    char* memory;
    // memory_t* memory_struct;
} cpu_t;

int cpu_check_condition(uint32_t cpsr, cpu_arm_instruction_t instruction)
{
    uint8_t cond = (instruction >> 28) & 0xF;

    printf("Condition: cond=0x%X result=", cond);

    switch (cond) {
    case 0x0: // EQ
        return get_cpsr_zero(cpsr);
    case 0x1: // NE
        return !get_cpsr_zero(cpsr);
    case 0x2: // CS/HS
        return get_cpsr_carry(cpsr);
    case 0x3: // CC/LO
        return !get_cpsr_carry(cpsr);
    case 0x4: // MI
        return get_cpsr_negative(cpsr);
    case 0x5: // PL
        return !get_cpsr_negative(cpsr);
    case 0x6: // VS
        return get_cpsr_overflow(cpsr);
    case 0x7: // VC
        return !get_cpsr_overflow(cpsr);
    case 0x8: // HI
        return get_cpsr_carry(cpsr) && !get_cpsr_zero(cpsr);
    case 0x9: // LS
        return !get_cpsr_carry(cpsr) || get_cpsr_zero(cpsr);
    case 0xA: // GE
        return get_cpsr_negative(cpsr) == get_cpsr_overflow(cpsr);
    case 0xB: // LT
        return get_cpsr_negative(cpsr) != get_cpsr_overflow(cpsr);
    case 0xC: // GT
        return !get_cpsr_zero(cpsr) && (get_cpsr_negative(cpsr) == get_cpsr_overflow(cpsr));
    case 0xD: // LE
        return get_cpsr_zero(cpsr) || (get_cpsr_negative(cpsr) != get_cpsr_overflow(cpsr));
    case 0xE: // AL
        return 1;
    case 0xF: // NV
        return 0;
    default:
        return 0;
    }
}

// Returns 1 if the instruction was executed, 0 if it was not
int cpu_process_arm_instruction(cpu_t* cpu, cpu_arm_instruction_t instruction)
{
    // Get a string of the current mode
    char* mode;
    switch (get_cpsr_mode(cpu->registers.cpsr)) {
    case ARM_MODE_USER:
        mode = "USER";
        break;
    case ARM_MODE_FIQ:  
        mode = "FIQ";
        break;
    case ARM_MODE_IRQ:
        mode = "IRQ";
        break;
    case ARM_MODE_SUPERVISOR:
        mode = "SUPERVISOR";
        break;
    case ARM_MODE_ABORT:
        mode = "ABORT";
        break;
    case ARM_MODE_UNDEFINED:
        mode = "UNDEFINED";
        break;
    default:
        mode = "UNKNOWN";
        break;
    }

    // Print the PC and instruction
    printf("[ARM][%s] SP=0x%X, PC=0x%X, Instruction=0x%X, ", mode, cpu->registers.sp, cpu->registers.pc, instruction);

    // if (cpu->registers.sp != 0) {
    //     printf("SP not 0\n");
    //     return 0;
    // }

    // Check the condition to see if we should execute the instruction
    if (!cpu_check_condition(cpu->registers.cpsr, instruction)) {
        printf("false\n");
        return 1;
    }
    printf("true\n");

    // Decode the instruction
    uint8_t instruction_type = (instruction >> 26) & 0x3; // Operation (0x0 = data processing, 0x1 = load/store, 0x2 = branch, 0x3 = coprocessor)
    uint8_t i = (instruction >> 25) & 0x1; // Immediate Operand (0 = register, 1 = immediate)

    switch (instruction_type) {
    case 0x0: {
        // If bits 27-4 are 0001 0010 1111 1111 1111 0001 this is a Branch and Exchange instruction
        if ((instruction & 0xFFFFFF0) == 0x12FFF10) {
            // Branch and Exchange
            uint8_t rn = (instruction >> 0) & 0xF; // Register

            // Determine if the mode is ARM or Thumb and set the PC and mode accordingly
            if (cpu->registers.r[rn] & 0x1) {
                // Thumb
                cpu->registers.pc = cpu->registers.r[rn] & 0xFFFFFFFE;
                cpu->registers.pc -= 2; // TODO: this is a hack to make the PC correct
                cpu->registers.cpsr &= ~0x20;
                printf("BX: rs=%d, pc=%d, mode=THUMB\n", cpu->registers.r[rn], cpu->registers.pc);
            } else {
                // ARM
                cpu->registers.pc = cpu->registers.r[rn] & 0xFFFFFFFC;
                cpu->registers.pc -= 4; // TODO: this is a hack to make the PC correct
                cpu->registers.cpsr |= 0x20;
                printf("BX: rs=%d, pc=%d, mode=ARM\n", cpu->registers.r[rn], cpu->registers.pc);
            }
            return 1;
        }

        // If i is 1 or bit 7 is 0, this is a Data Processing or PSR Transfer instruction
        if (i == 1 || ((instruction >> 7) & 0x1) == 0) {
            // Data Processing
            uint8_t opcode = (instruction >> 21) & 0xF;
            uint8_t s = (instruction >> 20) & 0x1; // Set Condition Codes (0 = no, 1 = yes)
            uint8_t rn = (instruction >> 16) & 0xF; // First Operand Register
            uint8_t rd = (instruction >> 12) & 0xF; // Destination Register
            uint32_t src2; // Source 2 (either a register or an immediate value)

            // Get the second operand based on the value of i
            // i = 0: Register Operand
            // i = 1: Immediate Operand
            if (i == 0) {
                uint8_t shift_value = cpu->registers.r[instruction & 0xF]; // Second Operand Register
                uint8_t shift_type = (instruction >> 5) & 0x3; // Shift Type
                uint8_t shift_amount;

                // Register Operand
                if ((instruction >> 4) & 0x1) {
                    // Shift by Register
                    uint8_t rs = (instruction >> 8) & 0xF; // Shift Amount Register
                    shift_amount = cpu->registers.r[rs] & 0xFF; // Shift Amount
                } else {
                    // Shift by Immediate
                    shift_amount = (instruction >> 7) & 0x1F; // Shift Amount
                }

                // Shift the value
                switch (shift_type) {
                case 0x0: // Logical Shift Left
                    // Shifts left by the shift amount, filling the low bits with 0s
                    // Sets the carry flag to the last bit shifted out
                    // If the shift amount is more than 32, the result is 0 and the carry flag is set to 0
                    if (shift_amount > 32) {
                        src2 = 0;
                        if (s && rd != 15) {
                            set_cpsr_carry(&cpu->registers, 0);
                        }
                    } else {
                        src2 = cpu->registers.r[shift_value] << shift_amount;
                        if (s && rd != 15) {
                            // TODO: there may be a special case for LSL #0 where the carry flag is preserved
                            set_cpsr_carry(&cpu->registers, (cpu->registers.r[shift_value] >> (32 - shift_amount)) & 0x1);
                        }
                    }
                    break;
                case 0x1: // Logical Shift Right
                    // Shifts right by the shift amount, filling the high bits with 0s
                    // Sets the carry flag to the last bit shifted out
                    // If the shift amount is more than 32, the result is 0 and the carry flag is set to 0
                    if (shift_amount > 32) {
                        src2 = 0;
                        if (s && rd != 15) {
                            set_cpsr_carry(&cpu->registers, 0);
                        }
                    } else {
                        src2 = cpu->registers.r[shift_value] >> shift_amount;
                        if (s && rd != 15) {
                            set_cpsr_carry(&cpu->registers, (cpu->registers.r[shift_value] >> (shift_amount - 1)) & 0x1);
                        }
                    }
                    break;
                case 0x2: // Arithmetic Shift Right
                    // Shifts right by the shift amount, filling the high bits with the sign bit
                    // Sets the carry flag to the last bit shifted out
                    // If the shift amount is 32 or more, the result and carry flag are set to the sign bit
                    if (shift_amount >= 32) {
                        src2 = sign_extend(cpu->registers.r[shift_value] >> 31, 32 - shift_amount);
                        if (s && rd != 15) {
                            set_cpsr_carry(&cpu->registers, (cpu->registers.r[shift_value] >> 31) & 0x1);
                        }
                    } else {
                        src2 = sign_extend(cpu->registers.r[shift_value] >> shift_amount, 32 - shift_amount);
                        if (s && rd != 15) {
                            set_cpsr_carry(&cpu->registers, (cpu->registers.r[shift_value] >> (shift_amount - 1)) & 0x1);
                        }
                    }
                    break;
                case 0x3: // Rotate Right
                    // If the shift amount is 0, this is a RRX (Rotate Right Extended)
                    // Otherwise, this is a ROR (Rotate Right)
                    if (shift_amount == 0) {
                        // Rotate Right Extended
                        // Shifts right by 1 bit, filling the high bit with the carry flag
                        // Sets the carry flag to the last bit shifted out
                        src2 = (cpu->registers.r[shift_value] >> 1) | (get_cpsr_carry(cpu->registers.cpsr) << 31);
                        if (s && rd != 15) {
                            set_cpsr_carry(&cpu->registers, cpu->registers.r[shift_value] & 0x1);
                        }
                    } else {
                        // Rotate Right
                        // Shifts right by the shift amount, filling the high bits with the low bits
                        // Sets the carry flag to the last bit shifted out
                        src2 = rotl32(cpu->registers.r[shift_value], shift_amount);
                        if (s && rd != 15) {
                            set_cpsr_carry(&cpu->registers, (cpu->registers.r[shift_value] >> (shift_amount - 1)) & 0x1);
                        }
                    }
                    break;
                }
                printf("Register offset: shift_value=%d, shift_type=%d, shift_amount=%d, src2=%d\n", shift_value, shift_type, shift_amount, src2);
            } else {
                // Immediate Operand
                uint8_t rotate = (instruction >> 8) & 0xF; // Rotate
                uint32_t imm = instruction & 0xFF; // Immediate Value, zero-extended to 32 bits

                // Rotate the immediate value
                src2 = rotr32(imm, rotate * 2);
                printf("Immediate offset: rotate=%d, imm=%X, src2=%X\n", rotate, imm, src2);
            }

            // Perform the operation
            switch (opcode) {
            case 0b0000: // AND (Logical AND)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                cpu->registers.r[rd] = cpu->registers.r[rn] & src2;
                set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                printf("AND: rn = %d, src2 = %d, rd = %d\n", cpu->registers.r[rn], src2, cpu->registers.r[rd]);
                break;
            case 0b0001: // EOR (Logical Exclusive OR)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                cpu->registers.r[rd] = cpu->registers.r[rn] ^ src2;
                set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                printf("EOR: rn = %d, src2 = %d, rd = %d\n", cpu->registers.r[rn], src2, cpu->registers.r[rd]);
                break;
            case 0b0010: // SUB (Arithmetic Subtraction)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                // Sets the c flag if there was no borrow
                // Sets the v flag if there was overflow
                cpu->registers.r[rd] = cpu->registers.r[rn] - src2;
                set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                set_cpsr_carry(&cpu->registers, cpu->registers.r[rn] >= src2);
                set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rn] ^ src2) & (cpu->registers.r[rn] ^ cpu->registers.r[rd]) & 0x80000000);
                printf("SUB: rn = %d, src2 = %d, rd = %d\n", cpu->registers.r[rn], src2, cpu->registers.r[rd]);
                break;
            case 0b0011: // RSB (Reverse Subtract)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                // Sets the c flag if there was no borrow
                // Sets the v flag if there was overflow
                cpu->registers.r[rd] = src2 - cpu->registers.r[rn];
                set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                set_cpsr_carry(&cpu->registers, src2 >= cpu->registers.r[rn]);
                set_cpsr_overflow(&cpu->registers, (src2 ^ cpu->registers.r[rn]) & (src2 ^ cpu->registers.r[rd]) & 0x80000000);
                printf("RSB: rn = %d, src2 = %d, rd = %d\n", cpu->registers.r[rn], src2, cpu->registers.r[rd]);
                break;
            case 0b0100: // ADD (Addition)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                // Sets the c flag if there was no borrow
                // Sets the v flag if there was overflow
                cpu->registers.r[rd] = cpu->registers.r[rn] + src2;
                set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                set_cpsr_carry(&cpu->registers, cpu->registers.r[rn] >= src2);
                set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rn] ^ src2) & (cpu->registers.r[rn] ^ cpu->registers.r[rd]) & 0x80000000);
                printf("ADD: rn = %d, src2 = %d, rd = %d\n", cpu->registers.r[rn], src2, cpu->registers.r[rd]);
                break;
            case 0b0101: // ADC (Add with Carry)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                // Sets the c flag if there was no borrow
                // Sets the v flag if there was overflow
                cpu->registers.r[rd] = cpu->registers.r[rn] + src2 + get_cpsr_carry(cpu->registers.cpsr);
                set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                set_cpsr_carry(&cpu->registers, cpu->registers.r[rn] >= src2);
                set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rn] ^ src2) & (cpu->registers.r[rn] ^ cpu->registers.r[rd]) & 0x80000000);
                printf("ADC: rn = %d, src2 = %d, rd = %d\n", cpu->registers.r[rn], src2, cpu->registers.r[rd]);
                break;
            case 0b0110: // SBC (Subtract with Carry)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                // Sets the c flag if there was no borrow
                // Sets the v flag if there was overflow
                cpu->registers.r[rd] = cpu->registers.r[rn] - src2 - !get_cpsr_carry(cpu->registers.cpsr);
                set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                set_cpsr_carry(&cpu->registers, cpu->registers.r[rn] >= src2);
                set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rn] ^ src2) & (cpu->registers.r[rn] ^ cpu->registers.r[rd]) & 0x80000000);
                printf("SBC: rn = %d, src2 = %d, rd = %d\n", cpu->registers.r[rn], src2, cpu->registers.r[rd]);
                break;
            case 0b0111: // RSC (Reverse Subtract with Carry)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                // Sets the c flag if there was no borrow
                // Sets the v flag if there was overflow
                cpu->registers.r[rd] = src2 - cpu->registers.r[rn] - !get_cpsr_carry(cpu->registers.cpsr);
                set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                set_cpsr_carry(&cpu->registers, src2 >= cpu->registers.r[rn]);
                set_cpsr_overflow(&cpu->registers, (src2 ^ cpu->registers.r[rn]) & (src2 ^ cpu->registers.r[rd]) & 0x80000000);
                printf("RSC: rn=%d, src2=%d, rd=%d\n", cpu->registers.r[rn], src2, cpu->registers.r[rd]);
                break;
            case 0b1000: // TST (Test)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                uint32_t tst_result = cpu->registers.r[rn] & src2;
                set_cpsr_zero(&cpu->registers, tst_result == 0);
                set_cpsr_negative(&cpu->registers, tst_result < 0);
                printf("TST: rn=%d, src2=%d, tst_result=%d\n", cpu->registers.r[rn], src2, tst_result);
                break;
            case 0b1001:
                // If the S bit is not set, this is a MSR (Move to PSR) instruction
                // If the S bit is set, this is a TEQ (Test Equivalence) instruction
                if (!s) {
                    // MSR (Move to PSR)
                    uint8_t pd = (instruction >> 22) & 0x1; // Destination (0 = CPSR, 1 = SPSR_<current mode>)
                    uint8_t rm = instruction & 0xF; // Operand

                    if (pd == 0) {
                        // CPSR
                        // If in user mode, only the condition flags can be modified (bits 31-28)
                        // If in any other mode, all flags can be modified
                        printf("MSR: pd=%d, rm=%d, cpsr=0x%X, user_mode=%s\n", pd, rm, cpu->registers.cpsr, get_cpsr_mode(cpu->registers.cpsr) == ARM_MODE_USER ? "true" : "false");
                        if (get_cpsr_mode(cpu->registers.cpsr) == ARM_MODE_USER) {
                            cpu->registers.cpsr = (cpu->registers.cpsr & ~0xF0000000) | (cpu->registers.r[rm] & 0xF0000000);
                        } else {
                            cpu->registers.cpsr = cpu->registers.r[rm];
                        }
                        printf("MSR: pd=%d, rm=%d, cpsr=0x%X, user_mode=%s\n", pd, rm, cpu->registers.cpsr, get_cpsr_mode(cpu->registers.cpsr) == ARM_MODE_USER ? "true" : "false");
                    } else {
                        // SPSR_<current mode>
                        cpu->registers.spsr = cpu->registers.r[rm];
                        printf("MSR: pd=%d, rm=%d, spsr=0x%X\n", pd, rm, cpu->registers.spsr);
                    }
                } else {
                    // TEQ (Test Equivalence)
                    // Sets the z flag if the result is 0
                    // Sets the n flag if the result is negative
                    uint32_t teq_result = cpu->registers.r[rn] ^ src2;
                    set_cpsr_zero(&cpu->registers, teq_result == 0);
                    set_cpsr_negative(&cpu->registers, teq_result < 0);
                    printf("TEQ: rn=%d (0x%X), src2=%d, teq_result=%X\n", rn, cpu->registers.r[rn], src2, teq_result);
                }
                break;
            case 0b1010: // CMP (Compare)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                // Sets the c flag if there was no borrow
                // Sets the v flag if there was overflow
                uint32_t cmp_result = cpu->registers.r[rn] - src2;
                set_cpsr_zero(&cpu->registers, cmp_result == 0);
                set_cpsr_negative(&cpu->registers, cmp_result < 0);
                set_cpsr_carry(&cpu->registers, cpu->registers.r[rn] >= src2);
                set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rn] ^ src2) & (cpu->registers.r[rn] ^ cmp_result) & 0x80000000);
                printf("CMP: rn=%d (0x%X), src2=%d, cmp_result=%d\n", rn, cpu->registers.r[rn], src2, cmp_result);
                break;
            case 0b1011: // CMN (Compare Negated)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                // Sets the c flag if there was no borrow
                // Sets the v flag if there was overflow
                uint32_t cmn_result = cpu->registers.r[rn] + src2;
                set_cpsr_zero(&cpu->registers, cmn_result == 0);
                set_cpsr_negative(&cpu->registers, cmn_result < 0);
                set_cpsr_carry(&cpu->registers, cpu->registers.r[rn] >= src2);
                set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rn] ^ src2) & (cpu->registers.r[rn] ^ cmn_result) & 0x80000000);
                printf("CMN: rn=%d, src2=%d, cmn_result=%d\n", cpu->registers.r[rn], src2, cmn_result);
                break;
            case 0b1100: // ORR (Logical (inclusive) OR)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                cpu->registers.r[rd] = cpu->registers.r[rn] | src2;
                set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                printf("ORR: rn=%d, src2=%d, rd=%d\n", cpu->registers.r[rn], src2, cpu->registers.r[rd]);
                break;
            case 0b1101: // MOV (Move)(Logical operation)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                cpu->registers.r[rd] = src2;
                set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                printf("MOV: src2=0x%X, rd=%d (0x%X)\n", src2, rd, cpu->registers.r[rd]);
                break;
            case 0b1110: // BIC (Bit Clear)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                cpu->registers.r[rd] = cpu->registers.r[rn] & ~src2;
                set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                printf("BIC: rn=%d, src2=%d, rd=%d\n", cpu->registers.r[rn], src2, cpu->registers.r[rd]);
                break;
            case 0b1111: // MVN (Move Not)(Logical operation)
                // Sets the z flag if the result is 0
                // Sets the n flag if the result is negative
                cpu->registers.r[rd] = ~src2;
                set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                printf("MVN: src2=%d, rd=%d\n", src2, cpu->registers.r[rd]);
                break;
            default:
                printf("Unhandled opcode: %d\n", opcode);
                return 0;
            }
        }
        break;
    }
    case 0x1: {
        // Single Data Transfer
        uint8_t p = (instruction >> 24) & 0x1; // Pre/Post Indexing (0 = post, 1 = pre)
        uint8_t u = (instruction >> 23) & 0x1; // Up/Down (0 = down, 1 = up)
        uint8_t b = (instruction >> 22) & 0x1; // Byte/Word (0 = word, 1 = byte)
        uint8_t w = (instruction >> 21) & 0x1; // Writeback (0 = no, 1 = yes)
        uint8_t l = (instruction >> 20) & 0x1; // Load/Store (0 = store, 1 = load)
        uint8_t rn = (instruction >> 16) & 0xF; // Base Register
        uint8_t rd = (instruction >> 12) & 0xF; // Destination Register
        uint32_t offset; // Offset (either a register or an immediate value)

        // Get the offset based on the value of i
        // i = 0: Immediate Operand
        // i = 1: Register Operand
        if (i == 1) {
            uint8_t shift_value = cpu->registers.r[instruction & 0xF]; // Second Operand Register
            uint8_t shift_type = (instruction >> 5) & 0x3; // Shift Type
            uint8_t shift_amount;

            // Register Operand
            if ((instruction >> 4) & 0x1) {
                // Shift by Register
                uint8_t rs = (instruction >> 8) & 0xF; // Shift Amount Register
                shift_amount = cpu->registers.r[rs] & 0xFF; // Shift Amount
                printf("Register offset: shift_value=%d, shift_type=%d, rs=%d (0x%X) shift_amount=%d\n", shift_value, shift_type, rs, cpu->registers.r[rs], shift_amount);
            } else {
                // Shift by Immediate
                shift_amount = (instruction >> 7) & 0x1F; // Shift Amount
                printf("Register offset: shift_value=%d, shift_type=%d, shift_amount=%d\n", shift_value, shift_type, shift_amount);
            }

            // Shift the value
            switch (shift_type) {
            case 0x0: // Logical Shift Left
                // Shifts left by the shift amount, filling the low bits with 0s
                // Sets the carry flag to the last bit shifted out
                // If the shift amount is more than 32, the result is 0 and the carry flag is set to 0
                if (shift_amount > 32) {
                    offset = 0;
                    // if (s && rd != 15) {
                    //     set_cpsr_carry(&cpu->registers, 0);
                    // }
                } else {
                    offset = cpu->registers.r[shift_value] << shift_amount;
                    // if (s && rd != 15) {
                    //     // TODO: there may be a special case for LSL #0 where the carry flag is preserved
                    //     set_cpsr_carry(&cpu->registers, (cpu->registers.r[shift_value] >> (32 - shift_amount)) & 0x1);
                    // }
                }
                break;
            case 0x1: // Logical Shift Right
                // Shifts right by the shift amount, filling the high bits with 0s
                // Sets the carry flag to the last bit shifted out
                // If the shift amount is more than 32, the result is 0 and the carry flag is set to 0
                if (shift_amount > 32) {
                    offset = 0;
                    // if (s && rd != 15) {
                    //     set_cpsr_carry(&cpu->registers, 0);
                    // }
                } else {
                    offset = cpu->registers.r[shift_value] >> shift_amount;
                    // if (s && rd != 15) {
                    //     set_cpsr_carry(&cpu->registers, (cpu->registers.r[shift_value] >> (shift_amount - 1)) & 0x1);
                    // }
                }
                break;
            case 0x2: // Arithmetic Shift Right
                // Shifts right by the shift amount, filling the high bits with the sign bit
                // Sets the carry flag to the last bit shifted out
                // If the shift amount is 32 or more, the result and carry flag are set to the sign bit
                if (shift_amount >= 32) {
                    offset = sign_extend(cpu->registers.r[shift_value] >> 31, 32 - shift_amount);
                    // if (s && rd != 15) {
                    //     set_cpsr_carry(&cpu->registers, (cpu->registers.r[shift_value] >> 31) & 0x1);
                    // }
                } else {
                    offset = sign_extend(cpu->registers.r[shift_value] >> shift_amount, 32 - shift_amount);
                    // if (s && rd != 15) {
                    //     set_cpsr_carry(&cpu->registers, (cpu->registers.r[shift_value] >> (shift_amount - 1)) & 0x1);
                    // }
                }
                break;
            case 0x3: // Rotate Right
                // If the shift amount is 0, this is a RRX (Rotate Right Extended)
                // Otherwise, this is a ROR (Rotate Right)
                if (shift_amount == 0) {
                    // Rotate Right Extended
                    // Shifts right by 1 bit, filling the high bit with the carry flag
                    // Sets the carry flag to the last bit shifted out
                    offset = (cpu->registers.r[shift_value] >> 1) | (get_cpsr_carry(cpu->registers.cpsr) << 31);
                    // if (s && rd != 15) {
                    //     set_cpsr_carry(&cpu->registers, cpu->registers.r[shift_value] & 0x1);
                    // }
                } else {
                    // Rotate Right
                    // Shifts right by the shift amount, filling the high bits with the low bits
                    // Sets the carry flag to the last bit shifted out
                    offset = rotl32(cpu->registers.r[shift_value], shift_amount);
                    // if (s && rd != 15) {
                    //     set_cpsr_carry(&cpu->registers, (cpu->registers.r[shift_value] >> (shift_amount - 1)) & 0x1);
                    // }
                }
                break;
            }
        } else {
            // Immediate Operand
            offset = instruction & 0xFFF; // Immediate Value
            printf("Immediate offset: %X\n", offset);
        }

        // Calculate the address
        uint32_t address = cpu->registers.r[rn];

        // If R15 is the base register, add 8 to the address
        if (rn == 15) {
            // Add 12 if this is a store instruction
            if (l == 0) {
                address += 12;
            } else {
                address += 8;
            }
        }

        if (p == 1) {
            // Pre-Indexing
            if (u == 1) {
                // Up
                address += offset;
            } else {
                // Down
                address -= offset;
            }
        }

        // Perform the operation
        if (l == 1) {
            // Load
            if (b == 1) {
                // Byte
                cpu->registers.r[rd] = cpu->memory[address];
            } else {
                // Word
                cpu->registers.r[rd] = *(uint32_t*)&cpu->memory[address];
            }
        } else {
            // Store
            if (b == 1) {
                // Byte
                cpu->memory[address] = cpu->registers.r[rd];
            } else {
                // Word
                *(uint32_t*)&cpu->memory[address] = cpu->registers.r[rd];
            }
        }

        // Post-Indexing
        if (p == 0) {
            if (u == 1) {
                // Up
                address += offset;
            } else {
                // Down
                address -= offset;
            }
        }

        printf("Single Data Transfer: p=%d, u=%d, b=%d, w=%d, l=%d, rn=%d (0x%X), rd=%d (0x%X), offset=0x%X, address=0x%X\n", p, u, b, w, l, rn, cpu->registers.r[rn], rd, cpu->registers.r[rd], offset, address);
        fflush(stdout);

        // Writeback
        if (w == 1 || p == 0) {
            cpu->registers.r[rn] = address;
        }

        return 1;
    }
    case 0x2: {
        if (i == 1) {
            // Branch/Branch with Link
            uint8_t l = (instruction >> 24) & 0x1; // Link (0 = no, 1 = yes)
            int32_t offset = instruction & 0xFFFFFF; // Offset

            // Save the return address in the link register if we're branching with link
            if (l == 1) {
                cpu->registers.lr = cpu->registers.pc + 4;
            }

            // Shift the offset to the left by 2 bits
            offset = offset << 2;

            // Sign extend the offset
            int32_t signed_offset = sign_extend(offset, 26);

            // Add the offset to the PC
            cpu->registers.pc += signed_offset + 4;

            printf("Branch: offset=0x%X, link=%d, new_pc=0x%X\n", signed_offset, l, cpu->registers.pc);
        } else {
            // Block Data Transfer
            uint8_t p = (instruction >> 24) & 0x1; // Pre/Post Indexing (0 = post, 1 = pre)
            uint8_t u = (instruction >> 23) & 0x1; // Up/Down (0 = down, 1 = up)
            uint8_t s = (instruction >> 22) & 0x1; // PSR/Force User Mode (0 = do not load PSR or force user mode, 1 = load PSR or force user mode)
            uint8_t w = (instruction >> 21) & 0x1; // Writeback (0 = no, 1 = yes)
            uint8_t l = (instruction >> 20) & 0x1; // Load/Store (0 = store, 1 = load)
            uint8_t rn = (instruction >> 16) & 0xF; // Base Register
            uint16_t register_list = instruction & 0xFFFF; // Register List

            // Starting address
            uint32_t address = cpu->registers.r[rn];

            // Loop through each register in the register list
            for (int i = 0; i < 16; i++) {
                if ((register_list >> i) & 0x1) {
                    // Pre-Indexing
                    if (p == 1) {
                        if (u == 1) {
                            // Up
                            address += 4;
                        } else {
                            // Down
                            address -= 4;
                        }
                    }

                    // Load or store the register
                    if (l == 1) {
                        // Load
                        cpu->registers.r[i] = *(uint32_t*)&cpu->memory[address];

                        // Transfer SPRSP_<mode> to CPSR if we're loading the PC and S is set
                        if (i == 15 && s == 1) {
                            cpu->registers.cpsr = cpu->registers.spsr;
                        }
                    } else {
                        // Store
                        // TODO: Take registers from User bank if S is set
                        *(uint32_t*)&cpu->memory[address] = cpu->registers.r[i];
                    }

                    // Post-Indexing
                    if (p == 0) {
                        if (u == 1) {
                            // Up
                            address += 4;
                        } else {
                            // Down
                            address -= 4;
                        }
                    }
                }
            }

            printf("Block Data Transfer: p=%d, u=%d, s=%d, w=%d, l=%d, rn=%d (0x%X), register_list=0x%X, address=0x%X\n", p, u, s, w, l, rn, cpu->registers.r[rn], register_list, address);
            fflush(stdout);

            // Writeback
            if (w == 1 || p == 0) {
                cpu->registers.r[rn] = address;
            }
        }
        break;
    }
    case 0x3: {
        printf("Coprocessor\n");
        return 0;
        break;
    }
    }

    return 1;
}

int cpu_process_thumb_instruction(cpu_t* cpu, cpu_thumb_instruction_t instruction)
{
    // Print the PC and instruction
    printf("[THUMB] SP=0x%X, PC=0x%X, Instruction=0x%X, ", cpu->registers.sp, cpu->registers.pc, instruction);

    // if (cpu->registers.sp != 0) {
    //     printf("SP not 0\n");
    //     return 0;
    // }

    // Decode the instruction
    uint8_t instruction_type = (instruction >> 13) & 0x7;

    switch (instruction_type) {
    case 0b000: {
        // Shift by Immediate
        uint8_t opcode = (instruction >> 11) & 0x3;

        switch (opcode) {
        case 0x0: { // LSL (Logical Shift Left)
            uint8_t offset5 = (instruction >> 6) & 0x1F;
            uint8_t rs = (instruction >> 3) & 0x7;
            uint8_t rd = (instruction >> 0) & 0x7;

            cpu->registers.r[rd] = cpu->registers.r[rs] << offset5;

            // Set CPSR condition codes
            set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
            set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);

            printf("LSL: rs=%d, offset5=%d, rd=%d\n", cpu->registers.r[rs], offset5, cpu->registers.r[rd]);
            break;
        }
        case 0x1: { // LSR (Logical Shift Right)
            uint8_t offset5 = (instruction >> 6) & 0x1F;
            uint8_t rs = (instruction >> 3) & 0x7;
            uint8_t rd = (instruction >> 0) & 0x7;

            cpu->registers.r[rd] = cpu->registers.r[rs] >> offset5;

            // Set CPSR condition codes
            set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
            set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);

            printf("LSR: rs=%d, offset5=%d, rd=%d\n", cpu->registers.r[rs], offset5, cpu->registers.r[rd]);
            break;
        }
        case 0x2: { // ASR (Arithmetic Shift Right)
            uint8_t offset5 = (instruction >> 6) & 0x1F;
            uint8_t rs = (instruction >> 3) & 0x7;
            uint8_t rd = (instruction >> 0) & 0x7;

            cpu->registers.r[rd] = sign_extend(cpu->registers.r[rs] >> offset5, 32 - offset5);

            // Set CPSR condition codes
            set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
            set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);

            printf("ASR: rs=%d, offset5=%d, rd=%d\n", cpu->registers.r[rs], offset5, cpu->registers.r[rd]);
            break;
        }
        case 0x3: { // Add Offset to Stack Pointer
            uint8_t i = (instruction >> 10) & 0x1;
            uint8_t op = (instruction >> 9) & 0x1;
            uint8_t rn_or_offset3 = (instruction >> 6) & 0x7;
            uint8_t rs = (instruction >> 3) & 0x7;
            uint8_t rd = (instruction >> 0) & 0x7;

            // Get the operand based on the value of i
            // i = 0: Register Operand
            // i = 1: Immediate Operand
            uint32_t operand;

            if (i == 0) {
                operand = cpu->registers.r[rn_or_offset3];
            } else {
                operand = rn_or_offset3;
            }

            // Perform the operation specified by op
            if (op == 0) {
                // ADD
                cpu->registers.r[rd] = cpu->registers.r[rs] + operand;
            } else {
                // SUB
                cpu->registers.r[rd] = cpu->registers.r[rs] - operand;
            }

            // Set CPSR condition codes
            set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
            set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
            set_cpsr_carry(&cpu->registers, cpu->registers.r[rs] >= operand);
            set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rs] ^ operand) & (cpu->registers.r[rs] ^ cpu->registers.r[rd]) & 0x80000000);

            printf("ADD/SUB: rs=%d, operand=%d, rd=%d\n", cpu->registers.r[rs], operand, cpu->registers.r[rd]);
            break;
        }
        }
    }
    case 0b001: {
        // Add/Subtract
        uint8_t opcode = (instruction >> 11) & 0x3;
        uint8_t rd = (instruction >> 8) & 0x7;
        uint8_t offset8 = (instruction >> 0) & 0xFF;

        switch (opcode) {
        case 0x0: // MOV (Move)
            cpu->registers.r[rd] = offset8;

            // Set CPSR condition codes
            set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
            set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);

            printf("MOV: rd=%d, offset8=%d\n", cpu->registers.r[rd], offset8);
            break;
        case 0x1: // CMP (Compare)
            // Set CPSR condition codes
            set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == offset8);
            set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < offset8);
            set_cpsr_carry(&cpu->registers, cpu->registers.r[rd] >= offset8);
            set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rd] ^ offset8) & (cpu->registers.r[rd] ^ (cpu->registers.r[rd] - offset8)) & 0x80000000);

            printf("CMP: rd=%d, offset8=%d\n", cpu->registers.r[rd], offset8);
            break;
        case 0x2: // ADD (Add)
            cpu->registers.r[rd] += offset8;

            // Set CPSR condition codes
            set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
            set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
            set_cpsr_carry(&cpu->registers, cpu->registers.r[rd] >= offset8);
            set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rd] ^ offset8) & (cpu->registers.r[rd] ^ (cpu->registers.r[rd] - offset8)) & 0x80000000);

            printf("ADD: rd=%d, offset8=%d\n", cpu->registers.r[rd], offset8);
            break;
        case 0x3: // SUB (Subtract)
            cpu->registers.r[rd] -= offset8;

            // Set CPSR condition codes
            set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
            set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
            set_cpsr_carry(&cpu->registers, cpu->registers.r[rd] >= offset8);
            set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rd] ^ offset8) & (cpu->registers.r[rd] ^ (cpu->registers.r[rd] - offset8)) & 0x80000000);

            printf("SUB: rd=%d, offset8=%d\n", cpu->registers.r[rd], offset8);
            break;
        }
        break;
    }
    case 0b010: {
        if (((instruction >> 12) & 0x1) == 0) {
            if (((instruction >> 11) & 0x1) == 0) {
                if (((instruction >> 10) & 0x1) == 0) {
                    // ALU Operations
                    uint8_t opcode = (instruction >> 6) & 0xF;
                    uint8_t rs = (instruction >> 3) & 0x7;
                    uint8_t rd = (instruction >> 0) & 0x7;

                    switch (opcode) {
                    case 0x0: // AND (Logical AND)
                        cpu->registers.r[rd] &= cpu->registers.r[rs];

                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);

                        printf("AND: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0x1: // EOR (Logical Exclusive OR)
                        cpu->registers.r[rd] ^= cpu->registers.r[rs];

                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);

                        printf("EOR: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0x2: // LSL (Logical Shift Left)
                        cpu->registers.r[rd] <<= cpu->registers.r[rs];

                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);

                        printf("LSL: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0x3: // LSR (Logical Shift Right)
                        cpu->registers.r[rd] >>= cpu->registers.r[rs];

                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);

                        printf("LSR: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0x4: // ASR (Arithmetic Shift Right)
                        cpu->registers.r[rd] = sign_extend(cpu->registers.r[rd] >> cpu->registers.r[rs], 32 - cpu->registers.r[rs]);

                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);

                        printf("ASR: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0x5: // ADC (Add with Carry)
                        cpu->registers.r[rd] += cpu->registers.r[rs] + get_cpsr_carry(cpu->registers.cpsr);

                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                        set_cpsr_carry(&cpu->registers, cpu->registers.r[rd] >= cpu->registers.r[rs]);
                        set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rd] ^ cpu->registers.r[rs]) & (cpu->registers.r[rd] ^ (cpu->registers.r[rd] - cpu->registers.r[rs])) & 0x80000000);

                        printf("ADC: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0x6: // SBC (Subtract with Carry)
                        cpu->registers.r[rd] -= cpu->registers.r[rs] - !get_cpsr_carry(cpu->registers.cpsr);

                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                        set_cpsr_carry(&cpu->registers, cpu->registers.r[rd] >= cpu->registers.r[rs]);
                        set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rd] ^ cpu->registers.r[rs]) & (cpu->registers.r[rd] ^ (cpu->registers.r[rd] - cpu->registers.r[rs])) & 0x80000000);

                        printf("SBC: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0x7: // ROR (Rotate Right)
                        cpu->registers.r[rd] = rotl32(cpu->registers.r[rd], cpu->registers.r[rs]);

                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);

                        printf("ROR: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0x8: // TST (Test)
                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, (cpu->registers.r[rd] & cpu->registers.r[rs]) == 0);
                        set_cpsr_negative(&cpu->registers, (cpu->registers.r[rd] & cpu->registers.r[rs]) < 0);

                        printf("TST: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0x9: // NEG (Negate)
                        cpu->registers.r[rd] = -cpu->registers.r[rs];

                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);
                        set_cpsr_carry(&cpu->registers, 0);
                        set_cpsr_overflow(&cpu->registers, cpu->registers.r[rd] == 0x80000000);

                        printf("NEG: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0xA: // CMP (Compare)
                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == cpu->registers.r[rs]);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < cpu->registers.r[rs]);
                        set_cpsr_carry(&cpu->registers, cpu->registers.r[rd] >= cpu->registers.r[rs]);
                        set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rd] ^ cpu->registers.r[rs]) & (cpu->registers.r[rd] ^ (cpu->registers.r[rd] - cpu->registers.r[rs])) & 0x80000000);

                        printf("CMP: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0xB: // CMN (Compare Negated)
                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == -cpu->registers.r[rs]);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < -cpu->registers.r[rs]);
                        set_cpsr_carry(&cpu->registers, cpu->registers.r[rd] >= -cpu->registers.r[rs]);
                        set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rd] ^ -cpu->registers.r[rs]) & (cpu->registers.r[rd] ^ (cpu->registers.r[rd] - -cpu->registers.r[rs])) & 0x80000000);

                        printf("CMN: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0xC: // ORR (Logical (inclusive) OR)
                        cpu->registers.r[rd] |= cpu->registers.r[rs];

                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);

                        printf("ORR: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0xD: // MUL (Multiply)
                        cpu->registers.r[rd] *= cpu->registers.r[rs];

                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);

                        printf("MUL: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0xE: // BIC (Bit Clear)
                        cpu->registers.r[rd] &= ~cpu->registers.r[rs];

                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);

                        printf("BIC: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    case 0xF: // MVN (Move Not)
                        cpu->registers.r[rd] = ~cpu->registers.r[rs];

                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd] == 0);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd] < 0);

                        printf("MVN: rs=%d, rd=%d\n", cpu->registers.r[rs], cpu->registers.r[rd]);
                        break;
                    }
                } else {
                    // Hi Register Operations/Branch Exchange
                    uint8_t opcode = (instruction >> 8) & 0x3;
                    uint8_t h1 = (instruction >> 7) & 0x1;
                    uint8_t h2 = (instruction >> 6) & 0x1;
                    uint8_t rs_hs = (instruction >> 3) & 0x7;
                    uint8_t rd_hd = (instruction >> 0) & 0x7;

                    switch (opcode) {
                    case 0x0: // ADD (Add)
                        // Does not set CPSR condition codes
                        cpu->registers.r[rd_hd] += cpu->registers.r[rs_hs];
                        printf("ADD: rs=%d, rd=%d\n", cpu->registers.r[rs_hs], cpu->registers.r[rd_hd]);
                        break;
                    case 0x1: // CMP (Compare)
                        // Set CPSR condition codes
                        set_cpsr_zero(&cpu->registers, cpu->registers.r[rd_hd] == cpu->registers.r[rs_hs]);
                        set_cpsr_negative(&cpu->registers, cpu->registers.r[rd_hd] < cpu->registers.r[rs_hs]);
                        set_cpsr_carry(&cpu->registers, cpu->registers.r[rd_hd] >= cpu->registers.r[rs_hs]);
                        set_cpsr_overflow(&cpu->registers, (cpu->registers.r[rd_hd] ^ cpu->registers.r[rs_hs]) & (cpu->registers.r[rd_hd] ^ (cpu->registers.r[rd_hd] - cpu->registers.r[rs_hs])) & 0x80000000);

                        printf("CMP: rs=%d, rd=%d\n", cpu->registers.r[rs_hs], cpu->registers.r[rd_hd]);
                        break;
                    case 0x2: // MOV (Move)
                        // Does not set CPSR condition codes
                        cpu->registers.r[rd_hd] = cpu->registers.r[rs_hs];
                        printf("MOV: rs=%d, rd=%d\n", cpu->registers.r[rs_hs], cpu->registers.r[rd_hd]);
                        break;
                    case 0x3: // BX (Branch and Exchange)
                        // Does not set CPSR condition codes
                        // Determine if the mode is ARM or Thumb and set the PC and mode accordingly
                        if (cpu->registers.r[rs_hs] & 0x1) {
                            // Thumb
                            cpu->registers.pc = cpu->registers.r[rs_hs] & 0xFFFFFFFE;
                            cpu->registers.pc -= 2; // TODO: this is a hack to fix the PC being incremented after this instruction
                            cpu->registers.cpsr &= ~0x20;
                            printf("BX: rs=%d, pc=%d, mode=THUMB\n", cpu->registers.r[rs_hs], cpu->registers.pc);
                        } else {
                            // ARM
                            cpu->registers.pc = cpu->registers.r[rs_hs] & 0xFFFFFFFC;
                            cpu->registers.pc -= 4; // TODO: this is a hack to fix the PC being incremented after this instruction
                            cpu->registers.cpsr |= 0x20;
                            printf("BX: rs=%d, pc=%d, mode=ARM\n", cpu->registers.r[rs_hs], cpu->registers.pc);
                        }
                        break;
                    }
                }
            } else {
                // PC Relative Load
                uint8_t rd = (instruction >> 8) & 0x7;
                uint8_t offset8 = (instruction >> 0) & 0xFF;

                cpu->registers.r[rd] = cpu->registers.pc + (offset8 << 2);

                printf("PC Relative Load: rd=%d, offset8=%d\n", cpu->registers.r[rd], offset8);
            }
        } else {
            if (((instruction >> 9) & 0x1) == 0) {
                // Load/Store with Register Offset
                uint8_t l = (instruction >> 11) & 0x1;
                uint8_t b = (instruction >> 10) & 0x1;
                uint8_t ro = (instruction >> 6) & 0x7;
                uint8_t rb = (instruction >> 3) & 0x7;
                uint8_t rd = (instruction >> 0) & 0x7;

                // Calculate the address
                uint32_t address = cpu->registers.r[rb] + cpu->registers.r[ro];

                // Perform the operation
                if (l == 1) {
                    // Load
                    if (b == 1) {
                        // Byte
                        cpu->registers.r[rd] = cpu->memory[address];
                    } else {
                        // Word
                        cpu->registers.r[rd] = *(uint32_t*)&cpu->memory[address];
                    }
                } else {
                    // Store
                    if (b == 1) {
                        // Byte
                        cpu->memory[address] = cpu->registers.r[rd];
                    } else {
                        // Word
                        *(uint32_t*)&cpu->memory[address] = cpu->registers.r[rd];
                    }
                }

                printf("Load/Store with Register Offset: l=%d, b=%d, ro=%d, rb=%d, rd=%d\n", l, b, cpu->registers.r[ro], cpu->registers.r[rb], cpu->registers.r[rd]);
            } else {
                // Load/Store sign-extended byte/halfword
                uint8_t h = (instruction >> 11) & 0x1;
                uint8_t s = (instruction >> 10) & 0x1;
                uint8_t ro = (instruction >> 6) & 0x7;
                uint8_t rb = (instruction >> 3) & 0x7;
                uint8_t rd = (instruction >> 0) & 0x7;

                // Calculate the address
                uint32_t address = cpu->registers.r[rb] + cpu->registers.r[ro];

                // Perform the operation
                if (s == 1) {
                    // Load
                    if (h == 1) {
                        // Halfword
                        cpu->registers.r[rd] = sign_extend(*(int16_t*)&cpu->memory[address], 16);
                    } else {
                        // Byte
                        cpu->registers.r[rd] = sign_extend(cpu->memory[address], 8);
                    }
                } else {
                    // Store
                    if (h == 1) {
                        // Halfword
                        *(int16_t*)&cpu->memory[address] = cpu->registers.r[rd];
                    } else {
                        // Byte
                        cpu->memory[address] = cpu->registers.r[rd];
                    }
                }

                printf("Load/Store sign-extended byte/halfword: h=%d, s=%d, ro=%d, rb=%d, rd=%d\n", h, s, cpu->registers.r[ro], cpu->registers.r[rb], cpu->registers.r[rd]);
            }
        }
    }
    case 0b011: {
        // Load/Store with Immediate Offset
        uint8_t b = (instruction >> 12) & 0x1;
        uint8_t l = (instruction >> 11) & 0x1;
        uint8_t offset5 = (instruction >> 6) & 0x1F;
        uint8_t rb = (instruction >> 3) & 0x7;
        uint8_t rd = (instruction >> 0) & 0x7;

        // Calculate the address
        uint32_t address = cpu->registers.r[rb] + (offset5 << 2);

        // Perform the operation
        if (l == 1) {
            // Load
            if (b == 1) {
                // Byte
                cpu->registers.r[rd] = cpu->memory[address];
            } else {
                // Word
                cpu->registers.r[rd] = *(uint32_t*)&cpu->memory[address];
            }
        } else {
            // Store
            if (b == 1) {
                // Byte
                cpu->memory[address] = cpu->registers.r[rd];
            } else {
                // Word
                *(uint32_t*)&cpu->memory[address] = cpu->registers.r[rd];
            }
        }

        printf("Load/Store with Immediate Offset: b=%d, l=%d, offset5=%d, rb=%d, rd=%d\n", b, l, offset5, cpu->registers.r[rb], cpu->registers.r[rd]);
        break;
    }
    case 0b100: {
        if (((instruction >> 12) & 0x1) == 0) {
            // Load/Store Halfword
            uint8_t l = (instruction >> 10) & 0x1;
            uint8_t offset5 = (instruction >> 6) & 0x1F;
            uint8_t rb = (instruction >> 3) & 0x7;
            uint8_t rd = (instruction >> 0) & 0x7;

            // Calculate the address
            uint32_t address = cpu->registers.r[rb] + (offset5 << 1);

            // Perform the operation
            if (l == 1) {
                // Load
                cpu->registers.r[rd] = sign_extend(*(int16_t*)&cpu->memory[address], 16);
            } else {
                // Store
                *(int16_t*)&cpu->memory[address] = cpu->registers.r[rd];
            }

            printf("Load/Store Halfword: l=%d, offset5=%d, rb=%d, rd=%d\n", l, offset5, cpu->registers.r[rb], cpu->registers.r[rd]);
        } else {
            // Load/Store SP-relative
            uint8_t l = (instruction >> 11) & 0x1;
            uint8_t rd = (instruction >> 8) & 0x7;
            uint8_t offset8 = (instruction >> 0) & 0xFF;

            // Calculate the address
            uint32_t address = cpu->registers.sp + (offset8 << 2);

            // Perform the operation
            if (l == 1) {
                // Load
                cpu->registers.r[rd] = *(uint32_t*)&cpu->memory[address];
            } else {
                // Store
                *(uint32_t*)&cpu->memory[address] = cpu->registers.r[rd];
            }

            printf("Load/Store SP-relative: l=%d, rd=%d, offset8=%d\n", l, cpu->registers.r[rd], offset8);
        }
        break;
    }
    case 0b101: {
        if (((instruction >> 12) & 0x1) == 0) {
            // Load Address
            uint8_t sp = (instruction >> 11) & 0x1; // 0 = PC, 1 = SP
            uint8_t rd = (instruction >> 8) & 0x7;
            uint8_t offset8 = (instruction >> 0) & 0xFF;

            // Calculate the address
            if (sp == 0) {
                // PC
                cpu->registers.r[rd] = cpu->registers.pc + (offset8 << 2);
            } else {
                // SP
                cpu->registers.r[rd] = cpu->registers.sp + (offset8 << 2);
            }

            printf("Load Address: sp=%d, rd=%d, offset8=%d\n", sp, cpu->registers.r[rd], offset8);
        } else if (((instruction >> 10) & 0x1) == 0) {
            // Add Offset to Stack Pointer
            uint8_t s = (instruction >> 7) & 0x1; // 0 = offset is positive, 1 = offset is negative
            uint8_t sword7 = (instruction >> 0) & 0x7F;

            // Calculate the offset
            int32_t offset = s == 0 ? sword7 : -sword7;

            // Add the offset to the stack pointer
            cpu->registers.sp += offset << 2;

            printf("Add Offset to Stack Pointer: s=%d, offset=%d\n", s, offset);
        } else {
            // Push/Pop Registers
            uint8_t l = (instruction >> 11) & 0x1; // 0 = store to memory, 1 = load from memory
            uint8_t r = (instruction >> 8) & 0x1; // 0 = do not store LR/load PC, 1 = store LR/load PC
            uint8_t rlist = (instruction >> 0) & 0xFF; // Register list

            // Calculate the address
            uint32_t address = cpu->registers.sp;

            // Perform the operation
            if (l == 1) {
                // Load
                for (int i = 0; i < 8; i++) {
                    if ((rlist >> i) & 0x1) {
                        cpu->registers.r[i] = *(uint32_t*)&cpu->memory[address];
                        address += 4;
                    }
                }

                if (r == 1) {
                    cpu->registers.pc = *(uint32_t*)&cpu->memory[address] & 0xFFFFFFFE;
                    cpu->registers.cpsr &= ~0x20;
                    address += 4;
                }
            } else {
                // Store
                for (int i = 0; i < 8; i++) {
                    if ((rlist >> i) & 0x1) {
                        *(uint32_t*)&cpu->memory[address] = cpu->registers.r[i];
                        address += 4;
                    }
                }

                if (r == 1) {
                    *(uint32_t*)&cpu->memory[address] = cpu->registers.lr;
                    address += 4;
                }
            }

            // Update the stack pointer
            cpu->registers.sp = address;

            printf("Push/Pop Registers: l=%d, r=%d, rlist=%d\n", l, r, rlist);
        }
        break;
    }
    case 0b110: {
        if (((instruction >> 12) & 0x1) == 0) {
            // Multiple Load/Store
            uint8_t l = (instruction >> 11) & 0x1; // 0 = store to memory, 1 = load from memory
            uint8_t rb = (instruction >> 8) & 0x7;
            uint8_t rlist = (instruction >> 0) & 0xFF; // Register list

            // Calculate the address
            uint32_t address = cpu->registers.r[rb];

            // Perform the operation
            if (l == 1) {
                // Load
                for (int i = 0; i < 8; i++) {
                    if ((rlist >> i) & 0x1) {
                        cpu->registers.r[i] = *(uint32_t*)&cpu->memory[address];
                        address += 4;
                    }
                }
            } else {
                // Store
                for (int i = 0; i < 8; i++) {
                    if ((rlist >> i) & 0x1) {
                        *(uint32_t*)&cpu->memory[address] = cpu->registers.r[i];
                        address += 4;
                    }
                }
            }

            // Update the base register
            cpu->registers.r[rb] = address;

            printf("Multiple Load/Store: l=%d, rb=%d, rlist=%d\n", l, cpu->registers.r[rb], rlist);
        } else if (((instruction >> 8) & 0xF) == 0xF) {
            // Software Interrupt
            uint8_t value8 = (instruction >> 0) & 0xFF;

            // Move address of next instruction into LR
            cpu->registers.lr = cpu->registers.pc + 4;

            // Move CPSR into SPSR
            cpu->registers.spsr = cpu->registers.cpsr;

            // Load SWI vector into PC
            cpu->registers.pc = *(uint32_t*)&cpu->memory[0x8] & 0xFFFFFFFE;

            // Switch into ARM state and enter supervisor mode (SVC)
            cpu->registers.cpsr &= ~0x1F;
            cpu->registers.cpsr |= 0x13;

            printf("Software Interrupt: value8=%d\n", value8);
        } else {
            // Conditional Branch
            uint8_t cond = (instruction >> 8) & 0xF;
            uint8_t offset8 = (instruction >> 0) & 0xFF;

            // Calculate the offset
            int32_t offset = sign_extend(offset8 << 1, 9);

            // Check the condition
            if (cpu_check_condition(cpu->registers.cpsr, cond)) {
                // Branch
                cpu->registers.pc += offset + 4;
                printf("Conditional Branch: cond=%d, offset8=%d, branch=TRUE\n", cond, offset8);
            } else {
                printf("Conditional Branch: cond=%d, offset8=%d, branch=FALSE\n", cond, offset8);
            }
        }
        break;
    }
    case 0b111: {
        if (((instruction >> 12) & 0x1) == 0) {
            // Unconditional Branch
            uint8_t offset11 = (instruction >> 0) & 0x7FF;

            // Calculate the offset
            int32_t offset = sign_extend(offset11 << 1, 12);

            // Branch
            cpu->registers.pc += offset + 4;

            printf("Unconditional Branch: offset11=%d\n", offset11);
        } else {
            // Long Branch with Link
            uint8_t h = (instruction >> 11) & 0x1; // 0 = offset high, 1 = offset low
            uint32_t offset11 = (instruction >> 0) & 0x7FF;

            if (h == 0) {
                // When h = 0, the offset is the high 11 bits of the offset
                // This is shifted left by 12 bits and added to the current PC address.
                // The resulting address is placed in LR.
                cpu->registers.lr = cpu->registers.pc + (offset11 << 12);
            } else {
                // When h = 1, the offset field contains an 11-bit representation lower half of
                // the target address. This is shifted left by 1 bit and added to LR. LR, which now contains
                // the full 23-bit address, is placed in PC, the address of the instruction following the BL
                // is placed in LR and bit 0 of LR is set.
                cpu->registers.pc = cpu->registers.lr + (offset11 << 1);
                cpu->registers.pc -= 2; // TODO: this is a hack to fix the PC being incremented after this instruction
                cpu->registers.lr = cpu->registers.pc + 2;
                cpu->registers.lr |= 0x1;
            }

            printf("Long Branch with Link: h=%d, offset11=%d\n", h, offset11);
        }
        break;
    }
    default:
        printf("Unknown instruction: %d\n", instruction);
        return 0;
    }

    return 1;
}

// Run a program using the given CPU
// Return 0 if the program ran successfully, 1 if there was an error
int cpu_run(cpu_t* cpu)
{
    // Clear the registers
    memset(&cpu->registers, 0, sizeof(cpu_registers_t));

    // Start the CPU in ARM mode
    cpu->registers.cpsr |= 0x20;

    // Start the CPU in user mode
    cpu->registers.cpsr |= 0x10;

    // Continue processing instruction until the program ends or an error occurs
    while (cpu->registers.pc < sizeof(memory_t)) {
        // Process an instruction based on the current mode (ARM/THUMB)
        if (cpu->registers.cpsr & 0x20) {
            // ARM
            // Fetch the instruction
            uint32_t instruction = *(uint32_t*)&cpu->memory[cpu->registers.pc];

            // Process the instruction
            if (!cpu_process_arm_instruction(cpu, instruction)) {
                return 1;
            }

            // Increment the PC
            cpu->registers.pc += 4;
        } else {
            // THUMB
            // Fetch the instruction
            uint16_t instruction = *(uint16_t*)&cpu->memory[cpu->registers.pc];

            // Process the instruction
            if (!cpu_process_thumb_instruction(cpu, instruction)) {
                return 1;
            }

            // Increment the PC
            cpu->registers.pc += 2;
        }

        // Add a delay to slow down the CPU
        // This is not needed for the emulator to work, but it makes it easier to see what is happening
        // in the emulator
        // usleep(1000);

        // Check if the program has ended
        if (cpu->registers.pc == 0) {
            printf("Program counter is zero, program ended\n");
            // break;
        }

        // Check if the program has crashed
        if (cpu->registers.pc == 0xFFFFFFFF) {
            printf("Program crashed\n");
            break;
        }

        // Check if the program has entered an infinite loop
        if (cpu->registers.pc == cpu->registers.lr) {
            printf("Program entered an infinite loop\n");
            // break;
        }
    }

    return 0;
}

#endif /* __CPU_H__ */