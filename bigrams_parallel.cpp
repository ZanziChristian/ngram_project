//
// Created by Christian Zanzi on 21/11/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>

#define MAX_WORD_LEN 50
#define MAX_BIGRAMS 230000
#define MAX_WORDS 250000
#define MAX_BIGRAMS_LOCAL 25000

// --- Strutture globali (SoA) per i bigrammi di parole ---
char bigram_first[MAX_BIGRAMS][MAX_WORD_LEN];
char bigram_second[MAX_BIGRAMS][MAX_WORD_LEN];
int  bigram_count[MAX_BIGRAMS];
int  global_bigram_size = 0;

// Struttura per memorizzare i risultati locali di ciascun worker
typedef struct {
    // Bigrammi di parole (SoA)
    char local_bigram_first[MAX_BIGRAMS_LOCAL][MAX_WORD_LEN];
    char local_bigram_second[MAX_BIGRAMS_LOCAL][MAX_WORD_LEN];
    int local_bigram_count[MAX_BIGRAMS_LOCAL];
    int local_bigram_size;
    // Bigrammi di caratteri (SoA)
    char local_char_bigram_first[MAX_BIGRAMS_LOCAL];
    char local_char_bigram_second[MAX_BIGRAMS_LOCAL];
    int local_char_bigram_count[MAX_BIGRAMS_LOCAL];
    int local_char_bigram_size;
} LocalResults;

// --- Strutture globali (SoA) per i bigrammi di caratteri ---
char char_bigram_first[MAX_BIGRAMS];
char char_bigram_second[MAX_BIGRAMS];
int  char_bigram_count[MAX_BIGRAMS];
int  global_char_bigram_size = 0;

// Funzione per normalizzare una parola (minuscolo e tronca alla prima punteggiatura/spazio)
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

// Funzione per leggere l'intero contenuto di un file
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
        perror("Errore nell'allocazione della memoria");
        fclose(file);
        return NULL;
    }
    size_t bytes_read = fread(text, 1, file_length, file);
    text[bytes_read] = '\0';
    fclose(file);
    return text;
}

// Inizializza una struttura LocalResults
void init_local_results(LocalResults *lr) {
    lr->local_bigram_size = 0;
    lr->local_char_bigram_size = 0;
    for (int i = 0; i < MAX_BIGRAMS_LOCAL; i++) {
        lr->local_bigram_count[i] = 0;
        lr->local_char_bigram_count[i] = 0;
    }
}

// Cerca un bigramma di parole nei risultati locali; restituisce l'indice oppure -1 se non trovato
int local_find_bigram(LocalResults *lr, const char *first, const char *second) {
    for (int i = 0; i < lr->local_bigram_size; i++) {
        if (strcmp(lr->local_bigram_first[i], first) == 0 &&
            strcmp(lr->local_bigram_second[i], second) == 0) {
            return i;
        }
    }
    return -1;
}

// Aggiunge (o incrementa) un bigramma di parole nei risultati locali
void local_add_bigram(LocalResults *lr, const char *first, const char *second) {
    int index = local_find_bigram(lr, first, second);
    if (index != -1) {
        lr->local_bigram_count[index]++;
    } else {
        if (lr->local_bigram_size < MAX_BIGRAMS_LOCAL) {
            strcpy(lr->local_bigram_first[lr->local_bigram_size], first);
            strcpy(lr->local_bigram_second[lr->local_bigram_size], second);
            lr->local_bigram_count[lr->local_bigram_size] = 1;
            lr->local_bigram_size++;
        }
    }
}

// Cerca un bigramma di caratteri nei risultati locali; restituisce l'indice oppure -1 se non trovato
int local_find_char_bigram(LocalResults *lr, char first, char second) {
    for (int i = 0; i < lr->local_char_bigram_size; i++) {
        if (lr->local_char_bigram_first[i] == first &&
            lr->local_char_bigram_second[i] == second) {
            return i;
        }
    }
    return -1;
}

// Aggiunge (o incrementa) un bigramma di caratteri nei risultati locali
void local_add_char_bigram(LocalResults *lr, char first, char second) {
    int index = local_find_char_bigram(lr, first, second);
    if (index != -1) {
        lr->local_char_bigram_count[index]++;
    } else {
        if (lr->local_char_bigram_size < MAX_BIGRAMS_LOCAL) {
            lr->local_char_bigram_first[lr->local_char_bigram_size] = first;
            lr->local_char_bigram_second[lr->local_char_bigram_size] = second;
            lr->local_char_bigram_count[lr->local_char_bigram_size] = 1;
            lr->local_char_bigram_size++;
        }
    }
}

// Funzione per il merge dei risultati locali nel set globale (eseguito dal master dopo che tutti i worker hanno finito)
void merge_local_results(LocalResults *lr) {
    // Merge dei bigrammi di parole
    for (int i = 0; i < lr->local_bigram_size; i++) {
        int found = -1;
        for (int j = 0; j < global_bigram_size; j++) {
            if (strcmp(bigram_first[j], lr->local_bigram_first[i]) == 0 &&
                strcmp(bigram_second[j], lr->local_bigram_second[i]) == 0) {
                found = j;
                break;
            }
        }
        if (found != -1) {
            bigram_count[found] += lr->local_bigram_count[i];
        } else {
            if (global_bigram_size < MAX_BIGRAMS) {
                strcpy(bigram_first[global_bigram_size], lr->local_bigram_first[i]);
                strcpy(bigram_second[global_bigram_size], lr->local_bigram_second[i]);
                bigram_count[global_bigram_size] = lr->local_bigram_count[i];
                global_bigram_size++;
            }
        }
    }
    // Merge dei bigrammi di caratteri
    for (int i = 0; i < lr->local_char_bigram_size; i++) {
        int found = -1;
        for (int j = 0; j < global_char_bigram_size; j++) {
            if (char_bigram_first[j] == lr->local_char_bigram_first[i] &&
                char_bigram_second[j] == lr->local_char_bigram_second[i]) {
                found = j;
                break;
            }
        }
        if (found != -1) {
            char_bigram_count[found] += lr->local_char_bigram_count[i];
        } else {
            if (global_char_bigram_size < MAX_BIGRAMS) {
                char_bigram_first[global_char_bigram_size] = lr->local_char_bigram_first[i];
                char_bigram_second[global_char_bigram_size] = lr->local_char_bigram_second[i];
                char_bigram_count[global_char_bigram_size] = lr->local_char_bigram_count[i];
                global_char_bigram_size++;
            }
        }
    }
}

int main() {
    // Lettura e preprocessing del file
    char *text = read_file("mobydick.txt");
    if (!text)
        return 1;

    // Sostituisce caratteri non alfanumerici (eccetto lo spazio) con lo spazio
    char *src = text, *dst = text;
    while (*src) {
        if (*src == ' ' || isalnum(*src))
            *dst++ = *src;
        else
            *dst++ = ' ';
        src++;
    }
    *dst = '\0';

    // Tokenizzazione: suddivide il testo in parole
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

    double start_time = omp_get_wtime();

    // Determina il numero di thread e calcola la dimensione omogenea del chunk
    int num_threads = omp_get_max_threads();
    int chunk_size = word_count / num_threads;
    if (word_count % num_threads != 0)
        chunk_size++; // Assicura che tutti i dati siano coperti

    // Allocazione di un array di LocalResults, uno per ogni thread
    LocalResults *local_results = (LocalResults *)malloc(num_threads * sizeof(LocalResults));
    for (int i = 0; i < num_threads; i++) {
        init_local_results(&local_results[i]);
    }

    // Fase parallela: ciascun worker elabora il proprio chunk in base all'indice del thread
    #pragma omp parallel num_threads(num_threads)
    {
        int tid = omp_get_thread_num();
        int start_idx = tid * chunk_size;
        int end_idx = start_idx + chunk_size;
        if (end_idx > word_count)
            end_idx = word_count;

        // Elaborazione del chunk assegnato
        // Per i bigrammi di parole, si considerano coppie (words[i], words[i+1])
        for (int i = start_idx; i < end_idx - 1; i++) {
            local_add_bigram(&local_results[tid], words[i], words[i+1]);
            // Per la parola corrente, elaborazione dei bigrammi di caratteri
            int len = strlen(words[i]);
            for (int k = 0; k < len - 1; k++) {
                local_add_char_bigram(&local_results[tid], words[i][k], words[i][k+1]);
            }
        }
        // Per l'ultimo elemento del chunk (se esiste) elaboriamo solo i bigrammi di caratteri
        if (end_idx - 1 < word_count) {
            int len = strlen(words[end_idx - 1]);
            for (int k = 0; k < len - 1; k++) {
                local_add_char_bigram(&local_results[tid], words[end_idx - 1][k], words[end_idx - 1][k+1]);
            }
        }
    } // Fine della fase parallela: tutti i worker hanno terminato la loro elaborazione

    // Il thread master effettua il merge dei risultati locali nelle strutture globali
    for (int i = 0; i < num_threads; i++) {
        merge_local_results(&local_results[i]);
    }

    double end_time = omp_get_wtime();

    // Stampa dei risultati: Bigrammi di parole
    printf("Word Bigrams:\n");
    for (int i = 0; i < 100; i++) {
        printf("%s %s: %d\n", bigram_first[i], bigram_second[i], bigram_count[i]);
    }

    // Stampa dei risultati: Bigrammi di caratteri
    printf("\nCharacter Bigrams:\n");
    for (int i = 0; i < global_char_bigram_size; i++) {
        printf("%c %c: %d\n", char_bigram_first[i], char_bigram_second[i], char_bigram_count[i]);
    }

    printf("\nBigram size: %d\n",global_bigram_size);
    printf("\nBigram size: %d\n",global_char_bigram_size);
    printf("\nExecution Time: %.6f seconds\n", end_time - start_time);

    free(local_results);
    free(text);
    return 0;
}