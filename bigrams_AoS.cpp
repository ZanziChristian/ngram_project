//
// Created by Christian Zanzi on 19/11/24.
//
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <omp.h>

typedef struct {
    char ch1;
    char ch2;
    int frequency;
} BigramOfCharacter;

typedef struct {
    BigramOfCharacter* bigrams;
    int size;
    int capacity;
} BigramOfCharacterArray;

typedef struct {
    char *word1;
    char *word2;
    int frequency;
} Bigram;

typedef struct {
    Bigram* bigrams;
    int size;
    int capacity;
} BigramArray;

BigramOfCharacterArray *createBigramOfCharacterArray() {
    BigramOfCharacterArray *bigram_array = (BigramOfCharacterArray *)malloc(sizeof(BigramOfCharacterArray));
    bigram_array->bigrams = (BigramOfCharacter *)malloc(1000 * sizeof(BigramOfCharacter));
    bigram_array->size = 0;
    bigram_array->capacity = 1000;
    return bigram_array;
}

void addBigramOfCharacter(BigramOfCharacterArray *bigram_array, const char ch1, const char ch2) {
    if (bigram_array == nullptr) {
        return;
    }
    for (int i = 0; i < bigram_array->size; i++) {
        if (bigram_array->bigrams[i].ch1 == ch1 &&
            bigram_array->bigrams[i].ch2 == ch2) {
            bigram_array->bigrams[i].frequency++;
            return;
            }
    }

    if (bigram_array->size >= bigram_array->capacity) {
        bigram_array->capacity += 1000;
        bigram_array->bigrams = (BigramOfCharacter *)realloc(bigram_array->bigrams, bigram_array->capacity * sizeof(BigramOfCharacter));
    }

    bigram_array->bigrams[bigram_array->size] = (BigramOfCharacter){ch1, ch2, 1};
    bigram_array->size++;
}

void printBigramsOfCharacters(BigramOfCharacterArray *bigram_array) {
    for (int i = 0; i < bigram_array->size; i++) {
        if (bigram_array->bigrams[i].frequency > 15000)
        {
            printf("Bigramma %d: \"%c %c\", Frequenza: %d\n",
               i,
               bigram_array->bigrams[i].ch1,
               bigram_array->bigrams[i].ch2,
               bigram_array->bigrams[i].frequency);
        }
    }
}

BigramArray *createBigramArray() {
    BigramArray *bigram_array = (BigramArray *)malloc(sizeof(BigramArray));
    bigram_array->bigrams = (Bigram *)malloc(10000 * sizeof(Bigram));
    bigram_array->size = 0;
    bigram_array->capacity = 10000;
    return bigram_array;
}

void addBigram(BigramArray *bigram_array, const char *word1, const char *word2) {
    if (bigram_array == nullptr) return;

    for (int i = 0; i < bigram_array->size; i++) {
        if (strcmp(bigram_array->bigrams[i].word1, word1) == 0 &&
            strcmp(bigram_array->bigrams[i].word2, word2) == 0) {
            bigram_array->bigrams[i].frequency++;
            return;
        }
    }

    if (bigram_array->size >= bigram_array->capacity) {
        bigram_array->capacity += 1000;
        bigram_array->bigrams = (Bigram*)realloc(bigram_array->bigrams, bigram_array->capacity * sizeof(Bigram));
    }

    bigram_array->bigrams[bigram_array->size] = (Bigram){strdup(word1), strdup(word2), 1};
    bigram_array->size++;
}

void printBigrams(BigramArray *bigram_array) {
    for (int i = 0; i < bigram_array->size; i++) {
        if (bigram_array->bigrams[i].frequency > 700) {
            printf("Bigramma %d: \"%s %s\", Frequenza: %d\n",
                  i,
                  bigram_array->bigrams[i].word1,
                  bigram_array->bigrams[i].word2,
                  bigram_array->bigrams[i].frequency);
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
/*
 *
 * A function that takes an array containing the text of the document and create the tokenization, i. e. an array
 * in which there are the words written only with alphanumeric characters and every punctuation symbol is
 * removed.
 *
 */
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

void findBigramsInAToken(BigramOfCharacterArray *bigram_array, const char *word) {
    size_t len = strlen(word);
    for (size_t i = 0; i < len - 1; i++) {
        addBigramOfCharacter(bigram_array, word[i], word[i + 1]);
    }
}

int main()
{
    const char* filename = "mobydick.txt";
    char* text = load_text(filename);
    if (text == nullptr) {
        return EXIT_FAILURE;
    }

    //printf("numero di processori: %d \n", omp_get_num_procs());

    double start_time = omp_get_wtime();
    char* tokens = tokenize(text);
    free(text);
    BigramArray* b_array = createBigramArray();
    BigramOfCharacterArray* bc_array = createBigramOfCharacterArray();

    char* word1 = strtok(tokens, " ");
    char* word2 = strtok(nullptr, " ");

    while (word2 != nullptr) {
        findBigramsInAToken(bc_array, word1);
        addBigram(b_array, word1, word2);

        word1 = word2;
        word2 = strtok(nullptr, " ");
    }
    findBigramsInAToken(bc_array, word1);

    double end_time = omp_get_wtime();
    printf("Tempo di esecuzione: %.6f secondi\n", end_time - start_time);

    printf("\nBigrammi di caratteri:\n");
    printBigramsOfCharacters(bc_array);

    printf("\nBigrammi di parole:\n");
    printBigrams(b_array);

    free(tokens);
    for (int i = 0; i < b_array->size; i++) {
        free(b_array->bigrams[i].word1);
        free(b_array->bigrams[i].word2);
    }
    free(b_array->bigrams);
    free(b_array);
    free(bc_array->bigrams);
    free(bc_array);

    return EXIT_SUCCESS;
}