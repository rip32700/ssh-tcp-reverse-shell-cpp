//
// Created by rip on 7/1/19.
//

#include "utils.h"

int startsWith(const char *string, const char *prefix)
{
    if (strncmp(string, prefix, strlen(prefix)) == 0) return 1;
    return 0;
}

void tokenize(char* string, const char* delimiters, char** tokens)
{
    int i = 0;
    char* token;

    /* Establish string and get the first token: */
    token = strtok(string, delimiters);
    while(token != NULL)
    {
        /* While there are tokens in "string" */
        tokens[i++] = token;
        /* Get next token: */
        token = strtok(NULL, delimiters);
    }
}

char* concat(const char *s1, const char *s2)
{
    char *result = (char*) malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void initArray(Array* a, size_t initialSize) {
    a->array = (char*)malloc(initialSize * sizeof(int));
    a->used = 0;
    a->size = initialSize;
}

void insertArray(Array* a, char element) {
    if (a->used == a->size) {
        a->size *= 2;
        a->array = (char*)realloc(a->array, a->size * sizeof(char));
    }
    a->array[a->used++] = element;
}

void freeArray(Array* a) {
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}

int dirExists(const char *path)
{
    struct stat info;

    if (stat(path, &info) != 0)
    {
        return 0;
    }
    else
    {
        return (info.st_mode & S_IFDIR) != 0;
    }
}

char* readFile(FILE* fp, int* length)
{
    char *source = NULL;
    if (fp != NULL)
    {
        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0)
        {
            /* Get the size of the file. */
            long bufsize = ftell(fp);
            if (bufsize == -1)
            {
                fprintf(stderr, "[-] Error reading file length\n");
            }

            /* Allocate our buffer to that size. */
            *length = sizeof(char) * (bufsize + 1);
            source = (char*) malloc(sizeof(char) * (bufsize + 1));

            /* Go back to the start of the file. */
            if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */ }

            /* Read the entire file into memory. */
            size_t newLen = fread(source, sizeof(char), bufsize, fp);
            if ( ferror( fp ) != 0 )
            {
                fprintf(stderr, "[-] Error reading file\n", stderr);
            } else {
                source[newLen++] = '\0'; /* Just to be safe. */
            }
        }
    }

    return source;
}