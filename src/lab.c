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

#define PROMPT_OK 0
#define PROMPT_MISSING_END_QUOTE 1
#define PROMPT_MISSING_START_QUOTE 2
#define PROMPT_OTHER_ERROR 3

char *get_prompt(const char *env) {
    const char *prompt_value = getenv(env);
    const char *default_prompt = "shell$ ";
    
    if (prompt_value == NULL || strlen(prompt_value) == 0) {
        prompt_value = default_prompt;
    }
    
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
    long arg_max = sysconf(_SC_ARG_MAX);
    char **args = malloc((arg_max + 1) * sizeof(char *));
    if (args == NULL) {
        perror("malloc failed");
        return NULL;
    }

    char *token;
    char *line_copy = strdup(line);
    int i = 0;

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