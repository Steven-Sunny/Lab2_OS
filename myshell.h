/**
 * File: myshell.h
 * Name: Steven Sun
 * Student Number: 100816207
 * Work Group: 29
 */

#ifndef MYSHELL_H
#define MYSHELL_H
#define MAX_ARGS 64

/**
 * Defines function prototypes for the built-in 
 * commands and utility functions used in myshell.c and utility.c
 */

void tokenize (char *line, char **args);

void execute_clear();

void execute_cd(char **args);

void execute_dir(char **args);

void execute_pause();

void execute_environ();

void execute_echo(char **args);

void execute_help();

int check_redirection(char **args);

void execute_external_command(char **args, int background);


#endif // MYSHELL_H