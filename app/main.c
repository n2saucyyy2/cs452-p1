#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include "../src/lab.h"

#define PROMPT_OK 0
#define PROMPT_MISSING_END_QUOTE 1
#define PROMPT_MISSING_START_QUOTE 2
#define PROMPT_OTHER_ERROR 3

struct termios shell_tmodes;
int shell_terminal;
pid_t shell_pgid;

void init_shell() {
    // See if we are running interactively
    shell_terminal = STDIN_FILENO;
    int shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive) {
        // Loop until we are in the foreground
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
            kill(-shell_pgid, SIGTTIN);

        // Ignore interactive and job-control signals
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);

        // Put ourselves in our own process group
        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group");
            exit(1);
        }

        // Grab control of the terminal
        tcsetpgrp(shell_terminal, shell_pgid);

        // Save default terminal attributes for shell
        tcgetattr(shell_terminal, &shell_tmodes);
    }
}

int execute_command(char **args) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return -1;
    } else if (pid == 0) {
        // Child process
        pid_t child = getpid();
        setpgid(child, child);
        tcsetpgrp(shell_terminal, child);
        
        // Reset signal handlers
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        if (execvp(args[0], args) == -1) {
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        int status;
        setpgid(pid, pid);
        tcsetpgrp(shell_terminal, pid);

        if (waitpid(pid, &status, WUNTRACED) == -1) {
            perror("waitpid failed");
            return -1;
        }

        // Put shell back in foreground
        tcsetpgrp(shell_terminal, shell_pgid);

        return WEXITSTATUS(status);
    }
    return 0;
}


int main(int argc, char *argv[])
{
    int opt;
    char *custom_prompt = NULL;

    // Initialize shell
    init_shell();

    // Set default prompt
    if (setenv("MY_PROMPT", "shell$ ", 1 && custom_prompt == NULL) != 0) {
        perror("Failed to set default prompt");
    }

   // Process command-line arguments
for (int i = 1; i < argc; i++) {
    if (strncmp(argv[i], "MY_PROMPT=", 10) == 0) {
        char *custom_prompt = argv[i] + 10;  // Skip "MY_PROMPT="
        int prompt_result = set_prompt(custom_prompt);  // Pass raw string, no quotes check needed
        if (prompt_result != PROMPT_OK) {
            exit(1);  // Exit if prompt setting fails
        }
        break;
    }
}

    // Process other command-line options
    while ((opt = getopt(argc, argv, "vV")) != -1) {
        switch (opt) {
            case 'V':
            case 'v':
                printf("Version %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
                exit(0);
            default:
                fprintf(stderr, "Usage: %s [-v|-V] [MY_PROMPT=<prompt>]\n", argv[0]);
                exit(1);
        }
    }

    char *line;
    char **args;
    char *prompt;
    int status = 0;
    using_history();

    while (1) {
        // Ensure the shell is in the foreground
        tcsetpgrp(shell_terminal, shell_pgid);

        prompt = get_prompt("MY_PROMPT");
        if (prompt == NULL) {
            fprintf(stderr, "Failed to get prompt, using default\n");
            prompt = strdup("shell$ ");
            if (prompt == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                status = 1;
                break;
            }
        }

        line = readline(prompt);
        free(prompt);

        if (line == NULL) {
            // EOF (Ctrl-D) detected
            printf("\n");
            break;
        }

        if (strlen(line) > 0) {
            add_history(line);
            line = trim_white(line);
            args = cmd_parse(line);

            if (args != NULL) {
                if (strcmp(args[0], "cd") == 0) {
                    if (change_dir(args[1] ? &args[1] : NULL) != 0) {
                        fprintf(stderr, "Failed to change directory\n");
                    }
                } else if (strcmp(args[0], "exit") == 0) {
                    cmd_free(args);
                    free(line);
                    break;
                } else if (strcmp(args[0], "history") == 0) {
                    int limit = 0;
                    if (args[1] != NULL) {
                        limit = atoi(args[1]);
                    }
                    if (print_history(limit) != 0) {
                        fprintf(stderr, "Failed to print history\n");
                    }
                } if (strncmp(args[0], "MY_PROMPT=", 10) == 0) {
                    char *new_prompt = args[0] + 10;  // Skip "MY_PROMPT="
                    int prompt_result = set_prompt(new_prompt);
                     if (prompt_result != PROMPT_OK) {
                         // Error message is already printed in set_prompt
                        continue;  // Continue the shell loop
            }
            printf("Prompt updated successfully.\n");
        } else {
                    // Execute the command
                    if (execute_command(args) != 0) {
                        fprintf(stderr, "Command execution failed\n");
                    }
                }
                cmd_free(args);
            }
        }
        free(line);
    }

    printf("Exiting shell\n");
    rl_clear_history();
    return status;
}