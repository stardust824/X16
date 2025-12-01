#include <stdio.h>
#include <stdlib.h>
#include "bits.h"
#include "control.h"
#include "instruction.h"
#include "x16.h"
#include "trap.h"
#include "decode.h"


// Update condition code based on result
void update_cond(x16_t* machine, reg_t reg) {
    uint16_t result = x16_reg(machine, reg);
    if (result == 0) {
        x16_set(machine, R_COND, FL_ZRO);
    } else if (is_negative(result)) {
        x16_set(machine, R_COND, FL_NEG);
    } else {
        x16_set(machine, R_COND, FL_POS);
    }
}



// Execute a single instruction in the given X16 machine. Update
// memory and registers as required. PC is advanced as appropriate.
// Return 0 on success, or -1 if an error or HALT is encountered.
int execute_instruction(x16_t* machine) {
    // Fetch the instruction and advance the program counter
    uint16_t pc = x16_pc(machine);
    uint16_t instruction = x16_memread(machine, pc);
    // increments PC
    x16_set(machine, R_PC, pc + 1);

    if (LOG) {
        fprintf(LOGFP, "0x%x: %s\n", pc, decode(instruction));
    }

    // Variables we might need in various instructions
    reg_t dst, src1, src2, base;
    uint16_t result, indirect, offset, imm, cond, jsrflag, op1, op2, n, p, z;

    // Decode the instruction
    uint16_t opcode = getopcode(instruction);
    switch (opcode) {
        case OP_ADD:
            src1 = getbits(instruction, 6, 3);
            // get value in register
            op1 = x16_reg(machine, src1);
            dst = getbits(instruction, 9, 3);
            // checking how arg2 works and if it needs to be 
            // sign extended
            if (getbit(instruction, 5) == 0) {
                src2 = getbits(instruction, 0, 3);
                op2 = x16_reg(machine, src2);

            } else {
                // sign extend src2
                op2 = sign_extend(getbits(instruction, 0, 5), 5);
            }
            // add the contents of the 2 registers
            result = op1 + op2;
            x16_set(machine, dst, result);
            // update R_COND
            update_cond(machine, dst);
            break;

        case OP_AND:
            src1 = getbits(instruction, 6, 3);
            op1 = x16_reg(machine, src1);
            dst = getbits(instruction, 9, 3);
            if (getbit(instruction, 5) == 0) {
                src2 = getbits(instruction, 0, 3);
                op2 = x16_reg(machine, src2);
            } else {
                op2 = sign_extend(getbits(instruction, 0, 5), 5);
            }
            result = op1 & op2;
            x16_set(machine, dst, result);
            // update R_COND
            update_cond(machine, dst);
            break;

        case OP_NOT:
            dst = getbits(instruction, 9, 3);
            src1 = getbits(instruction, 6, 3);
            op1 = x16_reg(machine, src1);
            result = ~op1;
            x16_set(machine, dst, result);
            // update R_COND
            update_cond(machine, dst);
            break;

        case OP_BR:
            // TODO figure out if need to check if memory is valid
            n = getbit(instruction, 11);
            z = getbit(instruction, 10);
            p = getbit(instruction, 9);
            // get bits in conditional register
            cond = x16_cond(machine);
            // check if the flags and the bits are the same and if all bits are 0
            if (((n && (cond == FL_NEG)) || (z && (cond == FL_ZRO)) || (p && (cond == FL_POS)))  ||
                    (z == 0 && p == 0 && n == 0)) {
                // gets the offset and sign extends it
                offset = sign_extend(getbits(instruction, 0, 9), 9);
                // gets the pc
                op1 = x16_pc(machine);
                result = op1 + offset;
                // set pc to pc + offset
                x16_set(machine, R_PC, result);
            }
            // don't need to updat R_COND 
            break;

        case OP_JMP:
            // I don't think I need an if statement for R7. We'll see
            src1 = getbits(instruction, 6, 3);
            op1 = x16_reg(machine, src1);
            x16_set(machine, R_PC, op1);
            // don't need to update R_COND
            break;

        case OP_JSR:
            // get the program counter
            op1 = x16_pc(machine);
            // set r7 to pc
            x16_set(machine, R_R7, op1);
            // jsrr
            if (getbit(instruction, 11) == 0) {
                src1 = getbits(instruction, 6, 3);
                op2 = x16_reg(machine, src1);
                x16_set(machine, R_PC, op2);
                // jsr
            } else {
                // gets address and sign extends it
                op2 = sign_extend(getbits(instruction, 0, 11), 11);
                // pc = pc + offset
                x16_set(machine, R_PC, (x16_pc(machine) + op2));
            }
            // I don't think I need to update R_COND
            break;

        case OP_LD:
            offset = sign_extend(getbits(instruction, 0, 9), 9);
            dst = getbits(instruction, 9, 3);
            // gets pc
            op2 = x16_pc(machine);
            // index into memory
            result = x16_memread(machine, (op2 + offset));
            // set destination to the value
            x16_set(machine, dst, result);
            update_cond(machine, dst); 
            break;

        case OP_LDI:
            offset = sign_extend(getbits(instruction, 0, 9), 9);
            dst = getbits(instruction, 9, 3);
            // gets pc
            op1 = x16_pc(machine);
            // index into memory
            op2 = x16_memread(machine, (op1 + offset));
            // index into memory again, with the new address
            result = x16_memread(machine, op2);
            // set the destination register
            x16_set(machine, dst, result);
            update_cond(machine, dst);
            break;

        case OP_LDR:
            break;

        case OP_LEA:
            break;

        case OP_ST:
            break;

        case OP_STI:
            break;

        case OP_STR:
            break;

        case OP_TRAP:
            // Execute the trap -- do not rewrite
            return trap(machine, instruction);

        case OP_RES:
        case OP_RTI:
        default:
            // Bad codes, never used
            abort();
    }

    return 0;
}
