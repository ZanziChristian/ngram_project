//
// Created by Christian Zanzi on 12/12/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>

#define NUM_THREADS 8

typedef struct
{
    char *ch1;
    char *ch2;
    int *frequencies;
    int size;
} BigramsOfCharacters;

typedef struct
{
    char (*word1)[50];
    char (*word2)[50];
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
    bigramsOfWords->word1 = (char (*)[50]) malloc(length * 50 * sizeof(char));
    bigramsOfWords->word2 = (char (*)[50]) malloc(length * 50 * sizeof(char));
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
    char (*tokens)[50] = (char (*)[50]) malloc(50 * strlen(text) * sizeof(char));

    char *saveptr = NULL;
    int idx = 0;
    for (char *token = strtok_r(text, " ,.!?;:+*-'()[]{}/$%&_", &saveptr);
         token != NULL && idx < 260000;
         token = strtok_r(NULL, " ,.!?;:+*-'()[]{}/$%&_", &saveptr)) {
        strncpy(tokens[idx], token, strlen(token));
        idx++;
         }

    free(text);

    BigramsOfWords* bigrams_of_words = createBigramsOfWords(idx);
    BigramsOfCharacters* bigrams_of_characters = createBigramsOfCharacters();

    BigramsOfWords local_words[NUM_THREADS];
    BigramsOfCharacters local_characters[NUM_THREADS];

    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int chunk_size = idx / NUM_THREADS;

        local_words[thread_id] = *createBigramsOfWords(chunk_size);
        local_characters[thread_id] = *createBigramsOfCharacters();
        local_words[thread_id].size = 0;
        local_characters[thread_id].size = 0;
        int start = thread_id * chunk_size;
        int end = (thread_id == NUM_THREADS - 1) ? idx : start + chunk_size + 1;

        // Worker calcola bigrammi nel proprio chunk
        for (int i = start; i < end; i++) {
            bool found = false;
            for (int j = 0; j < local_words[thread_id].size; j++) {
                if (strcmp(local_words[thread_id].word1[j], tokens[i]) == 0 &&
                    strcmp(local_words[thread_id].word2[j], tokens[i + 1]) == 0) {
                    local_words[thread_id].frequencies[j]++;
                    found = true;
                    j = local_words[thread_id].size;
                }
            }
            if (!found) {
                strncpy(local_words[thread_id].word1[local_words[thread_id].size], tokens[i], strlen(tokens[i]));
                strncpy(local_words[thread_id].word2[local_words[thread_id].size], tokens[i + 1], strlen(tokens[i + 1]));
                local_words[thread_id].frequencies[local_words[thread_id].size] = 1;
                local_words[thread_id].size++;
            }

            /*for (int k = 0; k < ((int)strlen(tokens[i])-1); k++)
            {
                bool ch_found = false;
                for (int w = 0; w < local_characters[thread_id].size; w++)
                {
                    if (local_characters[thread_id].ch1[w] == tokens[i][k] &&
                        local_characters[thread_id].ch2[w] == tokens[i][k+1])
                    {
                        local_characters[thread_id].frequencies[w]++;
                        ch_found = true;
                        w = local_characters[thread_id].size;
                    }
                }
                if (!ch_found)
                {
                    local_characters[thread_id].ch1[local_characters[thread_id].size] = tokens[i][k];
                    local_characters[thread_id].ch2[local_characters[thread_id].size] = tokens[i][k + 1];
                    local_characters[thread_id].frequencies[local_characters[thread_id].size] = 1;
                    local_characters[thread_id].size++;
                }
            }*/
        }
    }

    // Master raccoglie i risultati dai Workers
    bigrams_of_words->size = 0;
    bigrams_of_characters->size = 0;
    for (int x = 0; x < NUM_THREADS; x++) {
        for (int y = 0; y < local_words[x].size; y++) {
            bool found = false;
            for (int z = 0; z < bigrams_of_words->size; z++) {
                if (strcmp(bigrams_of_words->word1[z], local_words[x].word1[y]) == 0 &&
                    strcmp(bigrams_of_words->word2[z], local_words[x].word2[y]) == 0) {
                    bigrams_of_words->frequencies[z] += local_words[x].frequencies[y];
                    found = true;
                    if (strcmp(local_words[x].word1[y], "of") == 0 &&
                    strcmp(local_words[x].word2[y], "the") == 0)
                    {
                        printf(" start: %s freq:%d it: %d\n", local_words[x].word1[y], local_words[x].frequencies[y], x);
                    }
                    z = bigrams_of_words->size;
                }
            }
            if (!found) {
                strncpy(bigrams_of_words->word1[bigrams_of_words->size], local_words[x].word1[y], strlen(local_words->word1[y]));
                strncpy(bigrams_of_words->word2[bigrams_of_words->size], local_words[x].word2[y], strlen(local_words->word2[y]));
                bigrams_of_words->frequencies[bigrams_of_words->size] = local_words[x].frequencies[y];
                bigrams_of_words->size++;
            }

            for (int p = 0; p < strlen(local_words[x].word1[y]); p++) {
                bool ch_found = false;
                for (int q = 0; q < bigrams_of_characters->size; q++) {
                    if (bigrams_of_characters->ch1[q] == local_characters[x].ch1[p] &&
                        bigrams_of_characters->ch2[q] == local_characters[x].ch2[p]) {
                        bigrams_of_characters->frequencies[q] += local_characters[x].frequencies[p];
                        ch_found = true;
                        if (strcmp(&local_characters[x].ch1[p], "t") == 0 &&
                        strcmp(&local_characters[x].ch2[p], "h") == 0)
                        {
                            //printf(" start: %s freq:%d it: %d\n", local_words[x].word1[y], local_words[x].frequencies[y], x);
                        }
                        q = bigrams_of_words->size;
                        }
                }
                if (!ch_found) {
                    bigrams_of_characters->ch1[bigrams_of_characters->size] = local_characters[x].ch1[p];
                    bigrams_of_characters->ch2[bigrams_of_characters->size] = local_characters[x].ch2[p];
                    bigrams_of_characters->frequencies[bigrams_of_characters->size] = local_characters[x].frequencies[p];
                    bigrams_of_characters->size++;
                }
            }
        }
    }

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
