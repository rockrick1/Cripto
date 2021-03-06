
#include "opes.h"

const unsigned A_SIZE = 128;
const unsigned MAX_FILE_NAME = 128;
const unsigned K_SIZE = 16;
const unsigned R = 12;

uint8_t *EXP;
uint8_t *LOG;


// Gera um K, dada uma chave A
char *gen_K(char *A) {
    char *K = malloc(16*sizeof(char) + 1);

    // Se A for mais curta que K_SIZE, concatena A com ela mesma até
    // ter esse tamanho
    for (int i = 0; i < K_SIZE; i++) {
        K[i] = A[i%strlen(A)];
    }
    K[K_SIZE] = '\0';
    return K;
}


// Gera o vetor de subchaves, conforme descrito no enunciado
uint64_t *gen_subkeys(int R, char *K) {
    uint64_t *L = malloc((4*R + 3)*sizeof(uint64_t));
    uint64_t *subkeys = malloc((4*R + 4)*sizeof(uint64_t));

    // converte K para L0 e L1
    for (int i = 0; i < K_SIZE/2; i++) {
        L[0] = L[0]<<8;
        L[1] = L[1]<<8;
        L[0] |= K[i];
        L[1] |= K[i+8];
    }

    // gera o resto do vetor L
    uint64_t num1 = 0x9e3779b97f4a7151;
    for (int j = 2; j <= 4*R + 2; j++) {
        L[j] = (L[j-1] + num1); // mod 2**64 aturomatico
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
        subkeys[i] = rotate_bit(subkeys[i], 3);
        A = subkeys[i];
        i++;

        // (b)
        L[j] += A + B;
        L[j] = rotate_bit(L[j], (A+B)%64);
        B = L[j];
        j++;
    }

    return subkeys;
}


// criptograma Xa e Xb usando as subchaves dadas, pelo método K128
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

// decriptograma XeFinal e XfFinal usando as subchaves dadas, pelo método K128
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


// Criptografa um arquivo inteiro, usando o K128_encript para cada bloco
// de 128 bits do arquivo, e escreve num arquivo de saida com nome dado
void encrypt_file(char *file_in_name, char *file_out_name, uint64_t *subkeys, int CBC) {
    FILE *file;
    uint8_t *buffer; // buffer para ler o arquivo
    uint64_t *filebits; // vetor para guardar os blocos de 64 bits
    long filelen; // tamanho do arquivo, em bytes
    unsigned long i, f;

    file = fopen(file_in_name, "rb");
    fseek(file, 0, SEEK_END);
    filelen = ftell(file);
    rewind(file);
    buffer = malloc((filelen+1)*sizeof(uint8_t));
    filebits = malloc((filelen+1)*sizeof(uint64_t));

    // le o arquivo de byte em byte, e guarda no buffer
    for(i = 0; i < filelen; i++) {
        if (fread(buffer + i, 1, 1, file));
    }

    // monta o vetor filebits de blocos de 8 bytes do arquivo lido
    uint64_t X = 0;
    for(i = 0, f = 0; i < filelen + 1; i++) {
        // monta um X
        X <<= 8;
        X |= buffer[i];

        // se esse for o ultimo byte, faz o padding
        if (i + 1 == filelen) {
            // tamanho do ultimo bloco de 128, em bytes
            int last = (i+1)%16;
            // se o ultimo bloco nao for de 128 bits, faz o padding com uns
            if (last != 0) {
                // se o bloco nao for de exatamente 64 bits, faz o padding
                // parcial
                if (last != 8) {
                    int aux = (i+1)%8;
                    int remaining = 8 - aux; // quantos bytes faltam no final
                    uint8_t uns = 0xff; // 1111 1111
                    // nesse ponto, X tem o fim do arquivo, entao deslocamos
                    // e adicionamos uns
                    for (int k = 0; k < remaining; k++) {
                        X <<= 8;
                        X |= uns;
                    }
                    filebits[f++] = X;
                    X = 0;
                }
                // se for de exatamente 64 bits, escreve o ultimo bloco
                // e faz o padding com 64 bits 1
                if (last == 8) filebits[f++] = X;
                if (last <= 8) filebits[f++] = 0xffffffffffffffff;

            }
            // se o bloco for de exatamente 128 bits, não faz padding
            else {
                filebits[f++] = X;
            }
            // escreve o tamanho real do arquivo original depois do padding,
            // ou do final, caso nao tenha sido feito padding
            filebits[f++] = filelen;
            filebits[f] = 0;
            // termina de processar o arquivo
            continue;
        }

        // terminou um X, adiciona no vetor
        if (!((i+1)%8) && i != 0) {
            filebits[f++] = X;
            X = 0;
        }
    }

    // criptografa e escreve no arquivo de saída
    FILE *write;
    write = fopen(file_out_name,"wb");

    // a esse ponto, f é o tamanho do arquivo, contando o padding, em blocos
    // de 64 bits
    uint64_t Xa, Xb;
    uint64_t *cript = NULL;
    for (i = 0; i < f; i += 2) {
        Xa = filebits[i];
        Xb = filebits[i+1];

        // faz o CBC
        if (CBC && i != 0) {
            Xa ^= cript[0];
            Xb ^= cript[1];
        }
        else if (CBC) {
            // VI é todo bits 1
            uint64_t initial_value = 0;
            initial_value = ~initial_value;
            Xa ^= initial_value;
            Xb ^= initial_value;
        }

        // criptografa e escreve no arquivo de saida
        cript = K128_encript(subkeys, Xa, Xb);
        Xa = cript[0];
        Xb = cript[1];

        fwrite(&Xa,sizeof(Xa), 1, write);
        fwrite(&Xb,sizeof(Xb), 1, write);
    }

    free(buffer);
    free(filebits);
    free(cript);
    fclose(file);
    fclose(write);
}


// Decriptografa um arquivo inteiro, usando o K128_decript para cada bloco
// de 128 bits do arquivo, e escreve num arquivo de saida com nome dado
void decrypt_file(char *file_in_name, char *file_out_name, uint64_t *subkeys, int CBC) {
    FILE *file;
    uint64_t *buffer;
    long filelen;
    unsigned long i, actual_filelen;

    file = fopen(file_in_name, "rb");
    fseek(file, 0, SEEK_END);
    filelen = ftell(file);
    rewind(file);
    buffer = malloc(((filelen/8)+1)*sizeof(uint64_t));

    // le o arquivo de 64 em 64 bits, e guarda num buffer
    for(i = 0; i < filelen/8; i++) {
        if (fread(buffer + i, sizeof(uint64_t), 1, file));
    }

    // decriptografa e escreve no arquivo de saída
    FILE *write;
    write = fopen(file_out_name,"wb");
    uint64_t Xa, Xb;
    uint64_t *decript = NULL;

    // descobre o tamanho exato do arquivo original, que escrevemos no final do
    // arquivo criptografado
    decript = K128_decript(subkeys, buffer[(filelen/8)-2], buffer[(filelen/8)-1]);
    if (CBC) {
        decript[0] ^= buffer[(filelen/8)-4];
    }
    actual_filelen = decript[0];

    // sempre que escrevermos um byte, incrementaremos esse aux, para
    // escrevermos 'actual_filelen' quantidade de bytes
    unsigned long aux = 0;
    for (i = 0; i < filelen/8 && aux < actual_filelen; i += 2) {
        Xa = buffer[i];
        Xb = buffer[i+1];

        decript = K128_decript(subkeys, Xa, Xb);
        Xa = decript[0];
        Xb = decript[1];

        // faz o CBC reverso
        if (CBC && i != 0) {
            Xa ^= buffer[i-2];
            Xb ^= buffer[i-1];
        }
        else if (CBC) {
            // VI é todo bits 1
            uint64_t initial_value = 0;
            initial_value = ~initial_value;
            Xa ^= initial_value;
            Xb ^= initial_value;
        }

        // converte os 8 bytes decriptografados pra 8 bytes separados e escreve
        // cada um, como chars
        uint8_t a[8], b[8];
        for (int j = 7; j >= 0; j--) {
            a[j] = Xa%256;
            b[j] = Xb%256;
            Xa >>= 8;
            Xb >>= 8;
        }

        // não deixamos exceder actual_filelen quantidade de bytes no arquivo
        char c_a, c_b;
        for (int j = 0; j < 8 && aux < actual_filelen; j++) {
            c_a = a[j];
            fwrite(&c_a, 1, 1, write);
            aux++;
        }
        for (int j = 0; j < 8 && aux < actual_filelen; j++) {
            c_b = b[j];
            fwrite(&c_b, 1, 1, write);
            aux++;
        }
    }

    free(buffer);
    free(decript);
    fclose(file);
    fclose(write);
}


// escreve brancos sobre cada char do arquivo, e depois deleta
void delete_file(char *filename) {
    FILE *file;
    long filelen;

    file = fopen(filename, "w+");
    fseek(file, 0, SEEK_END);
    filelen = ftell(file);
    rewind(file);

    char buf[2];
    sprintf(buf, " ");
    for (int i = 0; i < filelen; i++)
        fwrite(buf, strlen(buf), 1, file);
    fclose(file);
    remove(filename); // long live the king
}


// calcula a distancia de hamming dos bits de A e B
int hamming(uint64_t A, uint64_t B) {
    int h = 0;
    uint64_t C = A ^ B;
    while (C) {
        h += C & 1;
        C >>= 1;
    }
    return h;
}


// calcula a entropia pelo modo mode (podendo ser 1 ou 2)
void entropy(char *file_in_name, uint64_t *subkeys, int mode) {
    FILE *file;
    uint8_t *buffer;
    uint64_t *fbits;
    uint64_t *fbitsC;
    uint64_t *fbitsAlter;
    uint64_t *fbitsAlterC;
    short CBC = 1;
    long filelen;
    unsigned long i, j, f;

    file = fopen(file_in_name, "rb");
    fseek(file, 0, SEEK_END);
    filelen = ftell(file);
    rewind(file);
    buffer = malloc((filelen+1)*sizeof(uint8_t));

    fbits = malloc((filelen+1)*sizeof(uint64_t));
    fbitsC = malloc((filelen+1)*sizeof(uint64_t));
    fbitsAlter = malloc((filelen+1)*sizeof(uint64_t));
    fbitsAlterC = malloc((filelen+1)*sizeof(uint64_t));

    // le o arquivo de byte em byte, e guarda num buffer
    for(i = 0; i < filelen; i++) {
        if (fread(buffer + i, 1, 1, file));
    }

    // monta o vetor fbits de blocos de 8 bytes do arquivo lido
    uint64_t X = 0;
    for(i = 0, f = 0; i < filelen; i++) {
        // monta um X
        X <<= 8;
        X |= buffer[i];

        // terminou um X, adiciona no vetor
        if (!((i+1)%8) && i != 0) {
            fbits[f] = X;
            fbitsAlter[f] = X;
            f++;
            X = 0;
        }
    }

    // escreve o arquivo criptografado em fbitsC
    uint64_t Xa, Xb;
    uint64_t *cript = NULL;
    for (i = 0; i < f; i += 2) {
        Xa = fbitsAlter[i];
        Xb = fbitsAlter[i+1];
    }

    for (i = 0; i < f; i += 2) {
        Xa = fbits[i];
        Xb = fbits[i+1];

        if (CBC && i != 0) {
            Xa ^= cript[0];
            Xb ^= cript[1];
        }
        else if (CBC) {
            uint64_t initial_value = 0;
            initial_value = ~initial_value;
            Xa ^= initial_value;
            Xb ^= initial_value;
        }

        cript = K128_encript(subkeys, Xa, Xb);
        Xa = cript[0];
        Xb = cript[1];


        fbitsC[i] = Xa;
        fbitsC[i+1] = Xb;
    }

    long num_blocks = filelen/16;

    unsigned long max[num_blocks], min[num_blocks];
    float sum[num_blocks];
    for (int p = 0; p < num_blocks; p++) {
        max[p] = 0; min[p] = -1; sum[p] = 0.0;
    }
    // filelen*8 = numero de bits no arquivo
    for (j = 0; j < filelen*8; j++) {
        int H = 0;
        // altera o j-esimo bit do arquivo, e criptografa
        int block = j/64; // indice do bloco de 64 bits a ser alterado
        int big_block = j/128; // indice do inicio do bloco de 128 bits
        int idx_in_block = j%64; //indice do bit dentro do bloco
        uint64_t alter;

        if (mode == 1)
            alter = 0x8000000000000000; // apenas bit mais significativo=1
        else if (mode == 2)
            alter = 0x8080000000000000; // bit j e j+8 = 1
        // move o bit 1 para a posição da iteração atual
        for (int k = 0; k < idx_in_block; k++) {
            alter /= 2;
        }
        printBits(sizeof(alter), &alter);
        fbitsAlter[block] = fbits[block] ^ alter;

        // criptografa fbitsAlter
        for (i = 0; i < f; i += 2) {
            Xa = fbitsAlter[i];
            Xb = fbitsAlter[i+1];

            if (CBC && i != 0) {
                Xa ^= cript[0];
                Xb ^= cript[1];
            }
            else if (CBC) {
                uint64_t initial_value = 0;
                initial_value = ~initial_value;
                Xa ^= initial_value;
                Xb ^= initial_value;
            }

            cript = K128_encript(subkeys, Xa, Xb);
            Xa = cript[0];
            Xb = cript[1];

            fbitsAlterC[i] = Xa;
            fbitsAlterC[i+1] = Xb;
        }

        // calcula as distancias
        H += hamming(fbitsC[big_block*2], fbitsAlterC[big_block*2]);
        H += hamming(fbitsC[(big_block*2) + 1], fbitsAlterC[(big_block*2) + 1]);

        // atualiza os maximos, minimos e somas
        if (H > max[big_block]) max[big_block] = H;
        if (H < min[big_block]) min[big_block] = H;
        sum[big_block] += H;

        // retorna ao original, e passa para a proxima iteração
        fbitsAlter[block] = fbits[block];
    }

    // imprime tudo numa tabelinha
    printf("bloco\tmax\tmin\tmedia\n");
    for (int p = 0; p < num_blocks; p++) {
        printf("%d \t %lu \t %lu \t %.2f\n", (p+1)*128, max[p], min[p], sum[p]/128.0);
    }

    fclose(file);
    free(cript);
    free(buffer);
    free(fbits);
    free(fbitsC);
    free(fbitsAlter);
    free(fbitsAlterC);
}


int main(int argc, char **argv) {
    char *A, *K;
    uint64_t *subkeys;

    EXP = malloc(257*sizeof(uint8_t));
    LOG = malloc(257*sizeof(uint8_t));
    gen_exp_log();

    A = malloc(A_SIZE*sizeof(char));

    strcpy(A, "a1b2c3d4e5f6g7h8");
    if (!validade_entry(A)) {
        printf("Invalid password!\n");
        free(A);
        return 0;
    }
    K = gen_K(A);
    subkeys = gen_subkeys(R, K);

    ////////////////////////////// file stuff //////////////////////////////////
    // entropy("512.txt", subkeys, 1);
    encrypt_file("512.txt", "out.bin", subkeys, 1);
    // delete_file("carradio.txt");
    decrypt_file("out.bin", "512out.txt", subkeys, 1);

    encrypt_file("carradio.txt", "out.bin", subkeys, 1);
    // delete_file("carradio.txt");
    decrypt_file("out.bin", "carout.txt", subkeys, 1);

    encrypt_file("refuse.png", "out.bin", subkeys, 1);
    decrypt_file("out.bin", "out.png", subkeys, 1);
    //
    // // encrypt_file("just do it.mp3", "out.bin", subkeys, 0);
    // // decrypt_file("out.bin", "out.mp3", subkeys);

    /////////////////////////// terminal stuff /////////////////////////////////
    // short mode = 0, delete = 0;
    // if (!(strcmp(argv[1], "-c")))
    //     mode = 1;
    // else if (!(strcmp(argv[1], "-d")))
    //     mode = 2;
    // else if (!(strcmp(argv[1], "-1")))
    //     mode = 3;
    // else if (!(strcmp(argv[1], "-2")))
    //     mode = 4;
    //
    // EXP = malloc(257*sizeof(uint8_t));
    // LOG = malloc(257*sizeof(uint8_t));
    // gen_exp_log();
    //
    // char out_file[MAX_FILE_NAME];
    // char in_file[MAX_FILE_NAME];
    // char *A, *K = NULL;
    // A = malloc(A_SIZE*sizeof(char));
    // uint64_t *subkeys;
    //
    // for (int i = 1; i < argc; i++) {
    //     if (!(strcmp(argv[i], "-i")))
    //         strcpy(in_file, argv[++i]);
    //
    //     else if (!(strcmp(argv[i], "-o")))
    //         strcpy(out_file, argv[++i]);
    //
    //     else if (!(strcmp(argv[i], "-p")))
    //         strcpy(A, argv[++i]);
    //
    //     else if (!(strcmp(argv[i], "-a")))
    //         delete = 1;
    // }
    //
    // if (!validade_entry(A)) {
    //     printf("Invalid password!\n");
    //     free(A);
    //     free(EXP);
    //     free(LOG);
    //     return 0;
    // }
    //
    // K = gen_K(A);
    // subkeys = gen_subkeys(R, K);
    //
    // // vai ter arquivo de saida e depois a senha
    // if (mode == 1 || mode == 2) {
    //     if (mode == 1) {
    //         encrypt_file(in_file, out_file, subkeys, 1);
    //         if (delete) delete_file(in_file);
    //     }
    //
    //     else { // mode = 2
    //         decrypt_file(in_file, out_file, subkeys, 1);
    //     }
    //
    //
    // }
    // // vai ter só o arquivo de entrada e a senha
    // else {
    //     if (mode == 3) {
    //         entropy(in_file, subkeys, 1);
    //     }
    //
    //     else { // mode = 4
    //         entropy(in_file, subkeys, 2);
    //     }
    // }


    ////////////////////////////// frees ///////////////////////////////////////
    free(subkeys);
    free(EXP);
    free(LOG);
    free(A);
    free(K);
    ////////////////////////////////////////////////////////////////////////////

    return 0;
}
