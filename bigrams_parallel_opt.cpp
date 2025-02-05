//
// Created by Christian Zanzi on 03/02/25.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>


#define MAX_WORD_LEN 50
#define MAX_BIGRAMS 230000
#define MAX_CHAR_BIGRAMS 3200000
#define MAX_WORDS 250000
#define MAX_BIGRAMS_LOCAL 25000
#define MAX_CHAR_BIGRAM_LOCAL 700000


char bigrams[MAX_BIGRAMS][MAX_WORD_LEN];
int  bigrams_count[MAX_BIGRAMS];
int  bigrams_size = 0;

char char_bigrams[MAX_CHAR_BIGRAMS][4];
int  char_bigrams_count[MAX_CHAR_BIGRAMS];
int  char_bigrams_size = 0;

int cmpstr(void const *a, void const *b) {
    char const *aa = (char const *)a;
    char const *bb = (char const *)b;

    return strcmp(aa, bb);
}

char *clean_text(char *text) {
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
    return text;
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
        perror("Errore nell'allocazione della memoria");
        fclose(file);
        return NULL;
    }
    size_t bytes_read = fread(text, 1, file_length, file);
    text[bytes_read] = '\0';
    fclose(file);
    return text;
}


int main() {
    char *text = read_file("mobydick.txt");
    if (!text)
        return 1;

    double start_time = omp_get_wtime();
    text = clean_text(text);

    // fase di split delle parole
    char *words[MAX_WORDS];
    int word_count = 0;
    char *token = strtok(text, " ");
    while (token) {
        normalize_word(token);
        if (strlen(token) > 0) {
            words[word_count] = token;
            word_count++;
        }
        token = strtok(NULL, " ");
    }

    int num_threads = omp_get_max_threads();
    int chunk_size = word_count / num_threads;
    if (word_count % num_threads) {
        chunk_size++;
    }

    // fase di map
    #pragma omp parallel num_threads(num_threads)
    {
        int tid = omp_get_thread_num();
        int start_idx = tid * chunk_size;
        int end_idx = start_idx + chunk_size;
        if (end_idx > word_count)
            end_idx = word_count - 1;

        char local_bigrams[27823][MAX_WORD_LEN];
        int local_bigrams_count[chunk_size];
        int local_bigrams_size = 0;

        for (int i = start_idx; i < end_idx; i++) {
            size_t len1 = strlen(words[i]);
            size_t len2 = strlen(words[i+1]);

            char *bigram = (char*)malloc((len1 + len2 + 2) * sizeof(char));
            snprintf(bigram, len1 + 1 + len2 + 1, "%s %s", words[i], words[i+1]);
            strcpy(local_bigrams[i - (tid * chunk_size)], bigram);
            local_bigrams_count[local_bigrams_size] = 1;
            local_bigrams_size++;
            free(bigram);


        }
        qsort(local_bigrams, local_bigrams_size, MAX_WORD_LEN, cmpstr);

        #pragma omp barrier
        #pragma omp critical
        {
            // Inserisci i bigrammi locali nel vettore globale
            for (int j = 0; j < local_bigrams_size; j++) {
                strcpy(bigrams[bigrams_size], local_bigrams[j]);
                bigrams_count[bigrams_size] = local_bigrams_count[j];
                bigrams_size++;
            }


        }
    }
    qsort(bigrams, bigrams_size, MAX_WORD_LEN, cmpstr);

    // Scorri e aggrega i duplicati
    int i = 0;
    int total_bigrams = 0;
    while (i < bigrams_size) {
        int count = bigrams_count[i];
        int j = i + 1;
        while (j < bigrams_size && strcmp(bigrams[i], bigrams[j]) == 0) {
            count += bigrams_count[j];
            j++;
        }
        if (count > 200)
        {
            printf("%s : %d\n", bigrams[i], count);
        }
        strcpy(bigrams[total_bigrams], bigrams[i]);
        bigrams_count[total_bigrams] = count;
        total_bigrams += 1;
        i = j;
    }
    printf("%d\n", total_bigrams);


    #pragma omp parallel num_threads(num_threads)
    {
        int tid = omp_get_thread_num();
        int start = tid * chunk_size;
        int end = start + chunk_size;
        if (end > word_count)
            end = word_count - 1;

        char local_char_bigrams[MAX_CHAR_BIGRAM_LOCAL][4]; // 2 caratteri + terminatore
        int local_char_size = 0;

        for (int i = start; i < end; i++) {
            for (int j = 0; j < strlen(words[i]) - 1; j++) {
                char *bigram = (char*)malloc(4 * sizeof(char));
                snprintf(bigram, 4, "%c %c", words[i][j], words[i][j+1]);
                strcpy(local_char_bigrams[local_char_size], bigram);
                local_char_size++;
            }
        }

        qsort(local_char_bigrams, local_char_size, 4, cmpstr);

        #pragma omp critical
        {
            for (int k = 0; k < local_char_size; k++) {
                strcpy(char_bigrams[char_bigrams_size], local_char_bigrams[k]);
                char_bigrams_count[char_bigrams_size] = 1;
                char_bigrams_size++;
            }
        }
    }
    qsort(char_bigrams, char_bigrams_size, 4, cmpstr);

    // Reduce per i bigrammi di caratteri: aggrega le occorrenze dei duplicati
    i = 0;
    int total_char_bigrams = 0;
    printf("\nBigrammi di caratteri:\n");
    while (i < char_bigrams_size) {
        int count = char_bigrams_count[i];
        int j = i + 1;
        while (j < char_bigrams_size && strcmp(char_bigrams[i], char_bigrams[j]) == 0) {
            count += char_bigrams_count[j];
            j++;
        }
        printf("%s : %d\n", char_bigrams[i], count);
        total_char_bigrams++;
        i = j;
    }
    double end_time = omp_get_wtime();
    printf("Totale bigrammi di caratteri: %d\n", total_char_bigrams);

    printf("Execution Time: %.6f seconds\n", end_time - start_time);
    free(text);
    return EXIT_SUCCESS;
}