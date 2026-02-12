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

#include <sys/stat.h>
#include <fcntl.h>

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
void execute_pause(){
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
    // End with newline
    printf("\n");
}

/**
 * 
 */
void execute_help(){

    // Check if the readme file actually exists
    if (access("readme", F_OK) == -1) {
        printf("Error: 'readme' manual not found.\n");
        return;
    }

    // Uses execute_external_command()
    char *more_args[] = {"more", "readme", NULL};
    execute_external_command(more_args, 0);
}

/**
 * Helper to check for redirection tokens in the argument list.
 * This function modifies the args array by setting the redirection 
 * symbol to NULL, effectively truncating the arguments for execvp.
 * Returns 1 if redirection was found and handled, 0 otherwise, -1 on error.
 */
int check_redirection(char **args) {
    int i = 0;

    while (args[i] != NULL) {
        if (strcmp(args[i], ">") == 0) {
            // Output redirection (overwrite)
            if (args[i + 1] == NULL) {
                fprintf(stderr, "myshell: syntax error near unexpected token '>'\n");
                return -1;
            }
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { 
                perror("myshell: open"); 
                return -1; 
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL; // Truncate args
            return 1;
        } 
        else if (strcmp(args[i], ">>") == 0) {
            // Output redirection (append)
            if (args[i + 1] == NULL) {
                fprintf(stderr, "myshell: syntax error near unexpected token '>>'\n");
                return -1;
            }
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) { 
                perror("myshell: open"); 
                return -1; 
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
            return 1;
        } 
        else if (strcmp(args[i], "<") == 0) {
            // Input redirection
            if (args[i + 1] == NULL) {
                fprintf(stderr, "myshell: syntax error near unexpected token '<'\n");
                return -1;
            }
            int fd = open(args[i + 1], O_RDONLY);
            if (fd < 0) { 
                perror("myshell: open"); 
                return -1; 
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[i] = NULL;
            return 1;
        }
        i++;
    }
    return 0;
}

/**
 * 
 */
void execute_external_command(char **args, int is_background) {
    // Fork a child process to execute an external command
    pid_t pid = fork();

    // Get the parent executable path name from the environment variable "shell"
    char *parent_shell_name = getenv("shell");

    if (pid == -1) {
        perror("Fork failed");
    } else if (pid == 0) {
        // --- CHILD PROCESS ---
        // Set environment variable "parent" to the full path of myshell before exec of child process
        setenv("parent", parent_shell_name, 1);
        // printf("Parent shell path: %s\n", getenv("parent"));

        // Apply I/O redirection if present in command line
        if (check_redirection(args) == -1) {
            exit(1);
        }

        if (execvp(args[0], args) == -1) {
            perror("Execution failed");
            exit(1);
        }
    } else {
        // --- PARENT PROCESS ---
        if (!is_background) {
            // Wait for child if '&' is not present 
            waitpid(pid, NULL, 0);
        }
        // If it is background, we return to the prompt immediately
        return;
    }
}