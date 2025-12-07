#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include "instruction.h"
#include <limits.h>

// NOTE: This program cannot handle numbers written in hex
// TODO: remember to return 0, handle val command, handle comments

// helper function declarations
int check_trap (char* line, FILE* output_file, int line_num);
void handle_instruction(char* line, FILE* output_file, int line_num);
int handle_value(char* value, int bits, int line_num);
int handle_register(char* reg, int line_num);

void usage() {
    fprintf(stderr, "Usage: ./xas file");
    exit(1);
}


int main(int argc, char** argv) {
    if (argc != 2) {
        usage();
    }

    // TODO: Your code here
    // Open file for reading
    FILE* input_file = fopen(argv[1], "r");
    FILE* output_file = fopen("a.obj", "wb");
    char line[1000];
    int start_address = htons(0x3000);
    int line_num = 1;
    if (input_file == NULL) {
        fprintf(stderr, "Problem opening file\n");
        return 2;
    }
    if (output_file == NULL) {
        fprintf(stderr, "Problem creating a.obj\n");
        return 2;
    }
    // write start address to file
    if (fwrite(&start_address, 2, 1, output_file) != 1) {
        fprintf(stderr, "Problem writing start address to ouput file\n");
        return 2;
    }
    // while not end of file
    while (fgets(line, 1000, input_file) != NULL) {
        // get rid of newline
        line[strlen(line) -1] = '\0';
        // not a comment
        if (line[0] != '#') {
            // not a trap. Check trap will handle all traps
            if (check_trap(line, output_file, line_num) == 0) {
                // it's an instruction
                handle_instruction(line, output_file, line_num);
            }
        }
        line_num ++;
    }
    return 0;
}

// checks if there are traps and handles them.
// Returns 0 if there are no traps, 1 otherwise
int check_trap(char* line, FILE* output_file, int line_num) {
    // fwrite emit file
    int instruction;
    short encoded;
    if (strcmp(line, "putc") == 0) {
        instruction = TRAP_OUT;
    } else if(strcmp(line, "getc") == 0) {
        instruction = TRAP_GETC;
    } else if(strcmp(line, "puts") == 0) {
        instruction = TRAP_PUTS;
    } else if(strcmp(line, "enter") == 0) {
        instruction = TRAP_IN;
    } else if(strcmp(line, "putsp") == 0) {
        instruction = TRAP_PUTSP;
    } else if(strcmp(line, "halt") == 0) {
        instruction = TRAP_HALT;
    } else {
        // not a trap
        return 0;
    } 
    // get emit and swap to big endian
    encoded = htons(emit_trap(instruction));
    // write to output file
    if (fwrite(&encoded, sizeof(encoded), 1, output_file) != 1) {
        fprintf(stderr, "Line number: %d Problem writing trap to output.\n", line_num);
        exit(2);
    }
    return 1;
}

// return 0 on succes, 1 otherwise
void handle_instruction(char* line, FILE* output_file, int line_num) {
    // Remember: next time you use strtok, call the string with NUll but keep the comma
    char * cur = strtok(line, " ");
    int dst, src1, src2, imm, on, z, p, n, base, offset, trap, val;
    short encoding;
    // check if add, and, etc
    if (strcmp(cur, "add") == 0) {
        dst = handle_register(strtok(NULL, " "), line_num);
        src1 = handle_register(strtok(NULL, " "), line_num);
        cur = strtok(NULL, " ");
        // add with 2 registers
        if (cur[0] == '%') {
            src2 = handle_register(cur, line_num);
            encoding = htons(emit_add_reg(dst, src1, src2));
        } else {
            imm = handle_value(cur, 5, line_num);
            encoding = htons(emit_add_imm(dst, src1, imm));
        } 

    } else if (strcmp(cur, "and") == 0) {
        dst = handle_register(strtok(NULL, " "), line_num);
        src1 = handle_register(strtok(NULL, " "), line_num);
        cur = strtok(NULL, " ");
        // a d with 2 registers
        if (cur[0] == '%') {
            src2 = handle_register(cur, line_num);
            encoding = htons(emit_and_reg(dst, src1, src2));
        } else {
            // and with immediate value
            imm = handle_value(cur, 5, line_num);
            encoding = htons(emit_and_imm(dst, src1, imm));
        }
    } else if (strcmp(cur, "not") == 0) {
        dst = handle_register(strtok(NULL, " "), line_num);
        src1 = handle_register(strtok(NULL, " "), line_num);
        encoding = htons(emit_not(dst, src1));
    } else if (strcmp(cur, "br") == 0) {
        // TODO DO AFTER LABELS

    } else if (strcmp(cur, "jmp") == 0) {
        base = handle_register(strtok(NULL, " "), line_num);
        encoding = htons(emit_jmp(base));

    } else if (strcmp(cur, "jsr") == 0) {
        // TODO after labels

    } else if (strcmp(cur, "jsrr") == 0) {
        base = handle_register(strtok(NULL, " "), line_num);
        encoding = htons(emit_jsrr(base));
    } else if (strcmp(cur, "ld") == 0) {
        dst = handle_register(strtok(NULL, " "), line_num);
        offset = handle_value(strtok(NULL, " "), 9, line_num);
        encoding = htons(emit_ld(dst, offset));
    } else if (strcmp(cur, "ldi") == 0) {
        // TODO after labels

    } else if (strcmp(cur, "ldr") == 0) {
        dst = handle_register(strtok(NULL, " "), line_num);
        src1 = handle_register(strtok(NULL, " "), line_num);
        offset = handle_value(strtok(NULL, " "), 6, line_num);
        encoding = htons(emit_ldr(dst, src1, offset));
    } else if (strcmp(cur, "lea") == 0) {
        // TODO after labels

    } else if (strcmp(cur, "ret") == 0) {
        fprintf(stderr, "Instruction ret is not used in X16\n");
        exit(2);
    } else if (strcmp(cur, "rti") == 0) {
        fprintf(stderr, "Instruction rti is not used in X16\n");
        exit(2);
    } else if (strcmp(cur, "st") == 0) {
        // TODO after labels

    } else if (strcmp(cur, "sti") == 0) {
        // TODO after labels

    } else if (strcmp(cur, "str") == 0) {
        src1 = handle_register(strtok(NULL, " "), line_num);
        base = handle_register(strtok(NULL, " "), line_num);
        offset = handle_value(strtok(NULL, " "), 6, line_num);
        encoding = htons(emit_str(src1, base, offset));
    } else if (strcmp(cur, "trap") == 0) {
        trap = handle_value(strtok(NULL, " "), 8, line_num);
        if (trap != TRAP_GETC && trap != TRAP_OUT && trap != TRAP_PUTS &&
                trap != TRAP_IN && trap != TRAP_PUTSP && trap != TRAP_HALT) {
            fprintf(stderr, "Line: %d. Not a valid trap value\n", line_num);
        }
        encoding = htons(emit_trap(trap));
    } else if (strcmp(cur, "val") == 0) {
        // NOTE: val works but xod doesn't understand it and reads it as branch.
        // this is normal, as per the handout
        val = handle_value(strtok(NULL, " "), 16, line_num);
        encoding = htons(emit_value(val));

    } else {
        fprintf(stderr, "Line: %d. Unknown instruction\n", line_num);
        exit(2);
    }
    // write to output
    if (fwrite(&encoding, sizeof(encoding), 1, output_file) != 1) {
        fprintf(stderr, "Line: %d. Problem writing to output file\n", line_num);
        exit(2);
    }

    // TODO Figure out if do anything else
    // TODO CHECK IF THERE ARE MORE THINGS ON THE LINE. IF COMMENT, OKAY. OTHERIWSE, ERROR
}

// returns register number, else exits
int handle_register(char* reg, int line_num) {
    printf("%s\n", reg);
    // handle comma
    if (reg[strlen(reg) -1] == ',') {
        reg[strlen(reg) -1] = '\0'; 
    }
    printf("%s\n", reg);
    if (strcmp(reg, "%r0") == 0) {
        return R_R0;
    } else if (strcmp(reg, "%r1") == 0) {
        return R_R1;
    } else if (strcmp(reg, "%r2") == 0) {
        return R_R2;
    } else if (strcmp(reg, "%r3") == 0) {
        return R_R3;
    } else if (strcmp(reg, "%r4") == 0) {
        return R_R4;
    } else if (strcmp(reg, "%r5") == 0) {
        return R_R5;
    } else if (strcmp(reg, "%r6") == 0) {
        return R_R6;
    } else if (strcmp(reg, "%r7") == 0) {
        return R_R7;
    } else {
        fprintf(stderr, "Line: %d. Problem reading register. Check %%\n", line_num);
        exit(2);
    }
}

// returns the value as an integer
// check if it is the right number of bits, errors if not
int handle_value(char* value, int bits, int line_num) {
    if (value[0] != '$') {
           fprintf(stderr, "Line: %d. Values should start with a \"$\"\n", line_num); 
           exit(2);
    }
    // handle comma
    if (value[strlen(value) -1] == ',') {
        value[strlen(value) -1] = '\0'; 
    }
    char* endptr;
    long number;
    // get rid of $
    value = &value[1];
    number = strtol(value, &endptr, 10);
    // sanity check
    if (number == LONG_MAX || number == LONG_MIN) {
        fprintf(stderr, "Line: %d. Overflow error with value conversion\n", line_num);
        exit(2);
    }
    int max_size = 1 << (bits - 1);
    if (number >= max_size || number < (max_size * -1)) {
        fprintf(stderr, "Line: %d. Value too big or too small\n", line_num);
        exit(2);
    }
    return (int) number;
}
