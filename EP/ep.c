#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

const unsigned A_SIZE = 128;
const unsigned K_SIZE = 16;


void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=size-1;i>=0;i--)
    {
        for (j=7;j>=0;j--)
        {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}

void print_char(char a) {
    int i;
    for (i = 0; i < 8; i++) {
        printf("%d", !!((a << i) & 0x80));
    }
}

void print_string(char *s) {
    for (int i = 0; i < strlen(s); i++)
        print_char(s[i]);
    printf("\n");
}


int validade_entry(char *A) {
    int digits = 0, letters = 0;

    if (strlen(A) < 8) return 0;
    int i = 0;
    while (*A) {
        if (isdigit(*A++) == 0) letters++;
        else digits++;
        if (digits >= 2 && letters >= 2 && i < 16) return 1;
        i++;
    }
    return 0;
}

char *gen_K(char *A) {
    char *K = malloc(16*sizeof(char) + 1);

    for (int i = 0; i < K_SIZE; i++) {
        K[i] = A[i%strlen(A)];
    }
    K[K_SIZE] = '\0';

    printf("K: %s\n", K);
    printf("k size: %lu\n", sizeof(K));
    return K;
}

// realiza o deslocamente circular de d bits no valor de a
uint64_t rotate_bit(uint64_t a, uint64_t d) {
    uint64_t A = a<<d;
    uint64_t B = a>>(64-d);
    return A|B;
}

uint64_t *gen_subkeys(int R, char *K) {
    uint64_t *L = malloc((4*R + 3)*sizeof(uint64_t));
    uint64_t *subkeys = malloc((4*R + 4)*sizeof(uint64_t));

    for (int i = 0; i < K_SIZE/2; i++) {
        L[0] = L[0]<<8;
        L[1] = L[1]<<8;
        L[0] += K[i];
        L[1] += K[i+8];
        print_char(K[i]);
        printf("\n");
        printBits(sizeof(L[0]), &L[0]);
        printBits(sizeof(L[1]), &L[1]);
    }
    printf("\n");


    uint64_t num1 = 0x9e3779b97f4a7151;
    for (int j = 2; j <= 4*R + 2; j++) {

        L[j] = (L[j-1] + num1); //mod 2**64;
        printf("\t\t");
        printBits(sizeof(num1), &num1);

        printf("anterior:\t");
        printBits(sizeof(L[j-1]), &L[j-1]);

        printf("atual:\t\t");
        printBits(sizeof(L[j]), &L[j]);

        printf("oie\n");
        printf("\n");
    }

    subkeys[0] = 0x8aed2a6bb7e15162;

    uint64_t num2 = 0x7c159e3779b97f4a;
    for (int j = 1; j <= 4*R + 3; j++) {
        subkeys[j] = subkeys[j-1] + num2;
    }

    int i = 0, j = 0;
    uint64_t A = 0, B = 0;
    for (int s = 1; s <= 4*R + 3; s++) {
        // (a)
        subkeys[i] += A + B;
        printf("antes: \n");
        printBits(sizeof(subkeys[i]), &subkeys[i]);
        subkeys[i] = rotate_bit(subkeys[i], 3);
        printf("depois: \n   ");
        printBits(sizeof(subkeys[i]), &subkeys[i]);
        A = subkeys[i];
        i++;

        // (b)
        L[j] += A + B;
        printf("antes: \n");
        printBits(sizeof(L[j]), &L[j]);
        L[j] = rotate_bit(L[j], (A+B)%64);
        printf("depois: \n");
        printBits(sizeof(L[j]), &L[j]);
        B = L[j];
        j++;
        printf("\n\n");
    }

    return subkeys;
}

int main(int argc, char **argv) {
    char *A, *K;
    uint64_t *subkeys;

    A = malloc(A_SIZE*sizeof(char));

    // if (!scanf("%s", A)) {
    //     printf("scanf failed\n");
    // }
    strcpy(A, "a1b2c3d4e5f6g7h8");
    strcpy(A, "00000000000000aa");
    if (!validade_entry(A)) {
        printf("Invalid entry!\n");
        return 0;
    }
    K = gen_K(A);

    printf("%d\n", validade_entry(A));

    subkeys = gen_subkeys(10, K);
    printf("%lu\n", subkeys[3]);

    free(subkeys);
    free(A);
    free(K);

    return 0;
}