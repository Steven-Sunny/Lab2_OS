#include "myshell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>

void tokenize (char *input, char **args){
    char *token = strtok(input, " \t\n");
    int index = 0;
    while (token != NULL && index < MAX_ARGS - 1) {
        args[index++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[index] = NULL;
}

void execute_clear(){
    //Clears the terminal screen using ANSI escape codes
    printf("\033[H\033[2J\033[3J");
    fflush(stdout);
}