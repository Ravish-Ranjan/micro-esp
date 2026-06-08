#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "./shell/commands.h"
#include "./fs/fs.h"

#define UART_NUM UART_NUM_0
#define BUF_SIZE 128

int shell_running = 1;

void app_main(void)
{
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

    char input[BUF_SIZE];
    int pos = 0;
    printf("Starting FS init...\n");
    init_fs();

    printf("> ");
    fflush(stdout);

    shell_running = 1;

    while (shell_running)
    {
        uint8_t c;
        int len = uart_read_bytes(UART_NUM, &c, 1, portMAX_DELAY);

        if (len <= 0)
            continue;

        if (c == 3)
        {
            printf("\n^C\n> ");
            pos = 0;
            fflush(stdout);
            continue;
        }
        else if (c == '\r' || c == '\n')
        {
            if (pos == 0)
            {
                printf("> ");
                fflush(stdout);
                continue;
            }
            input[pos] = '\0';
            pos = 0;

            printf("\n");
            execute_command(input);
            printf("> ");
            fflush(stdout);
        }
        else if (c == 127 || c == '\b')
        {
            if (pos > 0)
            {
                pos--;
                printf("\b \b");
                fflush(stdout);
            }
        }
        else if (pos < BUF_SIZE - 1)
        {
            input[pos++] = (char)c;
            printf("%c", c);
            fflush(stdout);
        }
        else if (c == 21)
        { // Ctrl+U
            while (pos > 0)
            {
                printf("\b \b");
                pos--;
            }
            fflush(stdout);
            continue;
        }
    }
}