#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "./shell/commands.h"
#include "./fs/fs.h"
#include "./shell/init_msg.h"

#define UART_NUM UART_NUM_0
#define BUF_SIZE 128
#define QUEUE_SIZE   20
static QueueHandle_t uart_queue;

int shell_running = 1;

void app_main(void){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, -1, -1);
    uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, QUEUE_SIZE, &uart_queue, 0);

    char input[BUF_SIZE];
    int pos = 0;
    printf("Starting FS init...\n");
    init_fs();
    init_msg();
    printf("> ");
    fflush(stdout);

    shell_running = 1;

    while (shell_running) {
        uint8_t c;
        int len = uart_read_bytes(UART_NUM, &c, 1, pdMS_TO_TICKS(100));

        if (!shell_running) break;
        
        if (len <= 0) continue;

        if (c == 3) {
            printf("\n^C\n> ");
            pos = 0;
            fflush(stdout);
            continue;
        } else if (c == '\r' || c == '\n') {
            if (pos == 0) {
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
        } else if (c == 127 || c == '\b') {
            if (pos > 0) {
                pos--;
                printf("\b \b");
                fflush(stdout);
            }
        } else if (pos < BUF_SIZE - 1) {
            input[pos++] = (char)c;
            printf("%c", c);
            fflush(stdout);
        } else if (c == 21) { 
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