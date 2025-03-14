//
// Created by Christian Zanzi on 15/02/25.
//
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>

#define MAX_WORD_LEN 50
#define MAX_WORDS 900000
#define NGRAM_SIZE 3

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
    (*word_count)--;
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

int reduce(Histogram *h1, const Histogram *h2, int index) {
    for (int i = 0; i < h2->size; i++) {
        if (index != 0 && strcmp(h2->ngrams[i], h1->ngrams[index - 1]) == 0) {
            h1->frequencies[index - 1] += h2->frequencies[i];
        } else {
            h1->ngrams[index] = strdup(h2->ngrams[i]);
            h1->frequencies[index] = h2->frequencies[i];
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
    for (int i = 0; i < word_count; i++) {
        character_count += strlen(words[i]);
    }

    Histogram *histogram = create_histogram(word_count);
    Histogram *char_histogram = create_histogram(character_count);

    // bigrams of words map phase
    for (int i = 0; i < word_count - NGRAM_SIZE + 1; i++) {
        histogram->ngrams[i] = strdup(create_ngram(words, i));
        histogram->frequencies[i] = 1;
        histogram->size++;
    }

    // bigrams of characters map phase
    for (int i = 0; i < word_count; i++) {
        for (int j = 0; j < (int)strlen(words[i]) - NGRAM_SIZE + 1; j++) {
            char_histogram->ngrams[char_histogram->size] = strdup(create_char_ngram(words[i], j));
            char_histogram->frequencies[char_histogram->size] = 1;
            char_histogram->size++;
        }
    }

    free(words);
    // bigrams of words sorting
    qsort_r(histogram->ngrams, histogram->size, sizeof(char *), histogram, compare_strings);

    histogram->size = reduce(histogram, histogram, 0);

    // bigrams of characters sorting
    qsort_r(char_histogram->ngrams, char_histogram->size, sizeof(char*), char_histogram, compare_strings);

    char_histogram->size = reduce(char_histogram, char_histogram, 0);

    double end_time = omp_get_wtime();
    for (int i = 0; i < histogram->size; i++) {
        if (histogram->frequencies[i] > 10000) {
            printf("%s %d\n", histogram->ngrams[i], histogram->frequencies[i]);
        }
    }
    for (int i = 0; i < char_histogram->size; i++) {
        if (char_histogram->frequencies[i] > 200000) {
            printf("%s %d\n", char_histogram->ngrams[i], char_histogram->frequencies[i]);
        }
    }
    printf("Execution Time: %.6f seconds\n", end_time - start_time);
    free(text);

    free_histogram(histogram);
    free_histogram(char_histogram);
    return EXIT_SUCCESS;
}