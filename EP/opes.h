#ifndef OPES_H
#define OPES_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// imprime o valor binario da variavel apontada por ptr, e dete tamanho size
void printBits(size_t const size, void const * const ptr);

// valida uma senha A, retornando 1 caso valida e 0 c.c
int validade_entry(char *A);

// rotaciona circularmente os bits de a em d posições para a esquerda
uint64_t rotate_bit(uint64_t a, uint64_t d);

// terceira operação basica 'bolinha' ou 'ponto', descrita no enunciado,
// usando os vetores EXP e LOG
uint64_t bolinha(uint64_t B, uint64_t C);

// inversa da função acima
uint64_t bolinha_inverse(uint64_t A, uint64_t C);

// gera os vetores EXP e LOG globais
void gen_exp_log();

#endif
