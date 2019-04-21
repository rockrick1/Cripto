
#include "opes.h"

const unsigned A_SIZE = 128;
const unsigned K_SIZE = 16;
const unsigned R = 12;

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

uint64_t *gen_subkeys(int R, char *K) {
    uint64_t *L = malloc((4*R + 3)*sizeof(uint64_t));
    uint64_t *subkeys = malloc((4*R + 4)*sizeof(uint64_t));

    for (int i = 0; i < K_SIZE/2; i++) {
        L[0] = L[0]<<8;
        L[1] = L[1]<<8;
        L[0] |= K[i];
        L[1] |= K[i+8];
        // print_char(K[i]);
        // printf("\n");
        // printBits(sizeof(L[0]), &L[0]);
        // printBits(sizeof(L[1]), &L[1]);
    }
    // printf("\n");


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
        // printf("antes: \n");
        // printBits(sizeof(subkeys[i]), &subkeys[i]);
        subkeys[i] = rotate_bit(subkeys[i], 3);
        // printf("depois: \n   ");
        // printBits(sizeof(subkeys[i]), &subkeys[i]);
        A = subkeys[i];
        i++;

        // (b)
        L[j] += A + B;
        // printf("antes: \n");
        // printBits(sizeof(L[j]), &L[j]);
        L[j] = rotate_bit(L[j], (A+B)%64);
        // printf("depois: \n");
        // printBits(sizeof(L[j]), &L[j]);
        B = L[j];
        j++;
        // printf("\n\n");
    }

    return subkeys;
}


uint64_t *K128_encript(uint64_t *subkeys, uint64_t Xa, uint64_t Xb) {
    uint64_t *ret = malloc(2*sizeof(uint64_t));

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
    ret[0] = XeFinal;
    ret[1] = XfFinal;
    return ret;
}


uint64_t *K128_decript(uint64_t *subkeys, uint64_t XeFinal, uint64_t XfFinal) {
    uint64_t *ret = malloc(2*sizeof(uint64_t));

    uint64_t ka, kb, ke, kf;
    uint64_t Xlf, Xle, Xe, Xf;
    uint64_t Xa, Xb;
    // primeiro, revertemos a tranformação T
    Xlf = bolinha_inverse(XeFinal, subkeys[4*R + 1]); // inversa de bolinha
    Xle = XfFinal - subkeys[4*R + 2]; // inversa da soma

    // Agora temos os valores resultantes da ultima iteração
    // Temos que refazer as iterações, mas com as operações reversas

    for (int i = 0, j = 4*R; i < R; i++, j -= 4) {
        // reversa da segunda parte
        ke = subkeys[j-1];
        kf = subkeys[j];

        uint64_t Y1, Y2, Z;
        // Y1 = Xle xor Z xor Xlf xor Z = Xle xor Xlf
        Y1 = Xle ^ Xlf;
        Y2 = bolinha(((bolinha(ke, Y1)) + Y1), kf);
        Z = (bolinha(ke, Y1)) + Y2;
        Xe = Xle ^ Z;
        Xf = Xlf ^ Z;

        // reversa da primeira parte
        ka = subkeys[j-3];
        kb = subkeys[j-2];

        Xa = bolinha_inverse(Xe, ka);
        Xb = Xf - kb;

        // prepara o proximo round
        Xle = Xa;
        Xlf = Xb;
    }
    ret[0] = Xa;
    ret[1] = Xb;
    return ret;
}


void encrypt_file(char *filename, uint64_t *subkeys) {
    FILE *file;
    char *buffer;
    uint64_t *filebits;
    long filelen;
    int i, f;

    file = fopen(filename, "rb");
    fseek(file, 0, SEEK_END);
    filelen = ftell(file);
    rewind(file);
    buffer = (char *)malloc((filelen+1)*sizeof(char));
    filebits = malloc((filelen+1)*sizeof(uint64_t));


    for(i = 0; i < filelen; i++) {
        if (fread(buffer + i, 1, 1, file));
    }

    // monta o vetor de bits do arquivo filebits
    uint64_t X = 0;
    for(i = 0, f = 0; i < filelen; i++) {
        // monta X
        X <<= 8;
        X |= buffer[i];
        // printBits(sizeof(buffer[i]), &buffer[i]);

        // se esse for o ultimo byte, faz o padding
        if (i + 1 == filelen) {
            // tamanho do ultimo bloco, em bytes
            int last = (i+1)%8;
            // se o ultimo bloco nao for de 64 bits, faz o padding com uns
            if (last != 0) {
                int remaining = 8 - last; // quantos bytes faltam no final
                uint8_t uns = 0xff; // 1111 1111
                // nesse ponto, X tem o fim do arquivo, entao deslocamos
                // e adicionamos uns
                for (int k = 0; k < remaining; k++) {
                    X <<= 8;
                    X |= uns;
                }
                filebits[f] = X;
                printBits(sizeof(X), &X);
                printf("last %d, remain %d\n", last, remaining);
                X = 0;
            }
            // agora checamos se o ultimo bloco tem 128 bits
            // se ele tinha menos ou exatamente 64 bits, o padding feito
            // acima nao foi o suficiente, logo adicionamos mais 8 bytes de uns
            last = (i+1)%16;
            if (last <= 8) {
                filebits[++f] = 0xffffffffffffffff;
            }
            // termina de processar o arquivo
            continue;
        }

        // terminou um X, adiciona no vetor
        if (!((i+1)%8) && i != 0) {
            filebits[f] = X;
            // printf("i: %d\n", i);
            // printBits(sizeof(X), &X);
            f++;
            X = 0;
        }
    }

    printf("File len: %ld\n", filelen);



    // criptografa e escreve no arquivo de saída
    FILE *write;
    write = fopen("out.bin","wb");
    uint64_t Xa, Xb;
    uint64_t *cript = NULL;
    for (i = 0; i < f; i += 2) {
        Xa = filebits[i];
        Xb = filebits[i+1];

        cript = K128_encript(subkeys, Xa, Xb);
        Xa = cript[0];
        Xb = cript[1];

        printf("%lx \t%lx\n", Xa, Xb);

        fwrite(&Xa,sizeof(Xa), 1, write);
        fwrite(&Xb,sizeof(Xb), 1, write);
    }


    free(buffer);
    free(filebits);
    free(cript);
    fclose(file);
    fclose(write);
}


void decrypt_file(char *filename, uint64_t *subkeys) {
    FILE *file;
    char *buffer;
    uint64_t *filebits;
    long filelen;
    int i, f;

    file = fopen(filename, "rb");
    fseek(file, 0, SEEK_END);
    filelen = ftell(file);
    rewind(file);
    buffer = (char *)malloc((filelen+1)*sizeof(char));
    filebits = malloc((filelen+1)*sizeof(uint64_t));


    for(i = 0; i < filelen; i++) {
        if (fread(buffer + i, 1, 1, file));
    }

    // monta o vetor de bits do arquivo filebits
    // ASSUME QUE O PADDING FOI FEITO CORRETAMENTE NA CRIPTOGRAFIA
    uint64_t X = 0;
    for(i = 0, f = 0; i < filelen; i++) {
        // monta X
        X <<= 8;
        X |= buffer[i];
        // printBits(sizeof(buffer[i]), &buffer[i]);

        // terminou um X, adiciona no vetor
        if (!((i+1)%8) && i != 0) {
            filebits[f] = X;
            f++;
            X = 0;
        }
    }

    printf("File len: %ld\n", filelen);
    // decriptografa e escreve no arquivo de saída
    FILE *write;
    write = fopen("dout","wb");
    uint64_t Xa, Xb;
    uint64_t *decript = NULL;
    for (i = 0; i < f; i += 2) {
        Xa = filebits[i];
        Xb = filebits[i+1];

        decript = K128_decript(subkeys, Xa, Xb);
        Xa = decript[0];
        Xb = decript[1];

        // printf("%lx \t%lx\n", Xa, Xb);

        // converte os 8 bytes decriptografados pra 8 chars e escreve
        uint8_t a[8], b[8];
        for (int j = 7; j >= 0; j--) {
            a[j] = Xa%256;
            b[j] = Xb%256;
            Xa >>= 8;
            Xb >>= 8;
        }
        char c_a, c_b;
        for (int j = 0; j < 8; j++) {
            printf("%c", a[j]);
            c_a = a[j];
            fwrite(&c_a,sizeof(c_a), 1, write);
        }
        for (int j = 0; j < 8; j++) {
            c_b = b[j];
            fwrite(&c_b,sizeof(c_b), 1, write);
        }
    }

    free(buffer);
    free(filebits);
    free(decript);
    fclose(file);
    fclose(write);
}

int main(int argc, char **argv) {
    char *A, *K;
    uint64_t *subkeys;

    EXP = malloc(257*sizeof(uint8_t));
    LOG = malloc(257*sizeof(uint8_t));
    gen_exp_log();
    // printf("bolinha\n");
    // uint64_t ble = bolinha(0x9e3779b97f4a7151, 0x1324819741ff2312);
    // printf("fim do bolinha\n");

    A = malloc(A_SIZE*sizeof(char));


    strcpy(A, "a1b2c3d4e5f6g7h8");
    strcpy(A, "00000000000000aa");
    if (!validade_entry(A)) {
        printf("Invalid entry!\n");
        free(A);
        return 0;
    }
    K = gen_K(A);
    subkeys = gen_subkeys(R, K);
    ////////////////////////////// file stuff //////////////////////////////////
    encrypt_file("carradio.txt", subkeys);
    decrypt_file("out.bin", subkeys);
    ////////////////////////////////////////////////////////////////////////////

    printf("%d\n", validade_entry(A));

    uint64_t *cript = K128_encript(subkeys, 0x9e3779b97f4a7151, 0x1324819741ff2312);
    printf("\n\n%lx\n%lx\n\n", cript[0], cript[1]);
    uint64_t *decript = K128_decript(subkeys, cript[0], cript[1]);
    printf("\n\n%lx\n%lx\n\n", decript[0], decript[1]);

    free(subkeys);
    free(EXP);
    free(LOG);
    free(cript);
    free(decript);
    free(A);
    free(K);

    return 0;
}
