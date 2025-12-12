#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include "instruction.h"

// NOTE: This program cannot handle comments that occur 
// directly after instructions with no spaces
// however, it's bad practice to do that anyway
// The program can only handle base 10 numbers

// TODO: br emit, free strings in labels, handle comments after an instruction
#define MAX_LABELS 200
#define MAX_LABEL_LENGTH 200

// struct def
typedef struct label {
    char* name;
    int location;
    char* ptr;
} label_t;

// helper function declarations
int check_trap (char* line, FILE* output_file, int line_num, int write);
int handle_instruction(char* line, FILE* fp, int l_num, label_t* labels, int m_indx);
int handle_value(char* value, int bits, int line_num);
int handle_register(char* reg, int line_num);
int make_label(char* instruction, label_t* lables, int machine_index, int line_num);
int is_label(char* label_name, int line_num);
int get_offset(char* name, label_t* labels, int m_index, int l_num, int bits);


void usage() {
    fprintf(stderr, "Usage: ./xas file");
    exit(1);
}


int main(int argc, char** argv) {
    if (argc != 2) {
        usage();
    }

    // Open file for reading
    FILE* input_file = fopen(argv[1], "r");
    FILE* output_file = fopen("a.obj", "wb");
    char line[1000];
    int start_address = htons(0x3000);
    int line_num = 1;
    // this keeps track of the number of lines that
    // are not comments or whitespace. It is used for lable math
    int machine_index = 0;
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
    // make list of labels
    label_t *labels = calloc(sizeof(label_t), MAX_LABELS);
    
    
    // while not end of file
    for (int i = 0; i < 2; i++) {
        machine_index = 0;
        int label = -1;
        line_num = 1;
        while (fgets(line, 1000, input_file) != NULL) {
            // get rid of newline
            line[strlen(line) -1] = '\0';
            // FIRST PASS
            if (i == 0) {
                label = make_label(line, labels, machine_index, line_num);
                char* cur = strtok(line, " ");
                // ":" Do NOT increment machine index
                // All whitespace =  do NOT increment machine index
                // Only a comment = do NOT increment machine index
                if (cur != NULL && cur[0] != '#' && label != 1) {
                    // it's a trap or instruction
                    machine_index++;
                }
                // SECOND PASS
                // check if trap
            } else if (check_trap(line, output_file, line_num, i) == 0) {
                // if not a label
                if (is_label(line, line_num) != 1) {
                    // increment machine index, if needed
                    machine_index += handle_instruction(line, output_file, line_num, labels, machine_index);

                }
            } else {
                // it's a trap
                machine_index++;
            }
            line_num ++;
        }
        // fseek to beginning
        fseek(input_file, 0, SEEK_SET);
    }
    // free all the malloced strings in labels
    for (int i = 0; i < MAX_LABELS; i++) {
        if (labels[i].ptr != NULL) {
            free(labels[i].ptr);
        } else {
            // don't want to free things we didn't malloc
            break;
        }
    }
    free(labels);
    fclose(input_file);
    fclose(output_file);
    return 0;
}

// checks if there are traps and handles them.
// Returns 0 if there are no traps, 1 otherwise
int check_trap(char* line, FILE* output_file, int line_num, int write) {
    // fwrite emit file
    int instruction;
    short encoded;
    int length = strlen(line);
    char copy[1000];
    strcpy(copy, line);
    // should handle spaces and comments
    line = strtok(copy, " ");
    if (line == NULL) {
        // whitespace
        return 0;
    }
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
    if (write == 1) {
        if (fwrite(&encoded, sizeof(encoded), 1, output_file) != 1) {
            fprintf(stderr, "Line number: %d Problem writing trap to output.\n", line_num);
            exit(2);
        }
    }
    return 1;
}

// return 1 if increment instruction, 0 otherwise
int handle_instruction(char* line, FILE* fp, int l_num, label_t* labels, int m_indx) {
    // Remember: next time you use strtok, call the string with NUll but keep the comma
    char * cur = strtok(line, " ");
    int dst, src1, src2, imm, on, z, p, n, base, offset, trap, val;
    short encoding;
    // check if pure whitespace
    if (cur == NULL) {
        // is just whitespace
        return 0;
    }
    // check if comment
    if (cur[0] == '#') {
        return 0;
    } else if (strcmp(cur, "add") == 0) {
        dst = handle_register(strtok(NULL, " "), l_num);
        src1 = handle_register(strtok(NULL, " "), l_num);
        cur = strtok(NULL, " ");
        if (cur == NULL) {
            fprintf(stderr, "Line: %d. Bad argument input. NULL. \n", l_num);
            exit(2);
        }
        // add with 2 registers
        if (cur[0] == '%') {
            src2 = handle_register(cur, l_num);
            encoding = htons(emit_add_reg(dst, src1, src2));
        } else {
            imm = handle_value(cur, 5, l_num);
            encoding = htons(emit_add_imm(dst, src1, imm));
        } 

    } else if (strcmp(cur, "and") == 0) {
        dst = handle_register(strtok(NULL, " "), l_num);
        src1 = handle_register(strtok(NULL, " "), l_num);
        cur = strtok(NULL, " ");
        if (cur == NULL) {
            fprintf(stderr, "Line: %d. Bad argument input. NULL. \n", l_num);
            exit(2);
        }
        // and with 2 registers
        if (cur[0] == '%') {
            src2 = handle_register(cur, l_num);
            encoding = htons(emit_and_reg(dst, src1, src2));
        } else {
            // and with immediate value
            imm = handle_value(cur, 5, l_num);
            encoding = htons(emit_and_imm(dst, src1, imm));
        }
    } else if (strcmp(cur, "not") == 0) {
        dst = handle_register(strtok(NULL, " "), l_num);
        src1 = handle_register(strtok(NULL, " "), l_num);
        encoding = htons(emit_not(dst, src1));
    } else if (strcmp(cur, "br") == 0) {
        offset = get_offset(strtok(NULL, " "), labels, m_indx, l_num, 9);
        encoding = htons(emit_br(0, 0, 0, offset));
    } else if (strcmp(cur, "brn") == 0) {
        offset = get_offset(strtok(NULL, " "), labels, m_indx, l_num, 9);
        encoding = htons(emit_br(1, 0, 0, offset));
    } else if (strcmp(cur, "brp") == 0) {
        offset = get_offset(strtok(NULL, " "), labels, m_indx, l_num, 9);
        encoding = htons(emit_br(0, 0, 1, offset));
    } else if (strcmp(cur, "brz") == 0) {
        offset = get_offset(strtok(NULL, " "), labels, m_indx, l_num, 9);
        encoding = htons(emit_br(0, 1, 0, offset));
    }  else if (strcmp(cur, "brnzp") == 0) {
        offset = get_offset(strtok(NULL, " "), labels, m_indx, l_num, 9);
        encoding = htons(emit_br(1, 1, 1, offset));
    } else if (strcmp(cur, "brnz") == 0) {
        offset = get_offset(strtok(NULL, " "), labels, m_indx, l_num, 9);
        encoding = htons(emit_br(1, 1, 0, offset));
    } else if (strcmp(cur, "brnp") == 0) {
        offset = get_offset(strtok(NULL, " "), labels, m_indx, l_num, 9);
        encoding = htons(emit_br(1, 0, 1, offset));
    } else if (strcmp(cur, "brzp") == 0) {
        offset = get_offset(strtok(NULL, " "), labels, m_indx, l_num, 9);
        encoding = htons(emit_br(0, 1, 1, offset));
    }  else if (strcmp(cur, "jmp") == 0) {
        base = handle_register(strtok(NULL, " "), l_num);
        encoding = htons(emit_jmp(base));
    } else if (strcmp(cur, "jsr") == 0) {
        offset = get_offset(strtok(NULL, " "), labels, m_indx, l_num, 11);
        encoding = htons(emit_jsr(offset));
    } else if (strcmp(cur, "jsrr") == 0) {
        base = handle_register(strtok(NULL, " "), l_num);
        encoding = htons(emit_jsrr(base));
    } else if (strcmp(cur, "ld") == 0) {
        dst = handle_register(strtok(NULL, " "), l_num);
        offset = get_offset(strtok(NULL, " "), labels, m_indx, l_num, 9);
        encoding = htons(emit_ld(dst, offset));
    } else if (strcmp(cur, "ldi") == 0) {
        dst = handle_register(strtok(NULL, " "), l_num);
        offset = get_offset(strtok(NULL, " "), labels, m_indx, l_num, 9);
        encoding = htons(emit_ldi(dst, offset));
    } else if (strcmp(cur, "ldr") == 0) {
        dst = handle_register(strtok(NULL, " "), l_num);
        src1 = handle_register(strtok(NULL, " "), l_num);
        offset = handle_value(strtok(NULL, " "), 6, l_num);
        encoding = htons(emit_ldr(dst, src1, offset));
    } else if (strcmp(cur, "lea") == 0) {
        dst = handle_register(strtok(NULL, " "), l_num);
        offset = get_offset(strtok(NULL, " "), labels, m_indx, l_num, 9);
        encoding = htons(emit_lea(dst, offset));
    } else if (strcmp(cur, "ret") == 0) {
        fprintf(stderr, "Instruction ret is not used in X16\n");
        exit(2);
    } else if (strcmp(cur, "rti") == 0) {
        fprintf(stderr, "Instruction rti is not used in X16\n");
        exit(2);
    } else if (strcmp(cur, "st") == 0) {
        // TODO test
        // take a memeory address (allegedly)
        src1 = handle_register(strtok(NULL, " "), l_num);
        offset = get_offset(strtok(NULL, " "), labels, m_indx, l_num, 9);
        encoding = htons(emit_st(src1, offset));
    } else if (strcmp(cur, "sti") == 0) {
        // TODO test
        // memory address (allegedly)
        src1 = handle_register(strtok(NULL, " "), l_num);
        offset = get_offset(strtok(NULL, " "), labels, m_indx, l_num, 9);
        encoding = htons(emit_sti(src1, offset));
    } else if (strcmp(cur, "str") == 0) {
        src1 = handle_register(strtok(NULL, " "), l_num);
        base = handle_register(strtok(NULL, " "), l_num);
        offset = handle_value(strtok(NULL, " "), 6, l_num);
        encoding = htons(emit_str(src1, base, offset));
    } else if (strcmp(cur, "trap") == 0) {
        trap = handle_value(strtok(NULL, " "), 8, l_num);
        if (trap != TRAP_GETC && trap != TRAP_OUT && trap != TRAP_PUTS &&
                trap != TRAP_IN && trap != TRAP_PUTSP && trap != TRAP_HALT) {
            fprintf(stderr, "Line: %d. Not a valid trap value\n", l_num);
        }
        encoding = htons(emit_trap(trap));
    } else if (strcmp(cur, "val") == 0) {
        // NOTE: val works but xod doesn't understand it and reads it as branch.
        // this is normal, as per the handout
        val = handle_value(strtok(NULL, " "), 16, l_num);
        encoding = htons(emit_value(val));

    } else {
        fprintf(stderr, "Line: %d. Unknown instruction:%s \n", l_num, cur);
        exit(2);
    }
    // write to output
    // check if writing
    if (fwrite(&encoding, sizeof(encoding), 1, fp) != 1) {
        fprintf(stderr, "Line: %d. Problem writing to output file\n", l_num);
        exit(2);
    }

    // TODO Figure out if do anything else
    // TODO CHECK IF THERE ARE MORE THINGS ON THE LINE. IF COMMENT, OKAY. OTHERIWSE, ERROR
    return 1;
}

// returns register number, else exits
int handle_register(char* reg, int line_num) {
    if (reg == NULL) {
        fprintf(stderr, "Line: %d. Bad register input.\n", line_num);
        exit(2);
    }
    // handle comma
    if (reg[strlen(reg) -1] == ',') {
        reg[strlen(reg) -1] = '\0'; 
    }
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
    // value is null. Prevents seg faults
    if (value == NULL) {
        fprintf(stderr, "Line: %d. Bad value input.\n", line_num);
        exit(2);
    } else if (value[0] != '$') {
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

// if instruction is NOT a label, returns 0
// if it is a label,returns 1 and the label is put into 
// an array of structs that store the label and the offset
int make_label(char* instruction, label_t* labels, int machine_index, int line_num) {
    char* copy = malloc(MAX_LABEL_LENGTH);
    char* pointer = copy;
    strcpy(copy, instruction);
    copy = strtok(copy, " ");
    // whitespace
    if (copy == NULL) {
        free(pointer);
        return 0; 
    }
    if (copy[strlen(copy) -1] != ':') {
        // not a label
        free(pointer);
        return 0;
    }

    if (copy[0] == '#') {
        // it's a comment. Not a label
        free(pointer);
        return 0;
    }
    // gets rid of the ':"
    copy[strlen(copy) -1] = '\0';
 
    if (strtok(NULL, " ") != NULL) {
        fprintf(stderr, "Line: %d. Label name cannot have spaces\n", line_num);
        exit(2);
    }
    // index through labels until find an empty spot
    for (int i = 0; i <= MAX_LABELS; i++) {
        // too many labels. Not allowed
        if (i == MAX_LABELS) {
            fprintf(stderr, "Line: %d. Too many labels.\
                    Change MAX_LABELS or write better code\n", line_num);
            exit(2);
        } else if (labels[i].name == NULL) {
            // put it in the labels
            labels[i].name = copy;
            labels[i].location = machine_index;
            labels[i].ptr = pointer;
            return 1;
        }
    }
    // should not get here
    return -1;
}

// Precondition: label_name does not contain whitespace or comments
// returns the offset needed to jump to label
int get_offset(char* name, label_t* labels, int m_index, int l_num, int bits) {
    int label_location = -1;
    int offset; 
    for (int i = 0; i < MAX_LABELS; i++) {
        if (labels[i].name != NULL && strcmp(labels[i].name, name) == 0) {
            label_location = labels[i].location;
            break;
        }
    }
    // machine index should never be negative one. label not real
    if (label_location == -1) {
        fprintf(stderr, "Line: %d. Label does not exist", l_num);
        exit(2);
    }
    offset = (label_location - (m_index + 1));
    // TODO check side
    int max_size = 1 << (bits - 1);
    if (offset >= max_size || offset < (max_size * -1)) {
        fprintf(stderr, "Line: %d. Value too big or too small\n", l_num);
        exit(2);
    }

    return offset;
}

// returns 1 if it's a valid label, 0 otherwise
int is_label(char* label_name, int line_num) {
    char* copy = malloc(MAX_LABEL_LENGTH);
    char* pointer = copy;
    strcpy(copy, label_name);
    // strtok deals with whitespace
    copy = strtok(copy, " ");
    // whitespace
    if (copy == NULL) {
        free(pointer);
        return 0;
    }
    if (copy[strlen(copy) -1] != ':') {
        // not a label
        free(pointer);
        return 0;
    }
    if (copy[0] == '#') {
        // it's a comment. Not a label
        free(pointer);
        return 0;
    }
    free(pointer);
    return 1;
}
