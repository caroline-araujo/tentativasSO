/*
** UC: 21111 - Sistemas Operativos
** e-fólio B 2022-23 (mtts.c)
**
** Aluno: 2102995 - Caroline Araujo
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <stdatomic.h>

#define ALPHABET_SIZE 26

struct worker_args {
    char *buf;
    size_t len;
    int task_id;
    size_t total_len;
    int *freq; // vetor de contagem local à thread
};

int freq[ALPHABET_SIZE] = {0};
size_t total_len;

void *worker(void *arg) {
    struct worker_args *wargs = (struct worker_args *)arg;
    char *buf = wargs->buf;
    size_t len = wargs->len;
    int *task_freq = calloc(ALPHABET_SIZE, sizeof(int)); // vetor de contagem local à thread

    for (size_t i = 0; i < len; i++) {
        char c = tolower(buf[i]);
        if (isalpha(c)) {
            task_freq[c - 'a']++;
        }
    }

    // adicionar as contagens locais ao vetor global
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        atomic_fetch_add(&freq[i], task_freq[i]);
    }

    double percent = (double)len / wargs->total_len * 100;
    printf("T%d= %7.3f%%\n", wargs->task_id, percent);

    free(task_freq);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s nt nc ficheiro\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int nt = atoi(argv[1]);
    int nc = atoi(argv[2]);
    char *filename = argv[3];

    int freqs[nt][ALPHABET_SIZE]; // vetor de frequências por thread
    pthread_t threads[nt];

    if (nt < 1 || nc < 1) {
        fprintf(stderr, "nt e nc devem ser maiores ou iguais a 1.\n");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // obter tamanho do arquivo
    fseek(file, 0, SEEK_END);
    size_t filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // ler arquivo para o buffer
    char *buf = malloc(filesize);
    if (!buf) {
    perror("malloc");
    exit(EXIT_FAILURE);
    }
    size_t bytes_read = fread(buf, 1, filesize, file);
    if (bytes_read != filesize) {
    perror("fread");
    exit(EXIT_FAILURE);
    }

    total_len = filesize; // variável global

    printf("Análise de letras do alfabeto com %d tarefas e blocos de %d caracteres\n", nt, nc);
    printf("Ficheiro %s (%ld bytes)\n", filename, filesize);
    printf("Relatório de tarefas:\n");

    // criar threads trabalhadoras
    size_t block_size = nc;
    for (int i = 0; i < nt; i++) {
    struct worker_args *wargs = malloc(sizeof(struct worker_args));
    wargs->buf = buf + i * block_size;
    wargs->len = i == nt - 1 ? filesize - i * block_size : block_size;
    wargs->freq = freqs[i];
    wargs->task_id = i;
    pthread_create(&threads[i], NULL, (void *(*)(void *))worker, wargs);
    }

    // esperar pelas threads trabalhadoras terminarem
    for (int i = 0; i < nt; i++) {
    pthread_join(threads[i], NULL);
    }

    // somar as frequências de todas as threads
    int freq[ALPHABET_SIZE] = {0};
    for (int i = 0; i < nt; i++) {
    for (int j = 0; j < 26; j++) {
    freq[j] += freqs[i][j];
    }
    }

    // calcular as percentagens de frequência
    size_t total = 0;
    for (int i = 0; i < 26; i++) {
    total += freq[i];
    }
    printf("Relatório de carateres:\n");
    for (int i = 0; i < 26; i++) {
    double percent = (double)freq[i] / total * 100;
    printf("%c: %.2f%%\n", 'a' + i, percent);
    }

    // calcular a frequência total de caracteres (excluindo espaços em branco)
    int total_chars = 0;
    for (int i = 0; i < filesize; i++) {
    if (!isspace(buf[i])) {
    total_chars++;
    }
    }

    // imprimir tabela final com a frequência de ocorrência das letras (excluindo espaços em branco)
    printf("\nRelatório de caracteres (excluindo espaços em branco):\n");
    for (int i = 0; i < 26; i++) {
    double percent = (double)freq[i] / total_chars * 100;
    printf("%c: %.2f%%\n", 'a' + i, percent);
    }

    // limpeza
    free(buf);
    fclose(file);
    return 0;
}