#ifndef MYSHELL_H
#define MYSHELL_H

#define MAX_ARGS 64

void tokenize (char *line, char **args);

void execute_clear();

#endif // MYSHELL_H