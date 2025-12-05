#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include "instruction.h"

// TODO: remember to return 0

// helper function declarations
int check_trap (char* line, FILE* output_file);

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
    // write start address to file
    if (fwrite(&start_address, 2 , 1, output_file) != 1) {
        fprintf(stderr, "Problem writing start address to ouput file");
        return 2;
    }
    // while not end of file
    while (fgets(line, 1000, input_file) != NULL) {
       // get rid of newline
       line[strlen(line)] = '\0';
       if (check_trap(line, output_file) == 0) {
            // figure out instruction type
            
            // handle_instruction
            // handle registers
            // handle values

       }
        

    }
    // fgets
    // Read each line
    // Check for traps
    // figure out instruction type
    // emit
    // grab emit return value and write to file
    
}

// checks if there are traps and handles them.
// Returns 0 if there are no traps, 1 otherwise
int check_trap(char* line, FILE* output_file) {
    // fwrite emit file
    int instruction;
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
    instruction = htons(emit_trap(instruction));
    // write to output file
    if (fwrite(&instruction, sizeof(instruction), 1, output_file) != 1 ) {
        fprintf(stderr, "Problem writing trap to output file");
        return 2;
    }
    return 1;
}

int handle_instruction(char* line, FILE* output_file) {
    // Remember: next time you use strtok, call the string with NUll but keep the comma
    char * cur = strtok(line, " ");
    // check if add, and, etc


}
