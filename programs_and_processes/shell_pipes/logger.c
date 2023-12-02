#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include "logger.h"

/*
    Private data and functions

    These are marked static to prevent access from outside of this file
*/

static const int MAX_MSG_LENGTH = 4096;

static const char *LABELS[] = {
    "",
    "CRITICAL",
    "WARN",
    "INFO",
    "DEBUG"
};

static void loggerLog(struct Logger *l, enum LogLevel level, char *format, va_list args) {
    if (level > l->verbosity) return;
    if (level == LOG_SILENT) return;

    char msg[MAX_MSG_LENGTH];

    vsprintf(msg, format, args);

    dprintf(l->log_fd, "[%s][%d] %s\n", LABELS[level], getpid(), msg);
}

/*
    Public functions

    These can be accessed as implementations of the struct's public interface
*/

void loggerWarn(struct Logger *l, char *format, ...)
{
    va_list args;
    va_start(args, format);
    loggerLog(l, LOG_WARN, format, args);
    va_end(args);
}

void loggerInfo(struct Logger *l, char* format, ...)
{
    va_list args;
    va_start(args, format);
    loggerLog(l, LOG_INFO, format, args);
    va_end(args);
}

void loggerDebug(struct Logger *l, char* format, ...) {
    va_list args;
    va_start(args, format);
    loggerLog(l, LOG_DEBUG, format, args);
    va_end(args);
}

void loggerCritical(struct Logger *l, char* format, ...) {
    va_list args;
    va_start(args, format);
    loggerLog(l, LOG_CRITICAL, format, args);
    va_end(args);
}


void loggerInitialize(struct Logger *l, char *path, enum LogLevel verbosity)
{
    int fd = open(path, O_CREAT | O_WRONLY | O_APPEND);
    if (fd == -1) perror("Unable to open log file");

    l->log_fd = fd;
    l->verbosity = verbosity;
}

/*
    Resource allocation and deallocation

    To be called from outside of the logger struct
*/
struct Logger *setupLogger()
{
    struct Logger *l = malloc(sizeof(struct Logger));
    l->initialize = loggerInitialize;
    l->warn = loggerWarn;
    l->info = loggerInfo;
    l->debug = loggerDebug;
    l->critical = loggerCritical;
    
    return l;
}

void destroyLogger(struct Logger *l) {
    free(l);
}
