#include "lab.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

int change_dir(char **dir) {
    const char *new_dir;
    if (dir == NULL || *dir == NULL) {
        new_dir = getenv("HOME");
        if (new_dir == NULL) {
            fprintf(stderr, "Home directory not set\n");
            return -1;
        }
    } else {
        new_dir = *dir;
    }

    if (chdir(new_dir) != 0) {
        perror("chdir failed");
        return -1;
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