
#include "opes.h"

const unsigned A_SIZE = 128;
const unsigned K_SIZE = 16;

uint8_t *EXP;
uint8_t *LOG;

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



void K128(uint64_t *subkeys, uint64_t Xa, uint64_t Xb) {
    int R = 12;

    uint64_t ka, kb, ke, kf;
    uint64_t Xla = Xa, Xlb = Xb;
    uint64_t Xle = Xa, Xlf = Xb;
    for (int i = 0, j = 1; i < R; i++, j += 4) {
        // se for a primeira iteração, usa o valor dado,
        // senão, usa o valor resultante da ultima iteração
        if (i != 0) {
            Xla = Xle;
            Xlb = Xlf;
        }
        // primeira parte
        ka = subkeys[j];
        kb = subkeys[j+1];

        Xla = bolinha(Xla, ka);
        Xlb = Xlb + kb;

        // segunda parte
        Xle = Xla;
        Xlf = Xlb;
        ke = subkeys[j+2];
        kf = subkeys[j+3];
        // 1
        uint64_t Y1, Y2, Z;
        Y1 = Xle ^ Xlf;
        // 2
        Y2 = bolinha(((bolinha(ke, Y1)) + Y1), kf);
        Z = (bolinha(ke, Y1)) + Y2;
        // 3
        // saida da iteração, entrada da proxima iteração
        Xle = Xle ^ Z;
        Xlf = Xlf ^ Z;
    }
    // Transofrmação T
    uint64_t XeFinal, XfFinal;
    XeFinal = bolinha(Xlf, subkeys[4*R + 1]);
    XfFinal = Xle + subkeys[4*R + 2];
}

uint64_t *gen_subkeys(int R, char *K) {
    uint64_t *L = malloc((4*R + 3)*sizeof(uint64_t));
    uint64_t *subkeys = malloc((4*R + 4)*sizeof(uint64_t));

    for (int i = 0; i < K_SIZE/2; i++) {
        L[0] = L[0]<<8;
        L[1] = L[1]<<8;
        L[0] |= K[i];
        L[1] |= K[i+8];
        print_char(K[i]);
        printf("\n");
        printBits(sizeof(L[0]), &L[0]);
        printBits(sizeof(L[1]), &L[1]);
    }
    printf("\n");


    uint64_t num1 = 0x9e3779b97f4a7151;
    for (int j = 2; j <= 4*R + 2; j++) {

        L[j] = (L[j-1] + num1); //mod 2**64;
        // printf("\t\t");
        // printBits(sizeof(num1), &num1);
        //
        // printf("anterior:\t");
        // printBits(sizeof(L[j-1]), &L[j-1]);
        //
        // printf("atual (%d):\t\t", j);
        // printBits(sizeof(L[j]), &L[j]);
        //
        // printf("oie\n");
        // printf("\n");
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


// gera os vetores de exp e log globais
void gen_exp_log() {
    // exp[x] = y e log[y] = x
    // y = f(x) = 45^x mod 257 (y = 0 se x = 128)
    EXP = malloc(257*sizeof(uint8_t));
    LOG = malloc(257*sizeof(uint8_t));

    for (int i = 0; i <= 256; i++) {
        long long unsigned tmp = 1;
        for (int j = 0; j < i; j++)
            tmp = (tmp*45)%257;
        EXP[i] = (uint8_t) tmp;
        LOG[EXP[i]] = i;
    }
}

int main(int argc, char **argv) {
    char *A, *K;
    uint64_t *subkeys;

    gen_exp_log();
    uint64_t ble = bolinha(0x9e3779b97f4a7151, 0x1324819741ff2312);
    // return 0;

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
    free(EXP);
    free(LOG);
    free(A);
    free(K);

    return 0;
}