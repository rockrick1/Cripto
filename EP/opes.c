#include "opes.h"

// Imprime um valor com representação binária
void printBits(size_t const size, void const * const ptr) {
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
        printf(".");
    }
    puts("");
}

// Valida uma entrada A
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

// realiza o deslocamente circular de d bits no valor de a
uint64_t rotate_bit(uint64_t a, uint64_t d) {
    uint64_t A = a<<d;
    uint64_t B = a>>(64-d);
    return A|B;
}

// Terceira operação básica
uint64_t bolinha(uint64_t B, uint64_t C) {
    extern uint8_t *EXP;
    uint8_t b[8], c[8];

    for (int i = 7; i >= 0; i--) {
        b[i] = B%256;
        c[i] = C%256;
        B >>= 8;
        C >>= 8;
    }

    uint64_t A = 0;
    for (int i = 0; i < 8; i++) {
        A <<= 8;
        A |= EXP[b[i]] ^ EXP[c[i]];
    }
    return A;
}

// Reversa da ultima operação
// Quero B, tendo A e C
uint64_t bolinha_inverse(uint64_t A, uint64_t C) {
    extern uint8_t *LOG, *EXP;
    uint8_t a[8], c[8];

    for (int i = 7; i >= 0; i--) {
        a[i] = A%256;
        c[i] = C%256;
        A >>= 8;
        C >>= 8;
    }

    uint64_t B = 0;
    uint8_t fBj = 0; // temporarios, para aplicar f reversa
    for (int j = 0; j < 8; j++) {
        B <<= 8;
        fBj = a[j] ^ EXP[c[j]];
        B |= LOG[fBj]; // reversa de f
    }
    return B;
}

// gera os vetores de exp e log globais
void gen_exp_log() {
    // exp[x] = y e log[y] = x
    // y = f(x) = 45^x mod 257 (y = 0 se x = 128)
    extern uint8_t *EXP;
    extern uint8_t *LOG;

    for (int i = 0; i <= 256; i++) {
        long long unsigned tmp = 1;
        // tiramos mod toda vez para nao explodir o valor
        for (int j = 0; j < i; j++)
            tmp = (tmp*45)%257;
        EXP[i] = (uint8_t) tmp;
        LOG[EXP[i]] = i;
    }
}
