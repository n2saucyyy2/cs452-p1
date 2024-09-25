/**
 * @file lab.c
 * @brief Implementation of shell utility functions
 *
 * This file contains the implementation of various utility functions
 * used by the custom shell. Key functions include:
 *
 * - Prompt management (get_prompt, set_prompt)
 * - Directory changing (change_dir)
 * - Command parsing (cmd_parse, cmd_free)
 * - String manipulation (trim_white)
 * - Command history management (print_history)
 * - Background process handling (start_background_process, check_background_processes)
 * - Shell initialization and cleanup (sh_init, sh_destroy)
 * - Job control (print_jobs)
 *
 * These functions provide core functionality for the shell implemented in main.c.
 *
 * @author nolanstetz
 * @date 25th of September 2024
 */
#include "lab.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <linux/limits.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <bits/waitflags.h>
#include <termios.h>
#include <signal.h>

#define PROMPT_OK 0
#define PROMPT_MISSING_END_QUOTE 1
#define PROMPT_MISSING_START_QUOTE 2
#define PROMPT_OTHER_ERROR 3

char *get_prompt(const char *env) {
    //get the prompt value from the env variable
    const char *prompt_value = getenv(env);
    const char *default_prompt = "shell$ ";
    //use default prompt if nothing given
    if (prompt_value == NULL || strlen(prompt_value) == 0) {
        prompt_value = default_prompt;
    }
    //allocate memory for prompt
    char *prompt = malloc(strlen(prompt_value) + 1);
    if (prompt == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    
    strcpy(prompt, prompt_value);
    return prompt;
}

int set_prompt(const char *new_prompt) {
    if (new_prompt == NULL || *new_prompt == '\0') {
        fprintf(stderr, "Error: custom prompt was not entered correctly\n");
        fprintf(stderr, "USAGE: MY_PROMPT=\"xxxx\"\n");
        return PROMPT_OTHER_ERROR;
    }

    // Don't check for quotes if it comes from the command-line argument
    if (strchr(new_prompt, '"') == NULL) {
        // Directly set the environment variable with the raw prompt
        if (setenv("MY_PROMPT", new_prompt, 1) != 0) {
            perror("Failed to set MY_PROMPT");
            return PROMPT_OTHER_ERROR;
        }
        return PROMPT_OK;
    }

    // Handle prompt from environment variable (which might have quotes)
    size_t len = strlen(new_prompt);
    if (new_prompt[0] != '"' || new_prompt[len - 1] != '"') {
        fprintf(stderr, "Error: Prompt must be enclosed in quotes\n");
        return PROMPT_OTHER_ERROR;
    }

    // Remove the quotes and set the environment variable
    char *trimmed_prompt = strndup(new_prompt + 1, len - 2);
    if (trimmed_prompt == NULL) {
        perror("Failed to allocate memory for new prompt");
        return PROMPT_OTHER_ERROR;
    }

    if (setenv("MY_PROMPT", trimmed_prompt, 1) != 0) {
        perror("Failed to set MY_PROMPT");
        free(trimmed_prompt);
        return PROMPT_OTHER_ERROR;
    }

    free(trimmed_prompt);
    return PROMPT_OK;
}


int change_dir(char **dir) {
    const char *new_dir;
    
    if (dir == NULL || *dir == NULL) {
        // No argument provided, change to home directory
        new_dir = getenv("HOME");
        if (new_dir == NULL) {
            // If HOME is not set, fall back to getpwuid
            struct passwd *pw = getpwuid(getuid());
            if (pw == NULL) {
                perror("Failed to get home directory");
                return -1;
            }
            new_dir = pw->pw_dir;
        }
    } else {
        new_dir = *dir;
    }

    if (chdir(new_dir) != 0) {
        perror("cd failed");
        return -1;
    }
    
    // Print current working directory after successful change
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current directory: %s\n", cwd);
    } else {
        perror("getcwd() error");
    }
    
    return 0;
}

char **cmd_parse(char const *line) {
    //get max num of arguments allowed and allocate memory for args array
    long arg_max = sysconf(_SC_ARG_MAX);
    char **args = malloc((arg_max + 1) * sizeof(char *));
    if (args == NULL) {
        perror("malloc failed");
        return NULL;
    }

    char *token;
    char *line_copy = strdup(line);
    int i = 0;
    //tokenize the line and store each token in the args array
    token = strtok(line_copy, " \t\n");
    while (token != NULL && i < arg_max) {
        args[i] = strdup(token);
        if (args[i] == NULL) {
            perror("strdup failed");
            cmd_free(args);
            free(line_copy);
            return NULL;
        }
        i++;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;

    free(line_copy);
    return args;
}

void cmd_free(char **line) {
    if (line == NULL) return;
    //free each argument string
    for (int i = 0; line[i] != NULL; i++) {
        free(line[i]);
    }
    free(line);
}

char *trim_white(char *line) {
    if (line == NULL) return NULL;

    // Trim leading whitespace
    while (isspace((unsigned char)*line)) line++;

    if (*line == 0) return line;

    // Trim trailing whitespace
    char *end = line + strlen(line) - 1;
    while (end > line && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    return line;
}

int print_history(int limit) {
    //get the history list
    HIST_ENTRY **hist_list;
    int hist_length, i, start;

    hist_list = history_list();
    if (hist_list == NULL) {
        fprintf(stderr, "Failed to retrieve history\n");
        return -1;
    }

    hist_length = history_length;
    
    if (limit <= 0 || limit > hist_length) {
        start = 0;
    } else {
        start = hist_length - limit;
    }

    for (i = start; i < hist_length; i++) {
        printf("%d: %s\n", i + 1, hist_list[i]->line);
    }

    return 0;
}

int start_background_process(struct shell *sh, char **args, char *full_command) {
    if (sh->bg_job_count >= MAX_BG_JOBS) {
        fprintf(stderr, "Maximum number of background jobs reached\n");
        return -1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return -1;
    } else if (pid == 0) {
        // Child process
        setpgid(0, 0);
        if (execvp(args[0], args) == -1) {
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        int job_id = sh->next_job_id++;
        sh->bg_jobs[sh->bg_job_count].job_id = job_id;
        sh->bg_jobs[sh->bg_job_count].pid = pid;
        sh->bg_jobs[sh->bg_job_count].command = strdup(full_command);
        sh->bg_jobs[sh->bg_job_count].status = 0; // 0 for Running
        sh->bg_job_count++;

        printf("[%d] %d\n", job_id, pid);
    }

    return 0;
}

void check_background_processes(struct shell *sh) {
    for (int i = 0; i < sh->bg_job_count; i++) {
        int status;
        pid_t result = waitpid(sh->bg_jobs[i].pid, &status, WNOHANG);

        if (result > 0) {
            // Process has finished
            sh->bg_jobs[i].status = 1; // 1 for Done
        }
    }
}

void sh_init(struct shell *sh) {
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty(sh->shell_terminal);

    if (sh->shell_is_interactive) {
        // Loop until we are in the foreground
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp()))
            kill(-sh->shell_pgid, SIGTTIN);

        // Ignore interactive and job-control signals
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);

        // Put ourselves in our own process group
        sh->shell_pgid = getpid();
        if (setpgid(sh->shell_pgid, sh->shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group");
            exit(1);
        }

        // Grab control of the terminal
        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);

        // Save default terminal attributes for shell
        tcgetattr(sh->shell_terminal, &sh->shell_tmodes);
    }

    sh->bg_job_count = 0;
    sh->next_job_id = 1; // Initialize next_job_id
    sh->prompt = get_prompt("MY_PROMPT");
}

void sh_destroy(struct shell *sh) {
    if (sh->prompt) {
        free(sh->prompt);
    }
    //free command strings for each background job
    for (int i = 0; i < sh->bg_job_count; i++) {
        if (sh->bg_jobs[i].command) {
            free(sh->bg_jobs[i].command);
        }
    }
}

void print_jobs(struct shell *sh) {
    for (int i = 0; i < sh->bg_job_count; i++) {
        struct bg_job *job = &sh->bg_jobs[i];
        //determine status
        const char *status = (job->status == 0) ? "Running" : "Done   ";
        printf("[%d] %d %s %s\n", job->job_id, job->pid, status, job->command);
    }
}