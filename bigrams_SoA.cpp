//
// Created by Christian Zanzi on 19/11/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>

typedef struct {
    char *ch1;
    char *ch2;
    int *frequency;
    int size;
    int capacity;
} BigramOfCharacterSoA;

typedef struct {
    char **word1;
    char **word2;
    int *frequency;
    int size;
    int capacity;
} BigramSoA;

BigramOfCharacterSoA *createBigramOfCharacterSoA() {
    BigramOfCharacterSoA *bigram_array = (BigramOfCharacterSoA *)malloc(sizeof(BigramOfCharacterSoA));
    bigram_array->ch1 = (char *)malloc(1000 * sizeof(char));
    bigram_array->ch2 = (char *)malloc(1000 * sizeof(char));
    bigram_array->frequency = (int *)malloc(1000 * sizeof(int));
    bigram_array->size = 0;
    bigram_array->capacity = 1000;
    return bigram_array;
}

void addBigramOfCharacterSoA(BigramOfCharacterSoA *bigram_array, const char ch1, const char ch2) {
    if (bigram_array == nullptr) return;

    for (int i = 0; i < bigram_array->size; i++) {
        if (bigram_array->ch1[i] == ch1 && bigram_array->ch2[i] == ch2) {
            bigram_array->frequency[i]++;
            return;
        }
    }

    if (bigram_array->size >= bigram_array->capacity) {
        bigram_array->capacity += 1000;
        bigram_array->ch1 = (char *)realloc(bigram_array->ch1, bigram_array->capacity * sizeof(char));
        bigram_array->ch2 = (char *)realloc(bigram_array->ch2, bigram_array->capacity * sizeof(char));
        bigram_array->frequency = (int *)realloc(bigram_array->frequency, bigram_array->capacity * sizeof(int));
    }

    bigram_array->ch1[bigram_array->size] = ch1;
    bigram_array->ch2[bigram_array->size] = ch2;
    bigram_array->frequency[bigram_array->size] = 1;
    bigram_array->size++;
}

void printBigramOfCharacterSoA(BigramOfCharacterSoA *bigram_array) {
    for (int i = 0; i < bigram_array->size; i++) {
        if (bigram_array->frequency[i] > 15000) {
            printf("Bigramma %d: \"%c %c\", Frequenza: %d\n",
                   i,
                   bigram_array->ch1[i],
                   bigram_array->ch2[i],
                   bigram_array->frequency[i]);
        }
    }
}

BigramSoA *createBigramSoA() {
    BigramSoA *bigram_array = (BigramSoA *)malloc(sizeof(BigramSoA));
    bigram_array->word1 = (char **)malloc(1000 * sizeof(char *));
    bigram_array->word2 = (char **)malloc(1000 * sizeof(char *));
    bigram_array->frequency = (int *)malloc(1000 * sizeof(int));
    bigram_array->size = 0;
    bigram_array->capacity = 1000;
    return bigram_array;
}

void addBigramSoA(BigramSoA *bigram_array, const char *word1, const char *word2) {
    if (bigram_array == nullptr) return;

    for (int i = 0; i < bigram_array->size; i++) {
        if (strcmp(bigram_array->word1[i], word1) == 0 &&
            strcmp(bigram_array->word2[i], word2) == 0) {
            bigram_array->frequency[i]++;
            return;
        }
    }

    if (bigram_array->size >= bigram_array->capacity) {
        bigram_array->capacity += 1000;
        bigram_array->word1 = (char **)realloc(bigram_array->word1, bigram_array->capacity * sizeof(char *));
        bigram_array->word2 = (char **)realloc(bigram_array->word2, bigram_array->capacity * sizeof(char *));
        bigram_array->frequency = (int *)realloc(bigram_array->frequency, bigram_array->capacity * sizeof(int));
    }

    bigram_array->word1[bigram_array->size] = strdup(word1);
    bigram_array->word2[bigram_array->size] = strdup(word2);
    bigram_array->frequency[bigram_array->size] = 1;
    bigram_array->size++;
}

void printBigramSoA(BigramSoA *bigram_array) {
    for (int i = 0; i < bigram_array->size; i++) {
        if (bigram_array->frequency[i] > 700) {
            printf("Bigramma %d: \"%s %s\", Frequenza: %d\n",
                   i,
                   bigram_array->word1[i],
                   bigram_array->word2[i],
                   bigram_array->frequency[i]);
        }
    }
}

char* load_text(const char* filename) {
    FILE* file = fopen(filename, "r, css=UTF-8");
    if (file == nullptr) {
        printf("Could not open file %s\n", filename);
        return nullptr;
    }

    fseek(file, 0, SEEK_END);
    long file_length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *text = (char*) malloc((file_length + 1) * sizeof(char));

    if (text == nullptr) {
        printf("Could not allocate memory for text\n");
        fclose(file);
        return nullptr;
    }

    int ch;
    for (int i = 0; i < file_length; i++) {
        ch = getc(file);
        if (ch >= 32 && ch <= 126) // check the characters and take only the ascii ones from the [space] to the ~
        {
            text[i] = (char)tolower(ch);
        }
        else
        {
            text[i] = 32; // insert a space where a special utf-8 character is found
        }
    }

    text[file_length] = '\0';
    fclose(file);

    return text;
}

char* tokenize(const char* text)
{
    char* tokens = (char*) malloc(strlen(text) + 1);
    int idx = 0;

    for (size_t i = 0; text[i] != '\0'; i++)
    {
        if (isalnum(text[i])) {
            tokens[idx++] = text[i];
        }
        else {
            tokens[idx++] = ' ';
        }
    }
    tokens[idx] = '\0';
    return tokens;
}

void findBigramsInAToken(BigramOfCharacterSoA *bigram_array, const char *word) {
    size_t len = strlen(word);
    for (size_t i = 0; i < len - 1; i++) {
        addBigramOfCharacterSoA(bigram_array, word[i], word[i + 1]);
    }
}

int main() {
    const char *filename = "mobydick.txt";
    char *text = load_text(filename);
    if (text == nullptr) {
        return EXIT_FAILURE;
    }

    double start_time = omp_get_wtime();
    char *tokens = tokenize(text);
    free(text);

    BigramSoA *b_array = createBigramSoA();
    BigramOfCharacterSoA *bc_array = createBigramOfCharacterSoA();

    char *word1 = strtok(tokens, " ");
    char *word2 = strtok(nullptr, " ");

    while (word2 != nullptr) {
        findBigramsInAToken(bc_array, word1);
        addBigramSoA(b_array, word1, word2);

        word1 = word2;
        word2 = strtok(nullptr, " ");
    }
    findBigramsInAToken(bc_array, word1);

    double end_time = omp_get_wtime();
    printf("Tempo di esecuzione: %.6f secondi\n", end_time - start_time);

    printf("\nBigrammi di caratteri:\n");
    printBigramOfCharacterSoA(bc_array);

    printf("\nBigrammi di parole:\n");
    printBigramSoA(b_array);

    free(tokens);
    for (int i = 0; i < b_array->size; i++) {
        free(b_array->word1[i]);
        free(b_array->word2[i]);
    }
    free(b_array->word1);
    free(b_array->word2);
    free(b_array->frequency);
    free(b_array);

    free(bc_array->ch1);
    free(bc_array->ch2);
    free(bc_array->frequency);
    free(bc_array);

    return EXIT_SUCCESS;
}