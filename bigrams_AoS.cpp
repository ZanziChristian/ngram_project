//
// Created by Christian Zanzi on 19/11/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>

#define MAX_WORD_LEN 50
#define MAX_BIGRAMS 52000

// Struttura per memorizzare un bigramma
typedef struct {
    char first[MAX_WORD_LEN];
    char second[MAX_WORD_LEN];
    int count;
} Bigram;

typedef struct {
    char first;
    char second;
    int count;
} CharacterBigram;

// Funzione per trovare un bigramma nell'array
int find_bigram(Bigram bigrams[], int size, char *first, char *second) {
    for (int i = 0; i < size; i++) {
        if (strcmp(bigrams[i].first, first) == 0 && strcmp(bigrams[i].second, second) == 0) {
            return i;
        }
    }
    return -1;
}

// Funzione per aggiungere un bigramma all'array
void add_bigram(Bigram bigrams[], int *size, char *first, char *second) {
    int index = find_bigram(bigrams, *size, first, second);
    if (index != -1) {
        bigrams[index].count++;
    } else {
        if (*size < MAX_BIGRAMS) {
            strcpy(bigrams[*size].first, first);
            strcpy(bigrams[*size].second, second);
            bigrams[*size].count = 1;
            (*size)++;
        }
    }
}

int find_character_bigram(CharacterBigram bigrams[], int size, char first, char second) {
    for (int i = 0; i < size; i++) {
        if (bigrams[i].first == first && bigrams[i].second == second) {
            return i;
        }
    }
    return -1;
}

void add_character_bigram(CharacterBigram bigrams[], int *size, char first, char second) {
    int index = find_character_bigram(bigrams, *size, first, second);
    if (index != -1) {
        bigrams[index].count++;
    } else {
        if (*size < MAX_BIGRAMS) {
            bigrams[*size].first = first;
            bigrams[*size].second = second;
            bigrams[*size].count = 1;
            (*size)++;
        }
    }
}

// Funzione per normalizzare una parola (rimuove la punteggiatura e converte in minuscolo)
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
    text[bytes_read] = '\0';  // Assicuriamoci che la stringa sia terminata

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
        }
        else {
        *dst++ = ' ';
        }
        src++;
    }
    *dst = '\0';


    char *words[300000];
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
    free(token);

    Bigram bigrams[MAX_BIGRAMS];
    CharacterBigram cbigrams[MAX_BIGRAMS];
    int bigram_count = 0;
    int cbigram_count = 0;

    for (int i = 0; i < word_count - 1; i++) {
        add_bigram(bigrams, &bigram_count, words[i], words[i + 1]);
        for (int j = 0; j < strlen(words[i]) - 1; j++) {
            add_character_bigram(cbigrams, &cbigram_count, words[i][j], words[i][j + 1]);
        }
    }
    double end_time = omp_get_wtime();

    printf("Bigrammi trovati:\n");
    for (int i = 0; i < bigram_count; i++) {
        printf("%s %s: %d\n", bigrams[i].first, bigrams[i].second, bigrams[i].count);
    }
    for (int i = 0; i < cbigram_count; i++) {
        printf("%c %c: %d\n", cbigrams[i].first, cbigrams[i].second, cbigrams[i].count);
    }

    printf("Bigram size %d", bigram_count);
    printf("Bigram size %d", cbigram_count);
    printf("Tempo di esecuzione: %.6f secondi\n", end_time - start_time);
    return 0;
}