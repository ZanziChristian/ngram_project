//
// Created by Christian Zanzi on 15/02/25.
//
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>

#define MAX_WORD_LEN 50
#define MAX_BIGRAMS 40000
#define MAX_CHAR_BIGRAMS 400000
#define MAX_WORDS 900000

typedef struct {
    char **bigrams;
    int *count;
    int size;
} Dictionary;

Dictionary *create_dict(int length) {
    Dictionary *d = malloc(sizeof(Dictionary));
    d->bigrams = (char **)malloc(length * sizeof(char*) + length * MAX_WORD_LEN * sizeof(char));
    char *bigram = (char *)d->bigrams + length * sizeof(char*);
    for (int i = 0; i < length; i++) {
        d->bigrams[i] = bigram + i * MAX_WORD_LEN;
    }
    d->count = (int *)malloc(length * sizeof(int));
    d->size = 0;
    return d;
}

void free_dict(Dictionary *d) {
    free(d->count);
    free(d->bigrams);
    free(d);
}

int compare_bigrams(void *ptr, const void *a, const void *b)   {
    Dictionary *d = (Dictionary *)ptr;
    int idx1 = *(int *)a;
    int idx2 = *(int *)b;
    return strcmp(d->bigrams[idx1], d->bigrams[idx2]);
}

int compare_strings(void *ptr, const void *a, const void *b) {
    Dictionary *d = (Dictionary *)ptr;
    const char *str1 = *(const char * const *)a;
    const char *str2 = *(const char * const *)b;
    return strcmp(str1, str2);
}

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
        perror("Errore nell'apertura del file");
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    const long file_length = ftell(file);
    rewind(file);
    char *text = malloc((file_length + 1) * sizeof(char));
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

int reduce(Dictionary *global, const Dictionary *local, int index) {
    for (int i = 0; i < local->size; i++) {
        if (index != 0 && strcmp(local->bigrams[i], global->bigrams[index - 1]) == 0) {
            global->count[index - 1] += local->count[i];
        } else {
            global->bigrams[index] = strdup(local->bigrams[i]);
            global->count[index] = local->count[i];
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

    double start_prep = omp_get_wtime();
    char **words = malloc(MAX_WORDS * sizeof(char*) + MAX_WORDS * MAX_WORD_LEN * sizeof(char));
    char *word = (char *)words + MAX_WORDS * sizeof(char*);
    for (int i = 0; i < MAX_WORDS; i++) {
        words[i] = word + i * MAX_WORD_LEN;
    }
    int word_count = 0;
    char *saveptr;
    char *token = strtok_r(text, " ", &saveptr);
    while (token) {
        normalize_word(token);
        if (strlen(token) > 0) {
            words[word_count++] = token;
        }
        token = strtok_r(NULL, " ", &saveptr);
    }
    word_count--;
    double end_prep = omp_get_wtime();

    Dictionary *histogram = create_dict(word_count);
    Dictionary *char_histogram = create_dict(word_count*8);

    double start_wmap = omp_get_wtime();
    // bigrams of words map phase
    for (int i = 0; i < word_count - 1; i++) {
        snprintf(histogram->bigrams[i], MAX_WORD_LEN, "%s %s", words[i], words[i+1]);
        histogram->count[i] = 1;
    }
    histogram->size = word_count;
    double end_wmap = omp_get_wtime();

    double start_cmap = omp_get_wtime();
    // bigrams of characters map phase
    for (int i = 0; i < word_count; i++) {
        for (int j = 0; j < strlen(words[i]) - 1; j++) {
            snprintf(char_histogram->bigrams[char_histogram->size], 4, "%c %c", words[i][j], words[i][j+1]);
            char_histogram->count[char_histogram->size] = 1;
            char_histogram->size++;
        }
    }
    double end_cmap = omp_get_wtime();

    // bigrams of words sorting
    double start_wsort = omp_get_wtime();
    qsort_r(histogram->bigrams, histogram->size, sizeof(char *), histogram, compare_strings);
    double end_wsort = omp_get_wtime();

    double start_wcombine = omp_get_wtime();
    histogram->size = reduce(histogram, histogram, 0);
    double end_wcombine = omp_get_wtime();

    // bigrams of characters sorting
    double start_csort = omp_get_wtime();
    qsort_r(char_histogram->bigrams, char_histogram->size, sizeof(char*), char_histogram, compare_strings);
    double end_csort = omp_get_wtime();

    double start_ccombine = omp_get_wtime();
    char_histogram->size = reduce(char_histogram, char_histogram, 0);
    double end_ccombine = omp_get_wtime();

    double end_time = omp_get_wtime();
    for (int i = 0; i < histogram->size; i++) {
        if (histogram->count[i] > 200) {
            printf("%s %d\n", histogram->bigrams[i], histogram->count[i]);
        }
    }
    for (int i = 0; i < char_histogram->size; i++) {
        if (char_histogram->count[i] > 20000) {
            printf("%s %d\n", char_histogram->bigrams[i], char_histogram->count[i]);
        }
    }
    printf("Execution Time: %.6f seconds\n", end_time - start_time);
    printf("Prep time: %lf s\n", end_prep - start_prep);
    printf("--------------------------------------\n");
    printf("Wmap time: %lf s\n", end_wmap - start_wmap);
    printf("Wsort time: %lf s\n", end_wsort - start_wsort);
    printf("Wcombine time: %lf s\n", end_wcombine - start_wcombine);
    printf("--------------------------------------\n");
    printf("Cmap time: %lf s\n", end_cmap - start_cmap);
    printf("Csort time: %lf s\n", end_csort - start_csort);
    printf("Ccombine time: %lf s\n", end_ccombine - start_ccombine);


    free(text);

    free_dict(histogram);
    free_dict(char_histogram);
    return EXIT_SUCCESS;
}