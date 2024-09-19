#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
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
    using_history();

    while ((line = readline("$ ")) != NULL) {
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
    return 0;
}