// //fix this $ ./myprogram MY_PROMPT=hello?"
// > f'
// > fd
// > bash: unexpected EOF while looking for matching `"'
// bash: syntax error: unexpected end of file
//SO THAT IT THROWS BEFORE CHANGING THE PROMPT

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include "../src/lab.h"

#define PROMPT_OK 0
#define PROMPT_MISSING_END_QUOTE 1
#define PROMPT_MISSING_START_QUOTE 2
#define PROMPT_OTHER_ERROR 3

int main(int argc, char *argv[])
{
    int opt;
    char *custom_prompt = NULL;

    // Process command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "MY_PROMPT=", 10) == 0) {
            custom_prompt = argv[i] + 10;  // Skip "MY_PROMPT="
            break;
        }
    }

    // If a custom prompt was provided, try to set it
    if (custom_prompt != NULL) {
        int prompt_result = set_prompt(custom_prompt);
        switch (prompt_result) {
            case PROMPT_OK:
                break;
            case PROMPT_MISSING_END_QUOTE:
                fprintf(stderr, "Error: Missing ending quotation mark in prompt\n");
                exit(1);
            case PROMPT_MISSING_START_QUOTE:
                fprintf(stderr, "Error: Missing beginning quotation mark in prompt\n");
                exit(1);
            case PROMPT_OTHER_ERROR:
                fprintf(stderr, "Error: Failed to set custom prompt\n");
                exit(1);
            default:
                fprintf(stderr, "Unknown error while setting prompt\n");
                exit(1);
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
        prompt = get_prompt("MY_PROMPT");
        if (prompt == NULL) {
            fprintf(stderr, "Failed to get prompt\n");
            status = 1;
            break;
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
                } else if (strncmp(args[0], "MY_PROMPT=", 10) == 0) {
                    char *new_prompt = args[0] + 10;  // Skip "MY_PROMPT="
                    int prompt_result = set_prompt(new_prompt);
                    switch (prompt_result) {
                        case PROMPT_OK:
                            break;
                        case PROMPT_MISSING_END_QUOTE:
                            fprintf(stderr, "Error: Missing ending quotation mark in prompt\n");
                            break;
                        case PROMPT_MISSING_START_QUOTE:
                            fprintf(stderr, "Error: Missing beginning quotation mark in prompt\n");
                            break;
                        case PROMPT_OTHER_ERROR:
                        default:
                            fprintf(stderr, "Error: Failed to set new prompt\n");
                            break;
                    }
                } else {
                    printf("Command entered: %s\n", args[0]);                
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