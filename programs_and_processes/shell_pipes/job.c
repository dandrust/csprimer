#include "job.h"
#include <stdio.h>
#include <stdlib.h>

static const char *LABELS[] = {
    "Running",
    "Stopped",
    "Done"
};

char *jobPrint(struct Job *j) {
    sprintf(j->printStr, "[%d]\t%s\t%d", j->jid, j->stateLabel(j), j->pid);
    return j->printStr;
}

const char *jobStateLabel(struct Job *j) {
    return LABELS[j->state];
}

struct Job *setupJob() {
    struct Job *j = malloc(sizeof(struct Job));
    j->print = jobPrint;
    j->stateLabel = jobStateLabel;
    return j;
}

void cleanupJob(struct Job *j) {
    free(j);
}
