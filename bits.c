#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "bits.h"
#include <stdio.h>



// Get the nth bit
uint16_t getbit(uint16_t number, int n) {
    // grab value by bit shifting and & it
    uint16_t bit = (1 & (number >> n));
    return bit;
}

// Get bits at location n that are the given number of bits wide
uint16_t getbits(uint16_t number, int n, int wide) {
    uint16_t mask = 0;
    // for loop to make bit mask
    for (int i = 0; i < wide; i++) {
        // fill with 1s
       mask = (mask << 1) + 1;
    }
    // have correct mask now
    // grab bits and shift them
    return (mask & (number >> n));
}

// Set the nth bit to the given bit value and return the result
uint16_t setbit(uint16_t number, int n) {
    uint16_t mask = 1;
    // nth bit of mask is 1
    mask = (mask << n);
    // ORing it with the mask should set it to 1
    return (mask | number);
}

// Clear the nth bit
uint16_t clearbit(uint16_t number, int n) {
    uint16_t mask = 1;
    // if the bit is 1
    if(getbit(number, n) == 1) {
       mask = (mask << n);
       return (mask ^ number);
    }
    // if the bit is already 0, it can just be returned
    return number;
}

// Sign extend a number of the given bits to 16 bits
uint16_t sign_extend(uint16_t x, int bit_count) {
    // if 1, can do a for loop like earlier
    if (getbit(x, (bit_count -1)) == 1) {
        // set them
        for (int i = bit_count; i < 16; i++) {
            x = setbit(x, i);
        }
    } else {
        // clear them
        for (int i = bit_count; i < 16; i++) {
            clearbit(x, i);
        }
    }
    return x;
}

bool is_positive(uint16_t number) {
    return getbit(number, 15) == 0;
}

bool is_negative(uint16_t number) {
    return getbit(number, 15) == 1;
}
