/*
Lab 2 – A Simple Shell

Requirements (Refer to the document for accuracy and details):
- internal commands: cd, clr, dir, environ, echo, help, pause, quit
- external commands: fork + exec
- redirection: <, >, >> (stdout redirection also for internal: dir/environ/echo/help)
- background: '&' at end => do not wait
- batch input: myshell batchfile (EOF => exit)
- env:
    shell=<fullpath>/myshell  (set in main)
    parent=<fullpath>/myshell (set in child before exec)
- prompt includes current directory

*/

#include "myshell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LINE 1024

static void reap_zombies(void) {
    //Cleans up all potential zombie processes using a while loop
    while (waitpid(-1, NULL, WNOHANG) > 0){//Reaping zombies
        }
}

// Processes a single line of input, executing the appropriate command based on the first token
static int process_line(char *line) {
    char *args[MAX_ARGS];
    // Tokenize the input line into arguments
    tokenize(line, args);
    // Check for i/o redirection and handle it
    if (check_redirection(args) == -1) {
        return 1;
    }
    // If no command is entered, continue
    if (args[0] == NULL){
        return 1;
    }
    // Handle clearing of the screen
    else if (strcmp(args[0], "clr") == 0){
        execute_clear();
    }    
    // Handle quitting the shell
    else if (strcmp(args[0], "quit") == 0){
        return 0;
    }
    // Handle changing directories
    else if (strcmp(args[0], "cd") == 0) {
        execute_cd(args);
    }
    // Handle pausing the shell
    else if (strcmp(args[0], "pause") == 0){
        execute_pause();
    }
    // Handle listing directory contents
    else if (strcmp(args[0], "dir") == 0) {
        execute_dir(args);
    }
    // Handle printing environment variables
    else if (strcmp(args[0], "environ") == 0){
        execute_environ();
    }
    // Handle echoing arguments
    else if (strcmp(args[0], "echo") == 0){
        execute_echo(args);
    }
    // Handle displaying help information
    else if (strcmp(args[0], "help") == 0){
        execute_help();
    }
    // If the command is not an internal command, treat as external command
    else {
        int background = 0;
        int last_arg = 0;
        while (args[last_arg] != NULL) last_arg++;
        
        if (last_arg > 0 && strcmp(args[last_arg-1], "&") == 0) {
            // Set flag to indicate it is a background process
            background = 1;
            // Remove the '&' from the arguments
            args[last_arg-1] = NULL;
        }
        // Execute as external command with appropriate background flag
        execute_external_command(args, background);
    }
    // Continue the shell loop
    return 1; 
}

int main(int argc, char *argv[]) {
        
    // Set environment variable "shell" to the full path of myshell 
    char actual_path[PATH_MAX];
    if (realpath(argv[0], actual_path) != NULL) {
        setenv("shell", actual_path, 1);
    }

    FILE *in = stdin;
    // Flag to indicate if we are running in batch mode
    int is_batch_mode = 0;

    if (argc == 2) {
        in = fopen(argv[1], "r");
        if (!in) { perror("fopen"); return 1; }
        // Set batch mode flag to indicate we're running in batch mode
        is_batch_mode = 1;
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [batchfile]\n", argv[0]);
        return 1;
    }

    char line[MAX_LINE];
    char cwd[PATH_MAX];

    int saved_stdout = dup(STDOUT_FILENO);
    int saved_stdin = dup(STDIN_FILENO);

    while (1) {
        // Reap any zombie processes before displaying the prompt or processing the next command
        reap_zombies();
        
        if (!is_batch_mode) {
            if (getcwd(cwd, sizeof(cwd)) == NULL) { perror("getcwd"); break; }
            // Flush stdout and stdin to ensure the prompt is displayed in the terminal and input is read properly
            fflush(stdout);
            fflush(stdin);
            // Restore original stdout and stdin if i/o redirection was used
            dup2(saved_stdout, STDOUT_FILENO);
            dup2(saved_stdin, STDIN_FILENO);

            printf(">myshell:%s$ ", cwd);
            fflush(stdout);
        }

        if (!fgets(line, sizeof(line), in)) break; // EOF => exit (batch requirement)

        if (process_line(line) == 0) break;
    }

    // Close the saved file descriptors for stdout and stdin before exiting
    close(saved_stdout);
    close(saved_stdin);

    if (is_batch_mode) fclose(in);
    return 0;
}