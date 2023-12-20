#ifndef PROCESS_H
#define PROCESS_H

#include <sys/wait.h>

#define MAX_ARGS 16
#define MAX_DEBUG_STRING_SIZE 1024

struct Process {
    char *args[MAX_ARGS];
    pid_t pid;
    char* (*print)(struct Process *);
    char printStr[MAX_DEBUG_STRING_SIZE];
    int background;
};

struct Process *setupProcess();
void destroyProcess(struct Process *);

#endif // PROCESS_H