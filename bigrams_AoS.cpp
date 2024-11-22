//
// Created by Christian Zanzi on 19/11/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>

typedef struct
{
    char *ch1;
    char *ch2;
    int *frequencies;
    int size;
} BigramsOfCharacters;

typedef struct
{
    char (*word1)[30];
    char (*word2)[30];
    int *frequencies;
    int size;
} BigramsOfWords;

BigramsOfCharacters* createBigramsOfCharacters()
{
    auto *bigramsOfCharacters = (BigramsOfCharacters *)malloc(sizeof(BigramsOfCharacters));
    bigramsOfCharacters->ch1 = (char *) malloc(1000 * sizeof(char));
    bigramsOfCharacters->ch2 = (char *) malloc(1000 * sizeof(char));
    bigramsOfCharacters->frequencies = (int *) malloc(1000 * sizeof(int));

    return bigramsOfCharacters;
}

BigramsOfWords* createBigramsOfWords(int length)
{
    auto *bigramsOfWords = (BigramsOfWords *)malloc(sizeof(BigramsOfWords));
    bigramsOfWords->word1 = (char (*)[30]) malloc(length * 50 * sizeof(char));
    bigramsOfWords->word2 = (char (*)[30]) malloc(length * 50 * sizeof(char));
    bigramsOfWords->frequencies = (int *) malloc(length * sizeof(int));

    return bigramsOfWords;
}

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

char (*tokenize(const char *text, int *length))[50]
{
    //char** tokens = (char**) malloc(strlen(text)*sizeof(char*));
    char (*tokens)[50] = (char (*)[50]) malloc(50 * strlen(text) * sizeof(char));
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
                /*strncpy(token, &text[start], end - start + 2);
                token[end - start + 1] = ' ';
                token[end - start + 2] = '\0';
                tokens[idx] = (char*) malloc(end - start + 1 * sizeof(char));*/
                strncpy(token, &text[start], end - start + 1);
                token[end - start + 1] = '\0';
                strncpy(tokens[idx], token, end - start + 1);
                // the token is taken without punctuation and inserted in an array
                idx++;
            }
            start = i + 1;
            free(token);
        }
    }
    *length = idx;
    return tokens;
}

int main()
{
    const char* filename = "mobydick.txt";
    char* text = load_text(filename);
    if (text == NULL) {
        return 1;
    }

    double start_time = omp_get_wtime();
    int length = 0;
    //char** tokens = tokenize(text, &length);
    char (*tokens)[50] = tokenize(text, &length);
    free(text);
    BigramsOfWords* bigrams_of_words = createBigramsOfWords(length);
    BigramsOfCharacters* bigrams_of_characters = createBigramsOfCharacters();

    for (int i = 0; i < length - 1; i++)
    {

        bool bigram_found = false;

        for (int j = 0; j < bigrams_of_words->size; j++)
        {
            if (strcmp(bigrams_of_words->word1[j], tokens[i]) == 0 && strcmp(bigrams_of_words->word2[j], tokens[i + 1]) == 0) {
                bigrams_of_words->frequencies[j]++;
                bigram_found = true;
            }
        }

        if (!bigram_found)
        {
            strncpy(bigrams_of_words->word1[bigrams_of_words->size], tokens[i], sizeof(bigrams_of_words->word1[bigrams_of_words->size]));
            strncpy(bigrams_of_words->word2[bigrams_of_words->size], tokens[i + 1], sizeof(bigrams_of_words->word2[bigrams_of_words->size]));
            bigrams_of_words->frequencies[bigrams_of_words->size] = 1;
            bigrams_of_words->size++;
        }

        for (int y = 0; y < ((int)strlen(tokens[i])-1); y++)
        {
            bool ch_found = false;
            for (int z = 0; z < bigrams_of_characters->size; z++)
            {
                //printf("%s %c %c %d \n", tokens[i], tokens[i][y], tokens[i][y+1], ((int)strlen(tokens[i])-1));

                if (bigrams_of_characters->ch1[z] == tokens[i][y] &&
                    bigrams_of_characters->ch2[z] == tokens[i][y+1])
                {
                    bigrams_of_characters->frequencies[z]++;
                    ch_found = true;
                }
            }
            if (!ch_found)
            {
                bigrams_of_characters->ch1[bigrams_of_characters->size] = tokens[i][y];
                bigrams_of_characters->ch2[bigrams_of_characters->size] = tokens[i][y + 1];
                bigrams_of_characters->frequencies[bigrams_of_characters->size] = 1;
                bigrams_of_characters->size++;
            }
        }

    }
    /*
    for (int x = 0; x < length; x++)
    {
        for (int y = 0; y < ((int)strlen(tokens[x])); y++)
        {
            bool found = false;
            for (int z = 0; z < bigrams_of_words->size; z++)
            {
                printf("%s %c %c %d \n", tokens[x], tokens[x][y], tokens[x][y+1], ((int)strlen(tokens[x])-1));

                //if (bigrams_of_characters->ch1[y] == bigrams_of_words->word1[x][y] &&
                //    bigrams_of_characters->ch2[y] == bigrams_of_words->word2[x][y + 1])
                if (bigrams_of_characters->ch1[z] == tokens[x][y] &&
                    bigrams_of_characters->ch2[z] == tokens[x][y+1])
                {
                    bigrams_of_characters->frequencies[y]++;
                    found = true;
                }
            }
            if (!found)
            {
                //bigrams_of_characters->ch1[bigrams_of_characters->size] = bigrams_of_words->word1[x][y];
                //bigrams_of_characters->ch2[bigrams_of_characters->size] = bigrams_of_words->word2[x][y + 1];
                bigrams_of_characters->ch1[bigrams_of_characters->size] = tokens[x][y];
                bigrams_of_characters->ch2[bigrams_of_characters->size] = tokens[x][y + 1];
                bigrams_of_characters->frequencies[bigrams_of_characters->size] = 1;
                bigrams_of_characters->size++;
            }
        }
    }
    */

    double end_time = omp_get_wtime();
    printf("%lf\n", end_time - start_time);

    for (int i = 0; i < bigrams_of_characters->size; i++) {
        if (bigrams_of_characters->frequencies[i] > 15000)
        {
            printf("Bigramma %d: \"%c %c\", Frequenza: %d\n",
               i,
               bigrams_of_characters->ch1[i],
               bigrams_of_characters->ch2[i],
               bigrams_of_characters->frequencies[i]);
        }
    }


    for (int i = 0; i < bigrams_of_words->size; i++)
    {
        if (bigrams_of_words->frequencies[i] > 700)
            printf("Bigramma %d: \"%s %s\", Frequenza: %d\n", i, bigrams_of_words->word1[i], bigrams_of_words->word2[i], bigrams_of_words->frequencies[i]);
    }

    //printf("%d \n", length);
    free(tokens);
    free(bigrams_of_words->word1);
    free(bigrams_of_words->word2);
    free(bigrams_of_words->frequencies);
    free(bigrams_of_words);
    free(bigrams_of_characters->ch1);
    free(bigrams_of_characters->ch2);
    free(bigrams_of_characters->frequencies);
    free(bigrams_of_characters);
    return 0;
}