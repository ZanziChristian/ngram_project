//
// Created by Christian Zanzi on 21/11/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>
#include <thread>

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
    long text_length = strlen(text);
    int segment_per_thread = text_length / NUM_THREADS;
    char (*tokens)[50] = (char (*)[50]) malloc(50 * text_length * sizeof(char));
    int total_tokens = 0;

    int chunk_ends[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS - 1; i++)
    {
        int chunk_end = segment_per_thread * (i + 1);
        while ((text[chunk_end] > 47 && text[chunk_end] < 58)
            || (text[chunk_end] > 64 && text[chunk_end] < 91)
            || (text[chunk_end] > 96 && text[chunk_end] < 123))
        {
            chunk_end++;
        }
        chunk_ends[i] = chunk_end;
    }
    chunk_ends[NUM_THREADS - 1] = text_length;

    int local_length[NUM_THREADS] = {0};
    #pragma omp parallel for num_threads(NUM_THREADS)
    for (int thread_id = 0; thread_id < NUM_THREADS; thread_id++) {
        int start_idx = (thread_id == 0) ? 0 : chunk_ends[thread_id - 1];
        int end_idx = chunk_ends[thread_id];
        char *local_text = &text[start_idx];

        char (*local_tokens)[50] = (char (*)[50]) malloc(50 * text_length * sizeof(char));
        int local_idx = 0;

        // Tokenizzazione del segmento assegnato
        char *saveptr = NULL;
        for (char *token = strtok_r(local_text, " ,.!?;:+*-'()[]{}/$%&_", &saveptr);
             token != NULL && token < &text[end_idx];
             token = strtok_r(NULL, " ,.!?;:+*-'()[]{}/$%&_", &saveptr)) {
            strncpy(local_tokens[local_idx], token, strlen(token));
            local_idx++;
        }
        /*for (int i = thread_id - 1; i < NUM_THREADS - 1; i++)
        {
            local_length[i] += local_idx;

        }
        printf("Thread #%d - %d\n", thread_id, local_length[thread_id]);*/

        // Copia locale nella memoria globale
        #pragma omp critical
        {
            for (int i = 0; i < NUM_THREADS - 1; i++)
            {
                local_length[i] = local_length[i - 1];

            }

            int start = total_tokens;
            printf("%d, start: %d, local_idx: %d\n", thread_id, local_length[thread_id], local_idx);
            memcpy(&tokens[local_length[thread_id]], local_tokens, local_idx * sizeof(char));
            /*for (int i = 0; i < local_idx; i++) {
                strncpy(tokens[start + i], local_tokens[i], strlen(local_tokens[i]));
            }*/
            total_tokens += local_idx;
        }

        free(local_tokens);
    }
    /*#pragma omp parallel num_threads(NUM_THREADS)
    {

        char (*local_tokens)[50] = (char (*)[50]) malloc(50 * text_length * sizeof(char));
        int thread_id = omp_get_thread_num();
        char *saveptr = NULL;
        int local_idx = 0;

        for (char *token = strtok_r(&text[chunk_ends[thread_id - 1]], " ,.!?;:+*-'()[]{}/$%&_", &saveptr);
             token != NULL && local_idx < chunk_ends[thread_id];
             token = strtok_r(NULL, " ,.!?;:+*-'()[]{}/$%&_", &saveptr))
        {
            strncpy(local_tokens[local_idx], token, strlen(token));
            local_idx++;
        }

        local_length[thread_id] = local_idx;
        #pragma omp barrier
            int start = 0;
            for (int j = 0; j < thread_id; j++)
            {
                start += local_length[j];
            }
            for (int i = 0; i < local_idx; i++)
            {
                strncpy(tokens[start + i], local_tokens[i], strlen(local_tokens[i]));
            }
            total_tokens += local_idx;

        free(local_tokens);
    }*/

    free(text);
    BigramsOfWords* bigrams_of_words = createBigramsOfWords(total_tokens);
    BigramsOfCharacters* bigrams_of_characters = createBigramsOfCharacters();

    for (int i = 0; i < total_tokens; i++)
    {
        bool bigram_found = false;
        for (int j = 0; j < bigrams_of_words->size; j++)
        {
            if (strcmp(bigrams_of_words->word1[j], tokens[i]) == 0 && strcmp(bigrams_of_words->word2[j], tokens[i + 1]) == 0)
            {
                bigrams_of_words->frequencies[j]++;
                bigram_found = true;
            }
        }

        if (!bigram_found)
        {
            strncpy(bigrams_of_words->word1[bigrams_of_words->size], tokens[i], strlen(tokens[i]));
            strncpy(bigrams_of_words->word2[bigrams_of_words->size], tokens[i + 1], strlen(tokens[i + 1]));
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