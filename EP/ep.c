#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const unsigned A_SIZE = 128;
const unsigned K_SIZE = 16;

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
    char *K = malloc(K_SIZE*sizeof(char));

    for (int i = 0; i < K_SIZE; i++)
        K[i] = A[i%strlen(A)];
    K[K_SIZE] = '\0';

    printf("K: %s\n", K);
    return K;
}

char **gen_subkeys(int R, char *K) {
    char **L = malloc((4*R + 3)*sizeof(char*));
    for (int i = 0; i < 4*R + 3; i++)
        L[i] = malloc(8*sizeof(char));

    // gera L_0 e L_1
    // desktop é little endian
    // checar no note tmb?
    for (int i = 0; i < 8; i++)
        L[0][i] = K[i];
        L[1][i] = K[i+8];

    for (int j = 2; j <= 4*R + 2; j++) {
        subkeys[j] = subkeys[j-1] + 0x9e3779b97f4a7151 mod 2**64
    }
}

int main(int argc, char **argv) {
    char *A, *K;

    A = malloc(A_SIZE*sizeof(char));

    if (!scanf("%s", A)) {
        printf("scanf failed\n");
    }
    if (!validade_entry(A)) {
        printf("Invalid entry!\n");
        return 0;
    }
    printf("%d\n", strlen(A));
    K = gen_K(A);

    printf("%d\n", validade_entry(A));

    free(A);
    free(K);

    return 0;
}