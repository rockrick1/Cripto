#ifndef OPES_H
#define OPES_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

void printBits(size_t const size, void const * const ptr);
int validade_entry(char *A);
uint64_t rotate_bit(uint64_t a, uint64_t d);
uint64_t bolinha(uint64_t B, uint64_t C);
uint64_t bolinha_inverse(uint64_t A, uint64_t C) ;

#endif