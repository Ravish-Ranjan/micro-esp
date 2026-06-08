#ifndef COMMANDS_H
#define COMMANDS_H

typedef void (*cmd_func)(int argc,char** argv);

typedef struct {
    const char* name;
    cmd_func func;
} command_t;

void execute_command(char* input);

#endif