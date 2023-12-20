#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include "logger.h"
#include "process.h"
#include "job.h"

#define CMD_BUFFER_SIZE 1024
#define MAX_JOBS 32
#define MAX_PROCESSES 8
#define TOKEN_EXIT "exit"
#define TOKEN_HELP "help"
#define TOKEN_LOGOUT "logout"
#define TOKEN_PIPE "|"
#define TOKEN_JOBS "jobs"
#define TOKEN_AMP "&"
#define TOKEN_FG "fg"
#define TOKEN_BG "bg"

struct Job *jobs[MAX_JOBS];

// TODO is this still being used?
volatile pid_t child_pid;

void render_prompt() {
    printf("> ");
    fflush(stdout);
}

void handle_sigint() {
    struct Logger *l = setupLogger();
    l->initialize(l, "shell_log.txt", LOG_DEBUG);
    
    l->debug(l, "SIGINT!");

    if(!child_pid) { 
        l->debug(l, "SIGINT caught by child before exec'ing. Returning...");
        exit(0);
    }
}

int parse_command_line(char *buffer, struct Process *processes[], struct Logger *logger) {
    int job_no = 0;
    int arg_no = 0;

    processes[job_no] = setupProcess();

    char *last_token;
    processes[job_no]->args[arg_no] = strtok(buffer, " \t\n");
    last_token = processes[job_no]->args[arg_no];
    logger->debug(logger, "read: %s (job %d, arg %d)", last_token, job_no, arg_no);

    while (last_token != NULL) { // until we reach end of string
        processes[job_no]->args[++arg_no] = strtok(NULL, " \t\n"); // get the next token
        last_token = processes[job_no]->args[arg_no];
        logger->debug(logger, "read: %s (job %d, arg %d)", last_token, job_no, arg_no);

        if (processes[job_no]->args[arg_no] != NULL && strcmp(TOKEN_PIPE, processes[job_no]->args[arg_no]) == 0) { // if the next token is a pipe
            logger->debug(logger, "pipe detected!");
            processes[job_no]->args[arg_no] = NULL; //      write NULL to args[i]
            
            // Pointer accounting (point to next job)
            job_no++;   //      increment the job index
            processes[job_no] = setupProcess();
            logger->debug(logger, "incremented job no to %d", job_no);
            arg_no = -1;   //      set arg index to zero
            logger->debug(logger, "reset arg no to %d", arg_no);
        }

        if (processes[job_no]->args[arg_no] != NULL && strcmp(TOKEN_AMP, processes[job_no]->args[arg_no]) == 0) { // if the next token is an `&`
            // If we find `&` assume that it's the last token and get out
            processes[job_no]->args[arg_no] = NULL;
            processes[job_no]->background = 1;
            break;
        }
    }
    logger->debug(logger, "found %d jobs", job_no + 1);
    return job_no + 1;
}

void handle_sigchild() {
    struct Logger *l = setupLogger();
    l->initialize(l, "shell_log.txt", LOG_DEBUG);
    
    l->debug(l, "SIGCHLD!");
    pid_t pid;
    
    int status;
    // find out which child has terminated
    while ((pid = waitpid(-1, &status, WNOHANG|WCONTINUED)) > 0 ) {
        l->debug(l, "signal recieved by child pid %d", pid);

        if (WIFEXITED(status)) {
            l->debug(l, "triggered by WIFEXITED (%d)", WEXITSTATUS(status));
        }

        if (WIFSIGNALED(status)) {
            l->debug(l, "triggered by WIFSIGNALED (sig %d)", WTERMSIG(status));
        }

        if (WIFSTOPPED(status)) { // This is what got triggered for SIGTSTP, signal code matched (20)
            l->debug(l, "triggered by WIFSTOPPED (sig %d)", WSTOPSIG(status));
        }

        if (WIFCONTINUED(status)) { // This is what got triggered by SIGCONT
            l->debug(l, "triggered by WIFCONTINUED");
        }

        int j = 0;
        // find the corresponding job
        while (jobs[j] != NULL) {
            if (jobs[j]->pid == pid) { 
                l->debug(l, "found child pid %d in jobs array", pid);
                // And update it's state
                jobs[j]->state = JOB_STATE_DONE;
            }
            j++;
        }
    }
}

void handle_sigtstop() {
    struct Logger *l = setupLogger();
    l->initialize(l, "shell_log.txt", LOG_DEBUG);
    
    l->debug(l, "SIGTSTP!");
    free(l);
}

/*
    Todo for job control:
    x Read the last token. If it's `&` don't wait on the pid (Single command) and add it to a jobs array
    x Implement a jobs builtin to print one line per job
    x SIGCHLD updates job state
    - print [jid] pid when background process starts
    - print [jib] Done <command> only on first `jobs` call after finish
    - Make `kill` a builtin wrapper that can intercept a job number (%1, %2, etc)

    Unknowns:
    - How can I prevent background process from reading from STDIN/STDOUT, then later make it read from those?
    - What signal can I use to suspend? Unsuspend?
    - Can you use a keyboard shortcut to push a running process into the background?
    - Will I need to set up an intermediary process for process groups so that I can send a signal all of them?
        - Will I need to do this for any/all foreground processes just in case they're pushed into the background?

    Thoughts:
    - Don't create a `Job` struct until you have to. Ie, it's issued with a trailing & OR the user pushes a fg process to the bg
    - Increment a counter for job no, for simplicity just implement an array to hold the jobs
    - 

    How can I change the terminal process group ID on the main thread?
        int tcsetpgrp(int fd, pid_t pgrp);
    
    What is SIGTTOU?

    How do I get the TTY fd?
        fileno(stdout)
*/
int main () {
    char cmd_buffer[CMD_BUFFER_SIZE];
    struct Process *processes[MAX_PROCESSES];
    int pipefd[2];
    int next_job_i = 0;
    int terminal_fd = fileno(stdout);
    
    for(int j = 0; j < MAX_JOBS; j++) {
        jobs[j] = NULL;
    }

    signal(SIGINT, handle_sigint);
    signal(SIGCHLD, handle_sigchild);
    signal(SIGTSTP, handle_sigtstop);

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

        int job_cnt = parse_command_line(cmd_buffer, processes, logger);

        if (processes[0]->args[0] == NULL) { 
            render_prompt();
            continue;
        }
        
        if ((strcmp(TOKEN_EXIT, processes[0]->args[0])) == 0 || (strcmp(TOKEN_LOGOUT, processes[0]->args[0])) == 0) {
            logger->info(logger, "Exiting application via %s", processes[0]->args[0]);
            logger->info(logger, "Goodbye!");
            return 0;
        }
        
        if ((strcmp(TOKEN_HELP, processes[0]->args[0])) == 0) {
            printf("help is on the way!\n");
            render_prompt();
            continue;
        }

        if ((strcmp(TOKEN_JOBS, processes[0]->args[0])) == 0) {
            logger->debug(logger, "Hit jobs builtin");
            
            int j = 0;

            while(jobs[j] != NULL) {
                logger->debug(logger, "jobs %p", jobs[j]);
                printf("%s\n", jobs[j]->print(jobs[j]));
                j++;
            }
            render_prompt();
            continue;
        }

        if ((strcmp(TOKEN_FG, processes[0]->args[0])) == 0) {
            logger->debug(logger, "Hit fg builtin");
            
            // Parse the arg (%n)
            // 
        }

        int input_fd = 0;

        for (int i = 0; i < job_cnt; i++) {
            logger->debug(logger, "Preparing to fork child %d (%s)", i, processes[i]->print(processes[i]));

            // If there is another process downstream create a pipe
            if (i < (job_cnt - 1)) {
                logger->debug(logger, "Creating a pipe on main thread");

                if (pipe(pipefd) == -1) {
                    perror("pipe");
                    exit;
                };
            }

            logger->debug(logger, "Forking child %d (%s)", i, processes[i]->print(processes[i]));
            
            child_pid = fork();
            processes[i]->pid = child_pid;
            
            if (processes[i]->pid < 0) {
                logger->debug(logger, "Unable to fork"); 
            } else if (processes[i]->pid == 0) {
                // Child Process


                logger->debug(logger, "Hello from child (%d) process", i);
                logger->debug(logger, "child_pid: %d", child_pid);

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

                logger->debug(logger, "Ready to exec");
                int rc_exec = execvp(processes[i]->args[0], processes[i]->args);
                if (rc_exec == -1) { 
                    return 0;
                }
            }

            // TODO: I have no idea what's up and what's down with all of this
            // This assignment sheet seems to have some pointers, maybe I'll try to start there next time
            // https://www.andrew.cmu.edu/course/15-310/applications/homework/homework4/lab4.pdf

            logger->debug(logger, "Setting term proc grp id to %d", child_pid);
            int foo = tcsetpgrp(terminal_fd, child_pid);
            if (foo == -1)
                perror("set terminal group");
            logger->debug(logger, "Terminal group is %d", tcgetpgrp(terminal_fd));

            logger->debug(logger, "Setting proc grp id of child %d to %d", child_pid, child_pid);
            int bar = setpgid(0, child_pid);
            if (bar == -1)
                perror("set process group");
            logger->debug(logger, "Process group for child (%d) is %d", child_pid, getpgid(child_pid));
            

            // Back in the parent thread...
            logger->debug(logger, "child_pid: %d", child_pid);

            if (i < job_cnt - 1) {
                logger->debug(logger, "Closing write-side of the pipe (fd %d) on main thread", pipefd[1]);
                close(pipefd[1]);
                
                logger->debug(logger, "Setting input_fd to %d for downstream (%s)", pipefd[0], processes[i+1]->print(processes[i+1]));
                input_fd = pipefd[0];
            } else {
                if (input_fd) { // don't close if input is coming from fd o aka STDIN
                    logger->debug(logger, "Closing read-side of the pipe (fd %d) on main thread", pipefd[1]);
                    close(pipefd[0]);
                }
            }
        }


        for (int i = 0; i < job_cnt; i++) {
            if (processes[i]->background) {
                logger->debug(logger, "Not waiting on on child %d running '%s' (pid %d)", i, processes[i]->args[0], processes[i]->pid);
                
                jobs[next_job_i] = setupJob();
                jobs[next_job_i]->state = JOB_STATE_RUNNING;
                jobs[next_job_i]->jid = next_job_i + 1;
                jobs[next_job_i]->pid = processes[i]->pid;
                next_job_i++;
            } else {
                logger->debug(logger, "Waiting on child %d running '%s' (pid %d)", i, processes[i]->args[0], processes[i]->pid);
                int fg_status;
                waitpid(processes[i]->pid, &fg_status, WUNTRACED);

                if (WIFEXITED(fg_status)) {
                    logger->debug(logger, "Child %d running '%s' (pid %d) has returned", i, processes[i]->args[0], processes[i]->pid);
                    free(processes[i]);
                } else if (WIFSTOPPED(fg_status)) {
                    logger->debug(logger, "Child %d running '%s' (pid %d) was stopped", i, processes[i]->args[0], processes[i]->pid);
                    jobs[next_job_i] = setupJob();
                    jobs[next_job_i]->state = JOB_STATE_STOPPED;
                    jobs[next_job_i]->jid = next_job_i + 1;
                    jobs[next_job_i]->pid = processes[i]->pid;
                    jobs[next_job_i]->command = processes[i]->print(processes[i]);

                    // TODO: Update struct Process to get only the command (instead of the debugging print string)
                    printf("\n[%d] %s\t%s\n", jobs[next_job_i]->jid, jobs[next_job_i]->stateLabel(jobs[next_job_i]), jobs[next_job_i]->command);

                    next_job_i++;

                    // TODO: something needs to change here so that the stopped process isn't reading/writing to STDIN/OUT
                    int foo = tcsetpgrp(terminal_fd, getpid());
                    if (foo == -1)
                        perror("set terminal group");
                    
                }
                
            }
        }

        render_prompt();
    }

    destroyLogger(logger);
}
