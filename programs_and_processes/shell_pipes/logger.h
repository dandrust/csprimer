#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>

enum LogLevel
{
    LOG_SILENT,
    LOG_CRITICAL,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
};

struct Logger
{
    int log_fd;
    enum LogLevel verbosity;
    void (*initialize)(struct Logger *, char *, enum LogLevel);
    void (*debug)(struct Logger *, char *, ...);
    void (*info)(struct Logger *, char *, ...);
    void (*warn)(struct Logger *, char *, ...);
    void (*critical)(struct Logger *, char *, ...);
};

struct Logger *setupLogger();
void destroyLogger(struct Logger *);

#endif // LOGGER_H