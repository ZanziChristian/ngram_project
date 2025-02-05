//
// Created by Christian Zanzi on 19/11/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>

#define MAX_WORD_LEN 50
#define MAX_BIGRAMS 230000
#define MAX_WORDS 300000

char bigram_first[MAX_BIGRAMS][MAX_WORD_LEN];
char bigram_second[MAX_BIGRAMS][MAX_WORD_LEN];
int bigram_count[MAX_BIGRAMS];

char char_bigram_first[MAX_BIGRAMS];
char char_bigram_second[MAX_BIGRAMS];
int char_bigram_count[MAX_BIGRAMS];

int find_bigram(int size, char *first, char *second) {
    for (int i = 0; i < size; i++) {
        if (strcmp(bigram_first[i], first) == 0 && strcmp(bigram_second[i], second) == 0) {
            return i;
        }
    }
    return -1;
}

void add_bigram(int *size, char *first, char *second) {
    int index = find_bigram(*size, first, second);
    if (index != -1) {
        bigram_count[index]++;
    } else {
        if (*size < MAX_BIGRAMS) {
            strcpy(bigram_first[*size], first);
            strcpy(bigram_second[*size], second);
            bigram_count[*size] = 1;
            (*size)++;
        }
    }
}

int find_character_bigram(int size, char first, char second) {
    for (int i = 0; i < size; i++) {
        if (char_bigram_first[i] == first && char_bigram_second[i] == second) {
            return i;
        }
    }
    return -1;
}

void add_character_bigram(int *size, char first, char second) {
    int index = find_character_bigram(*size, first, second);
    if (index != -1) {
        char_bigram_count[index]++;
    } else {
        if (*size < MAX_BIGRAMS) {
            char_bigram_first[*size] = first;
            char_bigram_second[*size] = second;
            char_bigram_count[*size] = 1;
            (*size)++;
        }
    }
}

void normalize_word(char *word) {
    int len = strlen(word);
    for (int i = 0; i < len; i++) {
        if (ispunct(word[i]) || isspace(word[i])) {
            word[i] = '\0';
            break;
        }
        word[i] = tolower(word[i]);
    }
}

char *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Errore nell'apertura del file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_length = ftell(file);
    rewind(file);

    char *text = (char *)malloc((file_length + 1) * sizeof(char));
    if (!text) {
        perror("Errore nell'allocazione di memoria");
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(text, 1, file_length + 1, file);
    text[bytes_read] = '\0';

    fclose(file);
    return text;
}

int main() {
    char *text = read_file("mobydick.txt");
    if (!text) {
        return 1;
    }
    double start_time = omp_get_wtime();

    char *src = text, *dst = text;
    while (*src) {
        if (*src == ' ' || isalnum(*src)) {
            *dst++ = *src;
        } else {
            *dst++ = ' ';
        }
        src++;
    }
    *dst = '\0';

    char *words[MAX_WORDS];
    int word_count = 0;
    char *token = strtok(text, " ");

    while (token) {
        normalize_word(token);
        if (strlen(token) > 0) {
            words[word_count++] = token;
        }
        token = strtok(NULL, " ");
    }
    free(text);

    int bigram_size = 0;
    int char_bigram_size = 0;

    for (int i = 0; i < word_count - 1; i++) {
        add_bigram(&bigram_size, words[i], words[i + 1]);
        for (int j = 0; j < strlen(words[i]) - 1; j++) {
            add_character_bigram(&char_bigram_size, words[i][j], words[i][j + 1]);
        }
    }
    double end_time = omp_get_wtime();

    printf("Word Bigrams:\n");
    for (int i = 0; i < bigram_size; i++) {
        if (bigram_count[i] > 200) {
            printf("%s %s: %d\n", bigram_first[i], bigram_second[i], bigram_count[i]);
        }
    }
    printf("Character Bigrams:\n");
    for (int i = 0; i < char_bigram_size; i++) {
        printf("%c %c: %d\n", char_bigram_first[i], char_bigram_second[i], char_bigram_count[i]);
    }

    printf("\nBigram size: %d\n",bigram_size);
    printf("\nBigram size: %d\n",char_bigram_size);
    printf("Execution Time: %.6f seconds\n", end_time - start_time);
    return 0;
}