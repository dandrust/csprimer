#ifndef JOB_H
#define JOB_H

#include <sys/wait.h>

#define MAX_DEBUG_STRING_SIZE 1024

enum JobState {
    JOB_STATE_RUNNING,
    JOB_STATE_STOPPED,
    JOB_STATE_DONE
};

struct Job {
    int jid;
    pid_t pid;
    enum JobState state;
    char *command;
    char* (*print)(struct Job *);
    const char* (*stateLabel)(struct Job *);
    char printStr[MAX_DEBUG_STRING_SIZE];
};

struct Job *setupJob();
void cleanupJob(struct Job *);

#endif // JOB_H