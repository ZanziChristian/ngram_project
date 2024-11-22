//
// Created by Christian Zanzi on 19/11/24.
//
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <omp.h>

typedef struct
{
    char ch1;
    char ch2;
    int frequency;
} BigramOfCharacter;

typedef struct
{
    BigramOfCharacter* bigrams;
    int size;
    int capacity;
} BigramOfCharacterArray;

typedef struct
{
    char *word1;
    char *word2;
    int frequency;
} Bigram;

typedef struct
{
    Bigram* bigrams;
    int size;
    int capacity;
} BigramArray;

BigramOfCharacterArray *createBigramOfCharacterArray()
{
    BigramOfCharacterArray *bigram_array = (BigramOfCharacterArray *)malloc(sizeof(BigramOfCharacterArray));
    bigram_array->bigrams = (BigramOfCharacter *)malloc(100000 * sizeof(BigramOfCharacter));
    bigram_array->size = 0;
    bigram_array->capacity = 1000;
    return bigram_array;
}

BigramOfCharacter *createBigramOfCharacter(const char ch1, const char ch2) {
    BigramOfCharacter *bigram = (BigramOfCharacter *)malloc(sizeof(BigramOfCharacter));
    if (bigram == NULL) {
        printf("Errore di allocazione memoria per il Bigram\n");
        return NULL;
    }

    bigram->ch1 = ch1;
    bigram->ch2 = ch2;
    bigram->frequency = 1;
    return bigram;
}

void addBigramOfCharacter(BigramOfCharacterArray *bigram_array, const char ch1, const char ch2) {
    // Check if the bigram is already in the array
    bool found = false;
    if (bigram_array == NULL)
    {
        return;
    }
    for (int i = 0; i < bigram_array->size; i++) {
        if (bigram_array->bigrams[i].ch1 == ch1 &&
            bigram_array->bigrams[i].ch2 == ch2) {
            bigram_array->bigrams[i].frequency++;
            found = true;
            }
    }

    if (bigram_array->size >= bigram_array->capacity) {
        // extends the capacity if it reached the limit
        bigram_array->capacity += 3000;
        bigram_array->bigrams = (BigramOfCharacter *)realloc(bigram_array->bigrams, bigram_array->capacity * sizeof(BigramOfCharacter));
    }

    if (!found)
    {
        bigram_array->bigrams[bigram_array->size] = *createBigramOfCharacter(ch1, ch2);
        bigram_array->size++;
    }

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

BigramArray *createBigramArray()
{
    BigramArray *bigram_array = (BigramArray *)malloc(sizeof(BigramArray));
    bigram_array->bigrams = (Bigram *)malloc(100000 * sizeof(Bigram));
    bigram_array->size = 0;
    bigram_array->capacity = 100000;
    return bigram_array;
}

Bigram *createBigram(const char *word1, const char *word2) {
    Bigram *bigram = (Bigram *)malloc(sizeof(Bigram));
    if (bigram == NULL) {
        printf("Errore di allocazione memoria per il Bigram\n");
        return NULL;
    }

    bigram->word1 = strdup(word1); // Duplica le parole
    bigram->word2 = strdup(word2);
    bigram->frequency = 1;         // Frequenza iniziale impostata a 1
    return bigram;
}

void addBigram(BigramArray *bigram_array, const char *word1, const char *word2) {
    // Check if the bigram is already in the array
    bool found = false;
    if (bigram_array == NULL)
    {
        return;
    }
    for (int i = 0; i < bigram_array->size; i++) {
        if (strcmp(bigram_array->bigrams[i].word1, word1) == 0 &&
            strcmp(bigram_array->bigrams[i].word2, word2) == 0) {
            bigram_array->bigrams[i].frequency++;
            found = true;
            }
    }

    if (bigram_array->size >= bigram_array->capacity) {
        // extends the capacity if it reached the limit
        bigram_array->capacity += 10000;
        bigram_array->bigrams = (Bigram *)realloc(bigram_array->bigrams, bigram_array->capacity * sizeof(Bigram));
    }

    if (!found)
    {
        bigram_array->bigrams[bigram_array->size] = *createBigram(word1, word2);
        bigram_array->size++;
    }

}

void printBigrams(BigramArray *bigram_array) {
    for (int i = 0; i < bigram_array->size; i++) {
        if (bigram_array->bigrams[i].frequency > 700)
        {
            printf("Bigramma %d: \"%s %s\", Frequenza: %d\n",
                  i,
                  bigram_array->bigrams[i].word1,
                  bigram_array->bigrams[i].word2,
                  bigram_array->bigrams[i].frequency);
        }
    }
}

// return the length of the document
long get_file_length(FILE* file) {
    fseek(file, 0, SEEK_END);  // Spostati alla fine del file
    long length = ftell(file);  // Ottieni la posizione del cursore (che è la lunghezza del file)
    fseek(file, 0, SEEK_SET);  // Ripristina il cursore all'inizio del file
    return length;
}

char* load_text(const char* filename)
{
    FILE* text_file = fopen(filename, "r, css=UTF-8");
    if (text_file == NULL)
    {
        printf("Could not open file %s\n", filename);
        return NULL;
    }

    long file_length = get_file_length(text_file);

    char *text = (char*) malloc((file_length + 1) * sizeof(char));

    if (text == NULL)
    {
        printf("Could not allocate memory for text\n");
        fclose(text_file);
        return NULL;
    }

    int ch;
    for (int i = 0; i < file_length; i++)
    {
        ch = getc(text_file);
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
    fclose(text_file);

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
    char* tokens = (char*) malloc(strlen(text));
    int idx = 0, start = 0, end = 0;
    // cycle for divide the tokens inside an array
    for (int i=0; i < strlen(text); i++)
    {
        // if a character i is not a letter or a number
        if ((text[i] >= 32 && text[i] <=47) || (text[i] >= 58 && text[i] <=64)
            || (text[i] >= 91 && text[i] <=96) || (text[i] >= 123 && text[i] <=126))
        {
            end = i - 1; // a token end is found
            char* token = (char*) malloc((end - start + 1) * sizeof(char));
            // if the first character of a token is a letter or a number
            if ((text[start] > 47 && text[start] < 58) || (text[start] > 64 &&
                text[start] < 91) || (text[start] > 96 && text[start] < 123))
            {
                strncpy(token, &text[start], end - start + 2);
                token[end - start + 1] = ' ';
                token[end - start + 2] = '\0';
                strncpy(&tokens[idx], token, end - start + 2);
                // the token is taken without punctuation and inserted in an array
                idx += end - start + 2;
            }
            start = i + 1;
            free(token);
        }
    }
    return tokens;
}

void findBigramsInAToken(BigramOfCharacterArray *bigram_array, const char *word)
{
    if (word == NULL)
        return;
    for (int i = 0; i < strlen(word) - 1; i++)
    {
        addBigramOfCharacter(bigram_array, word[i], word[i + 1]);
    }
}

int main()
{
    const char* filename = "mobydick.txt";
    char* text = load_text(filename);
    if (text == NULL) {
        return 1;
    }

    //printf("numero di processori: %d \n", omp_get_num_procs());

    double start_time = omp_get_wtime();
    char* tokens = tokenize(text);
    free(text);
    BigramArray* b_array = createBigramArray();
    BigramOfCharacterArray* bc_array = createBigramOfCharacterArray();

    for (int i = 0; i < strlen(tokens); i++)
    {
        char* word1 = (char*)malloc(strlen(&tokens[i]));
        sscanf(&tokens[i], "%s", word1);
        i += strlen(word1);
        char* word2 = (char*)malloc(strlen(&tokens[i]));
        sscanf(&tokens[i], "%s", word2);

        findBigramsInAToken(bc_array, word1);

        addBigram(b_array, word1, word2);
        free(word1);
        free(word2);
    }
    double end_time = omp_get_wtime();
    printf("Execution time: %f s\n", end_time - start_time);

    printBigramsOfCharacters(bc_array);
    printBigrams(b_array);
    for (int i = 0; i < b_array->size; i++) {
        free(b_array->bigrams[i].word1);
        free(b_array->bigrams[i].word2);
    }
    free(tokens);
    free(b_array->bigrams);
    free(bc_array);
    free(b_array);
    return 0;
}