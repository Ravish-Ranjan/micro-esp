#include <stdio.h>
#include <string.h>
#include "commands.h"

#define MAX_ARGS 10

extern command_t commands[];

void execute_command(char *input) {
    char *argv[MAX_ARGS];
    int argc = 0;

    char *token = strtok(input, " ");

    while (token && argc < MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    if (argc == 0)
        return;

    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].func(argc, argv);
            return;
        }
    }

    printf("Unknown command\n");
}