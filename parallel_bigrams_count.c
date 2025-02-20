//
// Created by Christian Zanzi on 15/02/25.
//
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>

#define MAX_WORD_LEN 50
#define MAX_WORDS 900000
#define NUM_THREADS 8
#define NGRAM_SIZE 2

typedef struct {
    char **ngrams;
    int *frequencies;
    int size;
} Histogram;

Histogram* create_histogram(int size) {
    Histogram *new_histogram = malloc(sizeof(Histogram));
    new_histogram->ngrams = malloc(size * sizeof(char*) + size * MAX_WORD_LEN * sizeof(char));
    char *bigram = new_histogram->ngrams + size * sizeof(char*);
    for (int i = 0; i < size; i++) {
        new_histogram->ngrams[i] = bigram + i * MAX_WORD_LEN;
    }
    new_histogram->frequencies = malloc(size * sizeof(int));
    new_histogram->size = 0;
    return new_histogram;
}

void free_histogram(Histogram *histogram) {
    free(histogram->frequencies);
    free(histogram->ngrams);
    free(histogram);
}

// utility text preprocessing functions
char *clean_text(char *text) {
    char *src = text, *dst = text;
    while (*src) {
        if (*src == ' ' || isalnum(*src))
            *dst++ = *src;
        else
            *dst++ = ' ';
        src++;
    }
    *dst = '\0';
    return text;
}

void normalize_word(char *word) {
    size_t len = strlen(word);
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
        perror("Error opening file");
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    const long file_length = ftell(file);
    rewind(file);
    char *text = malloc((file_length + 1) * sizeof(char));
    if (!text) {
        perror("Error allocating memory");
        fclose(file);
        return NULL;
    }
    size_t bytes_read = fread(text, 1, file_length, file);
    text[bytes_read] = '\0';
    fclose(file);
    return text;
}

char **tokenize(char *text, int *word_count) {
    char **words = malloc(MAX_WORDS * sizeof(char*) + MAX_WORDS * MAX_WORD_LEN * sizeof(char));
    char *word = (char *)words + MAX_WORDS * sizeof(char*);
    for (int i = 0; i < MAX_WORDS; i++) {
        words[i] = word + i * MAX_WORD_LEN;
    }
    char *saveptr;
    char *token = strtok_r(text, " ", &saveptr);
    while (token) {
        normalize_word(token);
        if (strlen(token) > 0) {
            words[*word_count] = token;
            (*word_count)++;
        }
        token = strtok_r(NULL, " ", &saveptr);
    }
    *word_count--;
    return words;
}

char *create_ngram(char **words, int idx) {
    char *ngram = malloc(MAX_WORD_LEN * sizeof(char));
    for (int i = idx; i < idx + NGRAM_SIZE; i++) {
        strcat(ngram, words[i]);
        if (i != idx + NGRAM_SIZE - 1)
            strcat(ngram, " ");
    }
    return ngram;
}

char *create_char_ngram(char *word, int idx){
    char *ngram = malloc(NGRAM_SIZE * sizeof(char));
    strncpy(ngram, word + idx, NGRAM_SIZE);
    ngram[NGRAM_SIZE] = '\0';
    return ngram;
}

int compare_strings(void *ptr, const void *a, const void *b) {
    const char *str1 = *(const char * const *)a;
    const char *str2 = *(const char * const *)b;
    return strcmp(str1, str2);
}

int reduce(Histogram *global, const Histogram *local, int index) {
    for (int i = 0; i < local->size; i++) {
        if (index != 0 && strcmp(local->ngrams[i], global->ngrams[index - 1]) == 0) {
            global->frequencies[index - 1] += local->frequencies[i];
        } else {
            global->ngrams[index] = strdup(local->ngrams[i]);
            global->frequencies[index] = local->frequencies[i];
            index++;
        }
    }
    return index;
}

int main() {
    char *text = read_file("mobydick.txt");
    if (!text)
        return 1;

    double start_time = omp_get_wtime();

    text = clean_text(text);
    int word_count = 0;
    char **words = tokenize(text, &word_count);
    int character_count = 0;
    #pragma omp parallel for reduction(+:character_count)
    for (int i = 0; i < word_count; i++) {
        character_count += strlen(words[i]);
    }

    Histogram *word_ngram = create_histogram(word_count);
    Histogram *character_ngram = create_histogram(character_count);

    int word_chunk_size = word_count / NUM_THREADS;
    if (word_count % NUM_THREADS)
        word_chunk_size ++;
    int character_chunk_size = character_count / NUM_THREADS;
    if (character_count % NUM_THREADS)
        character_chunk_size ++;
    Histogram **local_ngrams = malloc(NUM_THREADS * sizeof(Histogram*));
    Histogram **local_c_ngrams = malloc(NUM_THREADS * sizeof(Histogram*));

    #pragma omp parallel num_threads(NUM_THREADS)
    {
        int tid = omp_get_thread_num();
        local_ngrams[tid] = create_histogram(word_chunk_size);
        local_c_ngrams[tid] = create_histogram(character_chunk_size);

        #pragma omp for
        for (int i = 0; i < word_count - NGRAM_SIZE; i++) {
            local_ngrams[tid]->ngrams[local_ngrams[tid]->size] = strdup(create_ngram(words, i));
            local_ngrams[tid]->frequencies[local_ngrams[tid]->size] = 1;
            local_ngrams[tid]->size++;
        }

        #pragma omp for
        for (int i = 0; i < word_count; i++) {
            for (int j = 0; j < (int)strlen(words[i]) - NGRAM_SIZE + 1; j++) {
                local_c_ngrams[tid]->ngrams[local_c_ngrams[tid]->size] = strdup(create_char_ngram(words[i], j));
                local_c_ngrams[tid]->frequencies[local_c_ngrams[tid]->size] = 1;
                local_c_ngrams[tid]->size++;
            }
        }

        qsort_r(local_ngrams[tid]->ngrams, local_ngrams[tid]->size, sizeof(char *), local_ngrams[tid], compare_strings);

        qsort_r(local_c_ngrams[tid]->ngrams, local_c_ngrams[tid]->size, sizeof(char*), local_c_ngrams[tid], compare_strings);

        #pragma omp critical
        {
            for (int i = 0; i < local_ngrams[tid]->size; i++) {
                word_ngram->ngrams[word_ngram->size] = strdup(local_ngrams[tid]->ngrams[i]);
                word_ngram->frequencies[word_ngram->size] = local_ngrams[tid]->frequencies[i];
                word_ngram->size++;
            }

            for (int i = 0; i < local_c_ngrams[tid]->size; i++) {
                character_ngram->ngrams[character_ngram->size] = strdup(local_c_ngrams[tid]->ngrams[i]);
                character_ngram->frequencies[character_ngram->size] = local_c_ngrams[tid]->frequencies[i];
                character_ngram->size++;
            }
        }

        free_histogram(local_ngrams[tid]);
        free_histogram(local_c_ngrams[tid]);
    }
    free(words);
    free(local_ngrams);
    free(local_c_ngrams);

    qsort_r(word_ngram->ngrams, word_ngram->size, sizeof(char *), word_ngram, compare_strings);

    qsort_r(character_ngram->ngrams, character_ngram->size, sizeof(char*), character_ngram, compare_strings);

    int word_size = reduce(word_ngram, word_ngram, 0);
    int character_size = reduce(character_ngram, character_ngram, 0);

    double end_time = omp_get_wtime();
    for (int i = 0; i < word_size; i++) {
        if (word_ngram->frequencies[i] > 10000) {
            printf("%s %d\n", word_ngram->ngrams[i], word_ngram->frequencies[i]);
        }
    }
    for (int i = 0; i < character_size; i++) {
        if (character_ngram->frequencies[i] > 200000) {
            printf("%s %d\n", character_ngram->ngrams[i], character_ngram->frequencies[i]);
        }
    }
    printf("Execution Time: %.6f seconds\n", end_time - start_time);
    free(text);

    return EXIT_SUCCESS;
}