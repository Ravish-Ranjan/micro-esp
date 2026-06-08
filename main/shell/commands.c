#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include "../fs/fs.h"
#include "commands.h"
#include "driver/uart.h"

#define UART_NUM UART_NUM_0
#define BUF_SIZE 128

extern int shell_running;

void cmd_ls(int argc, char **argv) {
    fs_list();
}

void cmd_cat(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: cat <file>\n");
        return;
    }

    char path[128];
    snprintf(path, sizeof(path), "/spiffs/%s", argv[1]);

    FILE *f = fopen(path, "r");
    if (!f) {
        printf("File not found\n");
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }

    fclose(f);
}

void cmd_write(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: write <file> <text>\n");
        return;
    }

    char path[128];
    snprintf(path, sizeof(path), "/spiffs/%s", argv[1]);

    printf("Writing to: %s\n", path);

    FILE *f = fopen(path, "w");
    if (!f) {
        printf("fopen failed: errno=%d\n", errno);
        return;
    }

    for (int i = 2; i < argc; i++) {
        fprintf(f, "%s ", argv[i]);
    }

    fprintf(f, "\n");

    fflush(f);
    fclose(f);

    printf("Write complete\n");
}

void cmd_rm(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: rm <file>\n");
        return;
    }

    char path[128];
    snprintf(path, sizeof(path), "/spiffs/%s", argv[1]);

    remove(path);
}

void cmd_exit(int argc, char **argv) {
    printf("Exiting shell...\n");
    shell_running = 0;
}

void cmd_edit(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: edit [-a|-n] <file>\n");
        printf("  -a: append mode (default)\n");
        printf("  -n: new file mode (truncate)\n");
        return;
    }

    int arg_idx = 1;
    const char *mode = "a+";

    if (argv[1][0] == '-') {
        if (strcmp(argv[1], "-a") == 0) {
            mode = "a+";
            arg_idx = 2;
        } else if (strcmp(argv[1], "-n") == 0) {
            mode = "w+";
            arg_idx = 2;
        } else {
            printf("Unknown flag: %s\n", argv[1]);
            return;
        }
    }

    if (arg_idx >= argc) {
        printf("Usage: edit [-a|-n] <file>\n");
        return;
    }

    char path[128];
    snprintf(path, sizeof(path), "/spiffs/%s", argv[arg_idx]);

    FILE *f = fopen(path, mode);
    if (!f) {
        printf("Failed to open file\n");
        return;
    }

    printf("\n-- EDIT MODE --\n");
    printf(":w = save | :q = quit\n\n");

    char line[BUF_SIZE];
    int pos = 0;

    while (1) {
        uint8_t c;
        int len = uart_read_bytes(UART_NUM, &c, 1, portMAX_DELAY);

        if (len <= 0)
            continue;

        if (c == 3) {
            printf("\n^C (exit editor)\n");
            break;
        }

        if (c == '\r' || c == '\n') {
            line[pos] = '\0';

            printf("\n");

            if (strcmp(line, ":q") == 0) {
                printf("Exit without saving\n");
                break;
            }

            if (strcmp(line, ":w") == 0) {
                fflush(f);
                printf("Saved\n");
                pos = 0;
                continue;
            }

            fprintf(f, "%s\n", line);

            pos = 0;
        } else if (c == 127 || c == '\b') {
            if (pos > 0) {
                pos--;
                printf("\b \b");
                fflush(stdout);
            }
        } else if (pos < BUF_SIZE - 1) {
            line[pos++] = (char)c;
            printf("%c", c);
            fflush(stdout);
        }
    }

    fclose(f);

    printf("-- EXIT EDIT MODE --\n");
}

command_t commands[] = { 
    {"ls", cmd_ls}, 
    {"cat", cmd_cat}, 
    {"write", cmd_write}, 
    {"rm", cmd_rm}, 
    {"edit", cmd_edit}, 
    {"exit", cmd_exit}, 
    {NULL, NULL}
};