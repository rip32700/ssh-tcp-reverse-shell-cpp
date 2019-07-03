//
// Created by rip on 7/1/19.
//

#ifndef SSH_TCP_REVERSE_SHELL_CPP_UTILS_H
#define SSH_TCP_REVERSE_SHELL_CPP_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct {
    char* array;
    size_t used;
    size_t size;
} Array;


int startsWith(const char*, const char*);
void tokenize(char*, const char*, char**);
char* concat(const char*, const char*);

int dirExists(char *);
char* readFile(FILE*, int*);


void initArray(Array*, size_t);
void insertArray(Array*, char);
void freeArray(Array*);

#endif //SSH_TCP_REVERSE_SHELL_CPP_UTILS_H
