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
    (void)argc;
    (void)argv;
    shell_running = 0;
    printf("Exiting shell...\n");
    fflush(stdout);
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

void cmd_mv(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: mv <old-file-name> <new-file-name>\n");
        return;
    }

    char src_path[128];
    char dest_path[128];
    snprintf(src_path, sizeof(src_path), "/spiffs/%s", argv[1]);
    snprintf(dest_path, sizeof(dest_path), "/spiffs/%s", argv[2]);

    if (rename(src_path, dest_path) == 0) {
        printf("Renamed: %s -> %s\n", src_path, dest_path);
        return;
    }

    FILE *src_f = fopen(src_path, "r");
    if (!src_f) {
        printf("Source file not found\n");
        return;
    }

    FILE *chk = fopen(dest_path, "r");
    if (chk) {
        fclose(chk);
        fclose(src_f);
        printf("Destination already exists\n");
        return;
    }

    FILE *dest_f = fopen(dest_path, "w");
    if (!dest_f) {
        printf("Failed to open destination: errno=%d\n", errno);
        fclose(src_f);
        return;
    }

    char buf[256];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src_f)) > 0) {
        if (fwrite(buf, 1, n, dest_f) != n) {
            printf("Write error: errno=%d\n", errno);
            break;
        }
    }

    fflush(dest_f);
    fclose(dest_f);
    fclose(src_f);

    if (remove(src_path) != 0) {
        printf("Failed to remove source: errno=%d\n", errno);
    }
}

void cmd_cp(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: cp <src> [dst]\n");
        return;
    }

    char src_path[256];
    snprintf(src_path, sizeof(src_path), "/spiffs/%s", argv[1]);

    FILE *src = fopen(src_path, "rb");
    if (!src) {
        printf("Source file not found\n");
        return;
    }

    char dest_path[256];
    char candidate[256];
    const char *target_name;
    int idx = 1;
    const size_t name_limit = sizeof(dest_path) - (sizeof("/spiffs/") - 1) - 1;

    if (argc >= 3) {
        target_name = argv[2];
        snprintf(dest_path, sizeof(dest_path), "/spiffs/%.*s",
                 (int)name_limit, target_name);

        if (strcmp(dest_path, src_path) == 0) {
            printf("Source and destination are the same\n");
            fclose(src);
            return;
        }

        FILE *chk = fopen(dest_path, "rb");
        if (chk) {
            fclose(chk);

            const char *dot = strrchr(target_name, '.');
            while (1) {
                if (dot && dot != target_name) {
                    int base_len = (int)(dot - target_name);
                    snprintf(candidate, sizeof(candidate), "%.*s_%d%s",
                             base_len, target_name, idx, dot);
                } else {
                    snprintf(candidate, sizeof(candidate), "%s_%d",
                             target_name, idx);
                }

                snprintf(dest_path, sizeof(dest_path), "/spiffs/%.*s",
                         (int)name_limit, candidate);

                FILE *c = fopen(dest_path, "rb");
                if (c) {
                    fclose(c);
                    idx++;
                    continue;
                }
                break;
            }
        }
    } else {
        target_name = argv[1];
        const char *dot = strrchr(target_name, '.');

        while (1) {
            if (dot && dot != target_name) {
                int base_len = (int)(dot - target_name);
                snprintf(candidate, sizeof(candidate), "%.*s_%d%s",
                         base_len, target_name, idx, dot);
            } else {
                snprintf(candidate, sizeof(candidate), "%s_%d",
                         target_name, idx);
            }

            snprintf(dest_path, sizeof(dest_path), "/spiffs/%.*s",
                     (int)name_limit, candidate);

            FILE *c = fopen(dest_path, "rb");
            if (c) {
                fclose(c);
                idx++;
                continue;
            }
            break;
        }
    }

    FILE *dest = fopen(dest_path, "wb");
    if (!dest) {
        printf("Failed to open destination: errno=%d\n", errno);
        fclose(src);
        return;
    }

    char buf[256];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        if (fwrite(buf, 1, n, dest) != n) {
            printf("Write error: errno=%d\n", errno);
            break;
        }
    }

    fflush(dest);
    fclose(dest);
    fclose(src);

    printf("Copied: %s -> %s\n", src_path, dest_path);
}

void cmd_clear(int argc, char **argv) {
    (void)argc;
    (void)argv;
    printf("\033[2J\033[H\n\n");
    fflush(stdout);
}

command_t commands[] = { 
    {"ls", cmd_ls}, 
    {"cat", cmd_cat}, 
    {"write", cmd_write}, 
    {"rm", cmd_rm}, 
    {"mv", cmd_mv}, 
    {"cp", cmd_cp}, 
    {"edit", cmd_edit}, 
    {"clear", cmd_clear}, 
    {"exit", cmd_exit}, 
    {NULL, NULL}
};