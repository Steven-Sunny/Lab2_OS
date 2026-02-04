/**
 * File: utility.c
 */
#include "myshell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * 
 */
void tokenize (char *input, char **args){
    char *token = strtok(input, " \t\n");
    int index = 0;
    while (token != NULL && index < MAX_ARGS - 1) {
        args[index++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[index] = NULL;
}

/**
 * 
 */
void execute_clear(){
    //Clears the terminal screen using ANSI escape codes
    printf("\033[H\033[2J\033[3J");
    fflush(stdout);
}

/**
 * 
 */
void execute_cd(char **args) {
    char cwd[PATH_MAX];
    // No argument provided, print current directory
    if (args[1] == NULL) {
        // getcwd() fetches the pathname of the current working directory
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("Current directory: %s\n", cwd);
        } else {
            perror("cd: getcwd error: does the current directory exist anymore");
        }
    } 
    // Directory argument provided
    else {
        if (chdir(args[1]) != 0) {
            // Error reporting if directory doesn't exist
            perror("myshell: cd");
        } else {
            // Update the PWD environment variable after a successful change
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                // setenv() updates the environment variable to the new cwd
                setenv("PWD", cwd, 1); // 1 means TRUE to overwrite existing value
            }
        }
    }
}

/**
 * 
 */
void execute_pause(char **args){
    printf("Shell paused. Press Enter to continue...");
    // Ensure the message is printed immediately
    fflush(stdout); 

    // Read characters from standard input until a newline is encountered
    while (getchar() != '\n');
}

/**
 * 
 */
void execute_environ(){
    // TODO: Make this function work with output and input redirection
    int i = 0;
    extern char **environ;
    while (environ[i] != NULL) {
        printf("%s\n", environ[i]);
        i++;
    }
}

/**
 * 
 */
void execute_dir(char **args){
    // TODO: Make this function work with output and input redirection
    char *target;
    if (args[1] == NULL) {
        target = ".";
    } else {
        target = args[1];
    }
    DIR *dir_ptr = opendir(target);
    if (dir_ptr == NULL) {
        perror("Error opening directory");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir_ptr)) != NULL) {
        // Print the name of the file/folder
        printf("%s\n", entry->d_name);
    }
    closedir(dir_ptr);
}

/**
 * 
 */
void execute_echo(char **args){
    // TODO: Make this function work with output and input redirection
    // Start at 1 to skip the "echo" command itself
    int i = 1; 

    // Loop through all arguments until we hit NULL or a redirection operator
    while (args[i] != NULL) {
        // Check for redirection tokens (May need to change parser)
        // If your parser doesn't strip redirection tokens from 'args', 
        // you should stop printing when you encounter '>', '>>', or '<'.
        if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], "<") == 0) {
            break;
        }

        printf("%s", args[i]);

        // Add a single space between words if there is another argument coming
        if (args[i + 1] != NULL && 
            strcmp(args[i+1], ">") != 0 && 
            strcmp(args[i+1], ">>") != 0) {
            printf(" ");
        }
        i++;
    }
    
    // Always end with a newline
    printf("\n");
}