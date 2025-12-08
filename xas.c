#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include "instruction.h"
#include <limits.h>

// NOTE: This program cannot handle numbers written in hex. 
// Maximum labels: 200
// TODO: remember to return 0, FREE LABELS!!!
#define MAX_LABELS 200

// struct def
typedef struct label {
    char* name;
    int location;
} label_t;

// helper function declarations
int check_trap (char* line, FILE* output_file, int line_num, int write);
int handle_instruction(char* line, FILE* output_file, int line_num, int write);
int handle_value(char* value, int bits, int line_num);
int handle_register(char* reg, int line_num);
int make_label(char* instruction, label_t* lables, int machine_index, int line_num, int make);
int get_offset(char* label_name, label_t* labels, int machine_index, int line_num);


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
    
    // TODO: While loop to read file and scan for labels
    while (fgets(line, 1000, input_file) != NULL) {

    }

    // while not end of file
    for (int i = 0; i < 2; i++) {
        machine_index = 0;
        int label = -1;
        while (fgets(line, 1000, input_file) != NULL) {
            printf("Line: %d Getting rid of newline\n", line_num);
            // get rid of newline
            line[strlen(line) -1] = '\0';
            // check if label
            if (i == 0) {
                // only want it to make a label on the first pass
                label = make_label(line, labels, machine_index, line_num, i);
                // check if trap
            } else if (check_trap(line, output_file, line_num, i) == 0) {
                // it's an instruction
                printf("Line: %d. Handling instruction\n", line_num);
                // increment machine index, if needed
                machine_index += handle_instruction(line, output_file, line_num, i);
            } else {
                // it's a trap
                machine_index++;
            }
            line_num ++;
        }
        // fseek to beginning
        fseek(input_file, 0, SEEK_SET); 
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
    line = strtok(line, " ");
    // handles spaces
    //for (int i = 0; i < length; i++) {
        //if (line[0] == ' ') {
            //line++;
        //} else {
            //break;
        //}
    //}
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
int handle_instruction(char* line, FILE* output_file, int line_num, int write) {
    // Remember: next time you use strtok, call the string with NUll but keep the comma
    char * cur = strtok(line, " ");
    int dst, src1, src2, imm, on, z, p, n, base, offset, trap, val;
    short encoding;
    printf("In handle instruction\n");
    // check if pure whitespace
    if (cur == NULL) {
        // is just whitespace
        return 0;
    }
    // check if comment
    if (cur[0] == '#') {
        printf("Comment\n");
        return 0;
    } else if (strcmp(cur, "add") == 0) {
        printf("In add\n");
        printf("%s\n", line);
        dst = handle_register(strtok(NULL, " "), line_num);
        printf("Got destination\n");
        src1 = handle_register(strtok(NULL, " "), line_num);
        printf("Got src1\n");
        cur = strtok(NULL, " ");
        if (cur == NULL) {
            fprintf(stderr, "Line: %d. Bad argument input. NULL. \n", line_num);
            exit(2);
        }
        // add with 2 registers
        printf("Checking which add to emit\n");
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
        if (cur == NULL) {
            fprintf(stderr, "Line: %d. Bad argument input. NULL. \n", line_num);
            exit(2);
        }
        // and with 2 registers
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
        fprintf(stderr, "Line: %d. Unknown instruction:%s \n", line_num, cur);
        exit(2);
    }
    printf("writing to output\n");
    // write to output
    // check if writing
    if (write == 1) {
        if (fwrite(&encoding, sizeof(encoding), 1, output_file) != 1) {
            fprintf(stderr, "Line: %d. Problem writing to output file\n", line_num);
            exit(2);
        }
    }

    // TODO Figure out if do anything else
    // TODO CHECK IF THERE ARE MORE THINGS ON THE LINE. IF COMMENT, OKAY. OTHERIWSE, ERROR
    return 1;
}

// returns register number, else exits
int handle_register(char* reg, int line_num) {
    printf("in handle register\n");
    printf("%s\n", reg);
    if (reg == NULL) {
        fprintf(stderr, "Line: %d. Bad register input.\n", line_num);
        exit(2);
    }
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
int make_label(char* instruction, label_t* labels, int machine_index, int line_num, int make) {
    if (instruction[strlen(instruction) -1] != ':') {
        // not a label
        return 0;
    }
    // get rid of the ':'
    instruction[strlen(instruction) -1] = '\0';
    // deals with whitespace
    instruction = strtok(instruction, " ");
    if (instruction == NULL) {
        fprintf(stderr, "Line: %d. Invalid label name\n", line_num);
        exit(2);
    }
    // check if spaces in label name
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
            labels[i].name = instruction;
            labels[i].location = machine_index;
            return 1;
        }
    }
    // should not get here
    return -1;
}

// Precondition: label_name does not contain whitespace or comments
// returns the offset needed to jump to label
int get_offset(char* label_name, label_t* labels, int machine_index, int line_num) {
    int label_location = -1;
    int offset;
    
    for (int i = 0; i <= MAX_LABELS; i++) {
        if (strcmp(labels[i].name, label_name) == 0) {
            label_location = labels[i].location;
            break;
        }
    }
    if (label_location == -1) {
        fprintf(stderr, "Line: %d. Label does not exist", line_num);
        exit(2);
    }
    
    offset = (label_location - (machine_index + 1));
    return offset;
}
