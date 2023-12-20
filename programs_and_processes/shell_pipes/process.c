#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "process.h"

char *processPrint(struct Process *p) {
    int written;
    char *start_at = p->printStr;

    written = sprintf(p->printStr, "Process for");
    start_at += written;

    for (int i = 0; p->args[i] != NULL; i++) {
        written = sprintf(start_at, " %s", p->args[i]);
        start_at += written;
    }
    
    return p->printStr;
}

struct Process *setupProcess() {
    struct Process *p = malloc(sizeof(struct Process));
    p->background = 0;
    p->print = processPrint;
}

void destroyProcess(struct Process *p) {
    free(p);
}