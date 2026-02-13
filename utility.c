/**
 * File: utility.c
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
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * tokenize
 * --------
 * Split the input string into whitespace-separated tokens and
 * populate the provided `args` array. This function modifies
 * `input` in-place by inserting NUL terminators and sets the
 * final `args` element to NULL (suitable for `execvp`).
 *
 * Parameters:
 *  - input: writable NUL-terminated command line
 *  - args: array of char* sized at least MAX_ARGS
 */
void tokenize (char *input, char **args){
    // Get the first token from the input string
    char *token = strtok(input, " \t\n");
    int index = 0;

    // Extracting tokens until we hit the end or run out of space
    while (token != NULL && index < MAX_ARGS - 1) {
        // Store the pointer to the start of the word
        args[index++] = token;
        // NULL tells strtok to continue from where it left off
        token = strtok(NULL, " \t\n");
    }
    // execvp requires the argument list to be terminated by a NULL pointer
    args[index] = NULL;
}

/**
 * execute_clear
 * --------------
 * Clear the terminal screen using ANSI escape codes and flush
 * stdout so the caller sees the effect immediately.
 */
void execute_clear(){
    // Send ANSI escape sequences: \033[H (home) and \033[2J (clear screen) and \033[3J (clear scrollback)
    printf("\033[H\033[2J\033[3J");
    // Ensure the terminal updates immediately without waiting for a newline
    fflush(stdout);
}

/**
 * execute_cd
 * ----------
 * Change the current working directory. If no argument is
 * supplied, print the current working directory. Updates the
 * `PWD` environment variable on success.
 *
 * Parameters:
 *  - args: argv-style array where args[1] (if present) is the
 *    target directory.
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
 * execute_pause
 * -------------
 * Pause the shell until the user presses Enter. This reads from
 * stdin until a newline is seen.
 */
void execute_pause(){
    // Print a notification to the user to tell them the shell is paused
    printf("Shell paused. Press Enter to continue...");
    // Flush stdout to ensure the message is printed before waiting for input
    fflush(stdout);
    // Loop until the character read from the keyboard is a newline
    while (getchar() != '\n');
}

/**
 * execute_environ
 * ---------------
 * Print all environment variables available to the process.
 * Note: currently prints directly to stdout; redirection of
 * stdout is handled elsewhere by `check_redirection` when
 * executing external commands.
 */
void execute_environ(){
    int i = 0;
    // Access the global variable 'environ' defined in unistd.h
    extern char **environ;
    // Iterate through the array of strings until we hit the NULL terminator
    while (environ[i] != NULL) {
        printf("%s\n", environ[i]);
        i++;
    }
}

/**
 * execute_dir
 * -----------
 * List files in the directory specified by args[1], or the
 * current directory if none is provided. Prints one name per
 * line using `readdir`.
 */
void execute_dir(char **args){
    char *target;
    // Default to current directory "." if no path is provided
    if (args[1] == NULL) {
        target = ".";
    } else {
        target = args[1];
    }

    // Open the directory stream
    DIR *dir_ptr = opendir(target);
    if (dir_ptr == NULL) {
        perror("Error opening directory");
        return;
    }
    struct dirent *entry;
    // Read each entry (file/folder) and print its name
    while ((entry = readdir(dir_ptr)) != NULL) {
        printf("%s\n", entry->d_name);
    }
    // Close the stream to avoid file descriptor leaks
    closedir(dir_ptr);
}

/**
 * execute_echo
 * ------------
 * Print the arguments following `echo` separated by spaces. Stops
 * printing when it encounters a redirection operator so that a
 * command like `echo hi > out` will print only `hi` and allow the
 * redirection to be processed separately.
 */
void execute_echo(char **args){
    // Start at 1 to skip the "echo" itself
    int i = 1; 

    while (args[i] != NULL) {
        // If we see a redirection symbol, stop printing (redirection is handled elsewhere)
        if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], "<") == 0) {
            break;
        }

        // Print the current argument or the echoed text
        printf("%s", args[i]);

        // Add a space between words, but don't add one after the last word
        if (args[i + 1] != NULL && strcmp(args[i+1], ">") != 0 && strcmp(args[i+1], ">>") != 0) {
            printf(" ");
        }
        i++;
    }
    printf("\n");
    fflush(stdout);
}

/**
 * execute_help
 * ------------
 * Display the bundled `readme` file using the system `more`
 * pager via `execute_external_command`. If the `readme` file is
 * missing, print a friendly error message.
 */
void execute_help(){
    // Check if the readme file exists in the current directory
    if (access("readme", F_OK) == -1) {
        printf("Error: 'readme' manual not found.\n");
        return;
    }
    // Prepare arguments for the 'more' command to display the file page-by-page
    char *more_args[] = {"more", "readme", NULL};
    // Execute the 'more' command, not as a background process
    execute_external_command(more_args, 0);
}

/**
 * Helper to check for redirection tokens in the argument list.
 * This function modifies the args array by setting the redirection 
 * symbol to NULL, effectively truncating the arguments for execvp.
 * Returns 1 if redirection was found and handled, 0 otherwise, -1 on error.
 */
/* Redirect handling helpers
 * ------------------------
 * The parsing/apply functions below allow `check_redirection` to
 * detect multiple redirection operators in the argument list and
 * apply them in order. Notes:
 * - For stdout redirection, later redirects will overwrite earlier
 *   ones because `dup2` updates `STDOUT_FILENO` each time.
 * - `parse_redirects` allocates copies of filenames with `strdup`
 *   and therefore callers must free them via `free_redirects`.
 */
// Maximum number of redirections we will handle in a single command line
#define MAX_REDIRECTS 16

// Types of redirection we support in an enum
typedef enum { REDIR_OUT_TRUNC, REDIR_OUT_APPEND, REDIR_IN } redirect_type_t;

// Struct to hold information about a single redirection
typedef struct {
    redirect_type_t type;
    char *filename;
} redirect_t;

/* parse_redirects
 * ----------------
 * Scan `args` for redirection tokens (`>`, `>>`, `<`) and record
 * the operator and its filename into `redirects`. Returns -1 on
 * syntax errors (e.g. missing filename) or 0 on success. `count`
 * returns the number of redirects parsed.
 */
static int parse_redirects(char **args, redirect_t *redirects, int *count) {
    // Initialize count to 0 before parsing
    int i = 0;
    *count = 0;

    while (args[i] != NULL) {
        // Check if the current argument is one of the three redirection operators
        if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], "<") == 0) {
            // If the symbol is the last thing in the command, it's a syntax error
            if (args[i + 1] == NULL) {
                fprintf(stderr, "myshell: syntax error near unexpected token '%s'\n", args[i]);
                return -1;
            }
            // Check against our internal limit for number of redirects
            if (*count >= MAX_REDIRECTS) {
                fprintf(stderr, "myshell: too many redirections\n");
                return -1;
            }

            // Assign the correct type based on the symbol
            if (strcmp(args[i], ">") == 0) {
                // Overwrite/Truncate the file
                redirects[*count].type = REDIR_OUT_TRUNC;
            } else if (strcmp(args[i], ">>") == 0) {
                // Append to file
                redirects[*count].type = REDIR_OUT_APPEND;
            } else {
                // Read from file
                redirects[*count].type = REDIR_IN;
            }
            // Duplicate the filename string
            redirects[*count].filename = strdup(args[i + 1]);
            // Check if strdup succeeded
            if (redirects[*count].filename == NULL) {
                perror("myshell: strdup");
                return -1;
            }
            (*count)++;
            // Move past both the symbol and the filename
            i += 2;
            continue;
        }
        i++;
    }
    return 0;
}

/* apply_redirects
 * ---------------
 * Given a parsed list of `redirects`, open the target files and
 * duplicate their file descriptors onto stdin/stdout. Returns -1
 * on errors and 0 on success. Each open's fd is closed after
 * duplicating with `dup2`.
 */
static int apply_redirects(redirect_t *redirects, int count) {
    for (int i = 0; i < count; i++) {
        int fd;
        // Open file for Overwriting (Truncate)
        if (redirects[i].type == REDIR_OUT_TRUNC) {
            // Open the file for writing (O_WRONLY), create it if it doesn't exist (O_CREAT), and truncate it if it does (O_TRUNC)
            fd = open(redirects[i].filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            // Check if the file was opened successfully
            if (fd < 0) { perror("myshell: open"); return -1; }
            // Replace STDOUT with our file descriptor while checking for errors
            if (dup2(fd, STDOUT_FILENO) < 0) { 
                perror("myshell: dup2"); 
                close(fd); 
                return -1; 
            }
            close(fd);
        // Open file for Appending
        } else if (redirects[i].type == REDIR_OUT_APPEND) {
            // Open the file for writing (O_WRONLY), create it if it doesn't exist (O_CREAT), and append to it if it does (O_APPEND)
            fd = open(redirects[i].filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) { perror("myshell: open"); return -1; }
            // Replace STDOUT with our file descriptor while checking for errors
            if (dup2(fd, STDOUT_FILENO) < 0) { 
                perror("myshell: dup2"); 
                close(fd); 
                return -1; 
            }
            close(fd);
        // Open file for Reading
        } else if (redirects[i].type == REDIR_IN) {
            // Open the file for reading only (O_RDONLY)
            fd = open(redirects[i].filename, O_RDONLY);
            if (fd < 0) { perror("myshell: open"); return -1; }
            // Replace STDIN with our file descriptor while checking for errors
            if (dup2(fd, STDIN_FILENO) < 0) { 
                perror("myshell: dup2"); 
                close(fd); 
                return -1; 
            }
            // Close the file descriptor after duplicating
            close(fd);
        }
    }
    return 0;
}

/* free_redirects
 * --------------
 * Free any memory allocated by `parse_redirects`.
 */
static void free_redirects(redirect_t *redirects, int count) {
    // Loop through the redirects and free the duplicated filename strings
    for (int i = 0; i < count; i++) {
        free(redirects[i].filename);
        redirects[i].filename = NULL;
    }
}

/* remove_redirect_tokens
 * ----------------------
 * Remove redirection operators and their filenames from the
 * argument array so that `execvp` sees only the command and its
 * true arguments.
 */
static void remove_redirect_tokens(char **args) {
    int i = 0, j = 0;
    while (args[i] != NULL) {
        // If we find a symbol, we skip both the symbol and the filename that follows it
        if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], "<") == 0) {
            // Skip operator and filename
            i += 2;
            continue;
        }
        // Otherwise, "slide" the valid arguments down to the front of the array
        args[j++] = args[i++];
    }
    // NULL terminate the cleaned-up list
    args[j] = NULL;
}

/* check_redirection
 * -----------------
 * Check the `args` array for redirection operators and apply them if found. Returns 1 if redirection was found 
 * and handled, 0 if no redirection was found, and -1 on syntax or file open errors. This function modifies `args`
 * by removing the redirection tokens and their filenames, leaving only the 
 * command and its true arguments (suitable for `execvp`). 
 */
int check_redirection(char **args) {
    // Array to hold parsed redirection information and a count of how many we find
    redirect_t redirects[MAX_REDIRECTS];
    int count = 0;

    // Parse the args for redirection tokens and fill the redirects array
    if (parse_redirects(args, redirects, &count) == -1) {
        free_redirects(redirects, count);
        return -1;
    }

    // If no redirection tokens were found, we can return early without modifying args
    if (count == 0) return 0;

    // Apply the parsed redirections by opening files and duplicating fds
    if (apply_redirects(redirects, count) == -1) {
        // Free redirects before returning on error
        free_redirects(redirects, count);
        return -1;
    }

    // Remove tokens
    remove_redirect_tokens(args);

    // Free any memory allocated for filenames in the redirects array
    free_redirects(redirects, count);
    return 1;
}

/* execute_external_command
 * ------------------------
 * Forks a child process to execute an external command specified by
 * `args`. If `is_background` is nonzero, the parent does not wait
 * for the child to finish before returning.
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

        // Execute the external command with the given arguments. If execvp returns, it means there was an error.
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