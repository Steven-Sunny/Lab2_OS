/*
 * File: myshell.c
 * Name: Steven Sun
 * Student Number: 100816207
 * Work Group: 29
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

/*
 * reap_zombies
 * ----------------
 * Reaps any finished child processes (zombies) without blocking.
 * Uses `waitpid` with `WNOHANG` in a loop to clean up all children
 * that have exited since the last check. This allows the shell to
 * avoid leaking process entries while remaining responsive.
 */
static void reap_zombies(void) {
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        /* loop until no more exited children */
    }
}

// Processes a single line of input, executing the appropriate command based on the first token
/*
 * process_line
 * ------------
 * Parse a single input `line`, execute built-in commands or
 * invoke external commands. Returns 1 to continue the shell loop
 * or 0 to exit the shell (e.g., when `quit` is entered).
 *
 * Behavior notes:
 * - Tokenization is performed in-place by `tokenize`, which
 *   modifies `line` and fills `args` with pointers into `line`.
 * - `check_redirection` is called before built-ins; it applies
 *   any I/O redirections for the current process (used by child
 *   when executing external commands). On syntax errors it
 *   returns -1, causing the caller to continue.
 */
static int process_line(char *line) {
    // Tokenize the input line into arguments
    char *args[MAX_ARGS];
    tokenize(line, args);

    // Check for I/O redirection tokens and apply them if present
    if (check_redirection(args) == -1) {
        // Syntax or open error; keep shell running
        return 1; 
    }
    // Handle empty input
    if (args[0] == NULL) {
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
        /*
         * External command path. Detect a trailing `&` to run the
         * command in the background (parent will not wait).
         */
        int background = 0;
        int last_arg = 0;

        // Find the index of the last argument to check for `&`
        while (args[last_arg] != NULL) last_arg++;

        // If the last argument is `&`, set background flag and remove it from args
        if (last_arg > 0 && strcmp(args[last_arg - 1], "&") == 0) {
            background = 1; 
            // Drop the ampersand from argv
            args[last_arg - 1] = NULL;
        }
        // Execute the external command with the given arguments and background flag
        execute_external_command(args, background);
    }
    // Continue the shell loop
    return 1; 
}

int main(int argc, char *argv[]) {
        
    // Set environment variable "shell" to the full path of myshell 
    char actual_path[PATH_MAX];
    // If realpath succeeds, set the "shell" environment variable to the resolved path of the executable
    if (realpath(argv[0], actual_path) != NULL) {
        setenv("shell", actual_path, 1);
    }

    FILE *in = stdin;
    // Flag to indicate if we are running in batch mode
    int is_batch_mode = 0;
    // If a batch file is provided as an argument, open it for reading
    if (argc == 2) {
        in = fopen(argv[1], "r");
        // Error opening batch file
        if (!in) { perror("fopen"); return 1; }
        // Set batch mode flag to indicate we're running in batch mode
        is_batch_mode = 1;
    // Else if more than one argument is provided, print usage and exit
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [batchfile]\n", argv[0]);
        return 1;
    }
    // Prepare to read lines of input and current working directory
    char line[MAX_LINE];
    char cwd[PATH_MAX];

    // Save original stdout and stdin file descriptors to restore later
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
            // Display the prompt with the current working directory
            printf(">myshell:%s$ ", cwd);
            fflush(stdout);
        }
        // Read a line of input
        if (!fgets(line, sizeof(line), in)) break; // EOF => exit (batch requirement)
        // Process the input line; if it returns 0, exit the shell
        if (process_line(line) == 0) break;
    }

    // Close the saved file descriptors for stdout and stdin before exiting
    close(saved_stdout);
    close(saved_stdin);
    // If we opened a batch file, close it before exiting
    if (is_batch_mode) fclose(in);
    return 0;
}