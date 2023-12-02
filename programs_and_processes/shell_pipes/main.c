#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include "logger.h"

#define CMD_BUFFER_SIZE 1024
#define MAX_JOBS 8
#define MAX_PIPES MAX_JOBS - 1
#define MAX_ARGS 16
#define TOKEN_EXIT "exit"
#define TOKEN_HELP "help"
#define TOKEN_LOGOUT "logout"
#define TOKEN_PIPE "|"

#define DEBUG 0

// TODO is this still being used?
volatile pid_t child_proc;

// Temporarily reference this as an extern
extern void execute(char *);

void render_prompt() {
    printf("> ");
    fflush(stdout);
}

void handle_sigint() {
    if(!child_proc) { 
        return;
    }

    kill(child_proc, SIGINT);
}

/* 
    Process Stuff
*/
struct Process {
    int ifd;
    int ofd;
    char *args[MAX_ARGS];
    pid_t pid;
    char* (*print)(struct Process *);
    char printStr[1024];
};

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
    p->print = processPrint;
}

void destroyProcess(struct Process *p) {
    free(p);
}

/* 
    END Process Stuff
*/

int parse_command_line(char *buffer, struct Process *jobs[], struct Logger *logger) {
    int job_no = 0;
    int arg_no = 0;

    jobs[job_no] = setupProcess();

    char *last_token;
    jobs[job_no]->args[arg_no] = strtok(buffer, " \t\n");
    last_token = jobs[job_no]->args[arg_no];
    logger->debug(logger, "read: %s (job %d, arg %d)", last_token, job_no, arg_no);

    while (last_token != NULL) { // until we reach end of string
        jobs[job_no]->args[++arg_no] = strtok(NULL, " \t\n"); // get the next token
        last_token = jobs[job_no]->args[arg_no];
        logger->debug(logger, "read: %s (job %d, arg %d)", last_token, job_no, arg_no);

        if (jobs[job_no]->args[arg_no] != NULL && strcmp(TOKEN_PIPE, jobs[job_no]->args[arg_no]) == 0) { // if the next token is a pipe
            logger->debug(logger, "pipe detected!");
            jobs[job_no]->args[arg_no] = NULL; //      write NULL to args[i]
            
            // Pointer accounting (point to next job)
            job_no++;   //      increment the job index
            jobs[job_no] = setupProcess();
            logger->debug(logger, "incremented job no to %d", job_no);
            arg_no = -1;   //      set arg index to zero
            logger->debug(logger, "reset arg no to %d", arg_no);
        }
    }
    return job_no + 1;
}

// struct Job {
//     char *command; // may not need this...
//     // pid
// };

// struct Process {
//     char *args[MAX_ARGS];
// };

// struct ProcessGroup {

// };



int main () {
    char cmd_buffer[CMD_BUFFER_SIZE];
    struct Process *jobs[MAX_JOBS];
    struct pipe *pipes[MAX_PIPES];
    int pipefd[2];

    signal(SIGINT, handle_sigint);

    struct Logger *logger = setupLogger();
    logger->initialize(logger, "shell_log.txt", LOG_DEBUG);

    logger->info(logger, "Application started");

    render_prompt();
    
    while (1) {
        if (feof(stdin)) { 
            printf("\n");
            return 0;
        }

        fgets(cmd_buffer, CMD_BUFFER_SIZE, stdin);

        int job_cnt = parse_command_line(cmd_buffer, jobs, logger);
        int pipe_cnt = job_cnt - 1;

        logger->debug(logger, "found %d jobs", job_cnt);

        if (jobs[0]->args[0] == NULL) { 
            render_prompt();
            continue;
        }
        
        if ((strcmp(TOKEN_EXIT, jobs[0]->args[0])) == 0 || (strcmp(TOKEN_LOGOUT, jobs[0]->args[0])) == 0) {
            logger->info(logger, "Exiting application via %s", jobs[0]->args[0]);
            logger->info(logger, "Goodbye!");
            return 0;
        }
        
        if ((strcmp(TOKEN_HELP, jobs[0]->args[0])) == 0) {
            printf("help is on the way!\n");
            render_prompt();
            continue;
        }

        int input_fd = 0;

        for (int i = 0; i < job_cnt; i++) {
            logger->debug(logger, "Preparing to fork child %d (%s)", i, jobs[i]->print(jobs[i]));

            // If there is another process downstream create a pipe
            if (i < (job_cnt - 1)) {
                logger->debug(logger, "Creating a pipe on main thread");

                if (pipe(pipefd) == -1) {
                    perror("pipe");
                    exit;
                };
            }

            logger->debug(logger, "Forking child %d (%s)", i, jobs[i]->print(jobs[i]));
            
            jobs[i]->pid = fork();

            if (jobs[i]->pid < 0) {
                logger->debug(logger, "Unable to fork"); 
            } else if (jobs[i]->pid == 0) {
                // Child Process
                
                logger->debug(logger, "Hello from child (%d) process", i);

                // Check input stream
                if (input_fd != STDIN_FILENO) {
                    logger->debug(logger, "Dup2-ing fd (%d) to STDIN (%d)", input_fd, STDIN_FILENO);
                    dup2(input_fd, STDIN_FILENO);

                    // I can close this because STDIN_FILENO now points to the 
                    // read end of the pipe, so closing STDIN will shut down the pipe
                    close(input_fd);
                }

                // Check output stream
                if (i < job_cnt - 1) {
                    logger->debug(logger, "Dup2-ing fd (%d) to stdout (%d)", pipefd[1], STDOUT_FILENO);
                    dup2(pipefd[1], STDOUT_FILENO);
                    
                    // I can close this fd because STDOUT_FILENO is still open 
                    // and pointing to the pipe's write side
                    close(pipefd[1]);
                }

                int rc_exec = execvp(jobs[i]->args[0], jobs[i]->args);
                if (rc_exec == -1) { 
                    return 0;
                }
            }

            // Back in the parent thread...
            if (i < job_cnt - 1) {
                logger->debug(logger, "Closing write-side of the pipe (fd %d) on main thread", pipefd[1]);
                close(pipefd[1]);
                
                logger->debug(logger, "Setting input_fd to %d for downstream (%s)", pipefd[0], jobs[i+1]->print(jobs[i+1]));
                input_fd = pipefd[0];
            } else {
                if (input_fd) { // don't close if input is coming from fd o aka STDIN
                    logger->debug(logger, "Closing read-side of the pipe (fd %d) on main thread", pipefd[1]);
                    close(pipefd[0]);
                }
            }
        }

        for (int i = 0; i < job_cnt; i++) {
            logger->debug(logger, "Waiting on child %d running '%s' (pid %d)", i, jobs[i]->args[0], jobs[i]->pid);
            waitpid(jobs[i]->pid, NULL, 0);
            logger->debug(logger, "Child %d running '%s' (pid %d) has returned", i, jobs[i]->args[0], jobs[i]->pid);
            free(jobs[i]);
        }

        render_prompt();
    }

    destroyLogger(logger);
}
