#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include "../src/lab.h"

int main(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "vV")) != -1) {
        switch (opt) {
            case 'V':
            case 'v':
                printf("Version %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
                exit(0);
            default:
                fprintf(stderr, "Usage: %s [-v|-V]\n", argv[0]);
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
            fprintf(stderr, "Failed to set prompt\n");
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
                    if (change_dir(&args[1]) == 0) {
                        printf("Directory changed successfully\n");
                    }
                } else if (strcmp(args[0], "exit") == 0) {
                    cmd_free(args);
                    free(line);
                    break;
                } else if (strncmp(args[0], "MY_PROMPT=", 10) == 0) {
                    // Handle the case where the new prompt might include spaces
                    char new_prompt[256] = {0};  // Adjust size as needed
                    int i = 1;
                    strncpy(new_prompt, args[0] + 10, sizeof(new_prompt) - 1);
                    while (args[i] != NULL && i < 10) {  // Limit to prevent buffer overflow
                        strncat(new_prompt, " ", sizeof(new_prompt) - strlen(new_prompt) - 1);
                        strncat(new_prompt, args[i], sizeof(new_prompt) - strlen(new_prompt) - 1);
                        i++;
                    }
                    if (set_prompt(new_prompt) == 0) {
                        printf("Prompt updated successfully\n");
                    }
                } else {
                    printf("Command entered: %s\n", args[0]);
                    // Here you would typically fork and exec the command
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