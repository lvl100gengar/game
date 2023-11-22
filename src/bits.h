#ifndef BITS_H_
#define BITS_H_

#pragma warning(disable: 4146) // Disable warning C4146

#include <limits.h> // for CHAR_BIT
#include <stdint.h> // for uint32_t

// Rotate left
// From https://stackoverflow.com/a/776523
static inline uint32_t rotl32(uint32_t n, unsigned int c)
{
    const unsigned int mask = (CHAR_BIT * sizeof(n) - 1); // assumes width is a power of 2.

    // assert ( (c<=mask) &&"rotate by type width or more");
    c &= mask;
    return (n << c) | (n >> ((-c) & mask));
}

// Rotate right
// From https://stackoverflow.com/a/776523
static inline uint32_t rotr32(uint32_t n, unsigned int c)
{
    const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);

    // assert ( (c<=mask) &&"rotate by type width or more");
    c &= mask;
    return (n >> c) | (n << ((-c) & mask));
}

// Sign extend
static inline int32_t sign_extend(int32_t x, unsigned int b)
{
    // unsigned b; // number of bits representing the number in x
    // int x;      // sign extend this b-bit number to r
    int r;      // resulting sign-extended number
    int const m = 1U << (b - 1); // mask can be pre-computed if b is fixed

    x = x & ((1U << b) - 1);  // (Skip this if bits in x above position b are already zero.)
    r = (x ^ m) - m;

    return r;


    // if ((value >> (bits - 1)) & 1) {
    //     // The sign bit is set, so extend the sign by filling with 1s
    //     uint32_t mask = (1 << (32 - bits)) - 1;
    //     return value | (mask << bits);
    // } else {
    //     // The sign bit is not set, so no sign extension is needed
    //     return value;
    // }
}

#endif // BITS_H_
