/**
 * File: myshell.h
 */
#ifndef MYSHELL_H
#define MYSHELL_H
#define MAX_ARGS 64

void tokenize (char *line, char **args);

void execute_clear();

void execute_cd();

void execute_dir();

void execute_pause();

#endif // MYSHELL_H